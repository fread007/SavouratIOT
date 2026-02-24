#include "main.h"
#include "uart.h"
#include "isr.h"
#include "ring.h"   
#include "console.h"

//******************************handler********************************

bool_t change_color = 0;

// fonction d'interruption de l'UART0 qui vas stoquer tou les les caractères reçus dans une ring
void uart_handler(uint8_t cookie) {
    uint8_t c;
    while(0 != uart_receive(UART0, &c)){
        if(!ring_full()) {
            ring_put(c);
        }
    }
}

void timer_handler(uint8_t cookie) {
    change_color = 1;
}


//***************************Gestion UART********************************

static void (*line_callback)(char*) = NULL;
static char input_buffer[1024];
static int buffer_pos = 0;
static char tmp = ' '; 

//initialise la console 
void console_init(void (*callback)(char*)) {
    line_callback = callback;
    buffer_pos = 0;
    input_buffer[0] = '\0';
    console_clear();
    cursor_at(0, 0);
    cursor_show();
}

//deplace le curseur d'une position vers la gauche, droite, haut ou bas
void cursor_left(){
    kprintf("\033[1D"); // ESC[1D : deplace le curseur d'une position vers la gauche
}
void cursor_right(){
    kprintf("\033[1C"); // ESC[1C : deplace le curseur d'une position vers la droite
}
void cursor_down(){
    kprintf("\033[1B"); // ESC[1B : deplace le curseur d'une position vers le bas
}
void cursor_up(){
    kprintf("\033[1A"); // ESC[1A : deplace le curseur d'une position vers le haut
}

//deplace le curseur à une position précise (row, col)
void cursor_at(int row, int col){
    kprintf("\033[%d;%dH", row+1, col+1); // ESC[row;colH : deplace le curseur à la position (row, col) (1-based)
}

//attend la reception d'un caractère et le stocke dans c
void wait_for_char(uint8_t* c){
    while (0 == uart_receive(UART0, c)){
        continue;
    }
}

//demande la position actuelle du curseur et la stocke dans row et col
void cursor_position(int* row, int* col){
    kprintf("\033[6n"); // ESC[6n : demande la position actuelle du curseur

    uint8_t c;

    // La réponse doit être de la forme ESC[row;colR
    wait_for_char(&c);
    if (c != '\033'){ // ESC
        return;
    }
    wait_for_char(&c); 
    if (c != '['){
        return;
    }
    wait_for_char(&c);
    //debut de la reseption de la position du curseur
    *row = 0;
    *col = 0;
    while(c != ';'){    //caractere de separation entre row et col
        if (c >= '0' && c <= '9'){
            *row = (*row)*10 + (c - '0');
        }
        wait_for_char(&c);
    }
    wait_for_char(&c);
    while(c != 'R'){    //caractere de fin de la position du curseur
        if (c >= '0' && c <= '9'){
            *col = (*col)*10 + (c - '0');
        }
        wait_for_char(&c);
    }
}

//cache ou affiche le curseur
void cursor_hide(){
    kprintf("\033[?25l");   // ESC[?25l : cache le curseur
}
void cursor_show(){
    kprintf("\033[?25h"); // ESC[?25h : affiche le curseur
}

//change la couleur du texte ou du fond
void console_color(uint8_t color){
    kprintf("\033[%dm", color); // ESC[color m : change la couleur du texte ou du fond
}

//efface l'ecran et place le curseur en haut à gauche
void console_clear(){
    kprintf("\033[2J"); // ESC[2J : efface l'écran
    kprintf("\033[H"); // ESC[H : place le curseur en haut à gauche
}

// gère les entrées clavier et les commande spéciales

uint8_t state = 0;  //etats pour la gestion des commandes spéciales

void console_echo(uint8_t byte){
    switch (state)
    {
    case 0:
        if (byte == '\033') { // ESC : debut d'une commande spéciale
            state = 1;
            return;
        }
        break;
    case 1:
        if(byte == '[') {
            state = 2;
            return;
        }
        state = 0;
        break;
    case 2:
        state = 0;
        switch(byte) {
            case 'A': 
                cursor_up(); 
                return;
            case 'B': 
                cursor_down(); 
                return;
            case 'C': 
                cursor_right(); 
                return;
            case 'D': 
                cursor_left(); 
                return;
        }
        break;
    default:
        break;
    }
    
    if (byte == 3) { // ESC[\3 = Ctrl+C : efface la ligne en cours
        console_clear();
        buffer_pos = 0;
        input_buffer[0] = '\0';
        return;
    }
    
    if (byte == 10 || byte == 13) { // Enter : termine la ligne et appelle le callback
        input_buffer[buffer_pos] = '\0';
        kprintf("\n");
        if (line_callback != NULL) {
            line_callback(input_buffer);
        }
        buffer_pos = 0;
        input_buffer[0] = '\0';
        return;
    }
    
    if (byte == 127 || byte == 8) { // Backspace : efface le dernier caractère
        if (buffer_pos > 0) {
            buffer_pos--;
            input_buffer[buffer_pos] = '\0';
        }
        kprintf("\b \b"); // Efface le caractère à l'écran
        return;
    }
    
    if (byte >= 32 && byte <= 126) { // Caractère imprimable : ajoute à la ligne en cours
        if (buffer_pos < sizeof(input_buffer) - 1) {
            input_buffer[buffer_pos++] = byte;
            input_buffer[buffer_pos] = '\0';
            kprintf("%c", byte);
        }
        return;
    }
}

//***************************Gestion Curseur********************************

// gestion de l'evenement timer qui fait tourner le curseur
uint32_t color[] = {BLACK ,RED ,GREEN ,YELLOW ,BLUE ,MAGENTA ,CYAN ,WHITE};
uint32_t bgColor[] = {BG_BLACK ,BG_RED ,BG_GREEN ,BG_YELLOW ,BG_BLUE ,BG_MAGENTA ,BG_CYAN ,BG_WHITE};
uint8_t pos = 0;
uint8_t bgPos = 0;

void funny_cursor(uint8_t vide) {

    pos = (pos + 1) % 8;
    bgPos = (bgPos + 3) % 8;
    console_color(color[pos]);
    console_color(bgColor[bgPos]);
}


//******************************main********************************/

// fonction d'entrée avec la boucle principale
void _start() {
    //initialisation
    console_init(NULL);
    //cursor_hide();
    irqs_setup();
    irqs_enable();
    mmio_write32(UART0, 0x38, (1 << 4));    //active les interuption sur l'uart0
    irq_enable(UART0_IRQ, (void(*)(uint8_t, void*))uart_handler, NULL); //active l'interuption uart0 sur le VIC

    mmio_write32(TIMER0, 0x00, 100000); //set la valeur du timer0
    mmio_write32(TIMER0, 0x08, ((1<<5)|(1<<7)| (1<<6))); //active les interuption timer, le timer0 et le mode periodique
    irq_enable(TIMER0_IRQ, (void(*)(uint8_t, void*))timer_handler, NULL); //active l'interuption timer0 sur le VIC

    //boucle incipale
    while (1) {
        if(!ring_empty()) { 
            uint8_t c = ring_get();
            console_echo(c);
        }
        if(change_color) {
            change_color = 0;
            funny_cursor(0);
        }
        irqs_disable(); //pour ne pas avoir un nouveau caractere avant de dormire
        if(ring_empty()) {
            wfi();
        }
        irqs_enable();
    }
}

