#include "main.h"
#include "uart.h"
#include "isr.h"
#include "ring.h"   
#include "console.h"

static ring_t ring_RX; //ring pour stocker les caractères reçus de l'uart
static ring_t ring_TX; //ring pour stocker les caractères à envoyer à l'uart

//******************************handler********************************

bool_t change_color = 0;
bool_t TX_ON = 0;


// fonction d'interruption de l'UART0 RX qui vas stoquer tou les les caractères reçus dans une ring
void uartRX_handler() {
    uint8_t c;
    while(0 != uart_receive(UART0, &c)){
        if(!ring_full(&ring_RX)) {
            ring_put(&ring_RX, c);
        }
    }
}

// fonction d'interruption de l'UART0 TX qui vas envoyer les caractères stockés dans la ring
void uartTX_handler() {
    TX_ON = 1;
}

// fonction d'interuption uart qui renvois au bon handler en fonction du type d'interuption (RX ou TX)
void uart_handler(uint32_t irq, uint8_t cookie) {
    switch (irq) {
        case RX_IRQ:
            uartRX_handler();
            break;
        case TX_IRQ:
            uartTX_handler();
            break;
    }
}

// fonction d'interuption du timer qui change la couleur du texte et du fond
void timer_handler(uint32_t vide, uint8_t cookie) {
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
    // deplace le curseur d'une position vers la gauche ESC[D
    ring_put(&ring_TX, '\033');
    ring_put(&ring_TX, '[');  
    ring_put(&ring_TX, 'D');
}
void cursor_right(){
    // deplace le curseur d'une position vers la droite ESC[C
    ring_put(&ring_TX, '\033');
    ring_put(&ring_TX, '[');  
    ring_put(&ring_TX, 'C');  
}
void cursor_down(){
    // deplace le curseur d'une position vers le bas ESC[B
    ring_put(&ring_TX, '\033');
    ring_put(&ring_TX, '[');
    ring_put(&ring_TX, 'B');
}
void cursor_up(){
    // deplace le curseur d'une position vers le haut ESC[A
    ring_put(&ring_TX, '\033'); 
    ring_put(&ring_TX, '[');  
    ring_put(&ring_TX, 'A');  
}

//deplace le curseur à une position précise (row, col)
void cursor_at(int row, int col){
    //deplace le curseur à la position (row, col) ESC[row;colH
    ring_put(&ring_TX, '\033');
    ring_put(&ring_TX, '[');   
    ring_put_int(&ring_TX, row+1);
    ring_put(&ring_TX, ';');  
    ring_put_int(&ring_TX, col+1); 
    ring_put(&ring_TX, 'H');   
}

//attend la reception d'un caractère et le stocke dans c
void wait_for_char(uint8_t* c){
    while (0 == uart_receive(UART0, c)){
        continue;
    }
}

//demande la position actuelle du curseur et la stocke dans row et col
void cursor_position(int* row, int* col){
    ring_put(&ring_TX, '\033'); // ESC : debut d'une commande spéciale
    ring_put(&ring_TX, '[');   // ESC[ : suite d'une commande spéciale
    ring_put(&ring_TX, '6');   // ESC[6n : demande la position actuelle du curseur
    ring_put(&ring_TX, 'n');   // ESC[6n : demande la position actuelle du curseur


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
    //cache le curseur ESC[?25l
    ring_put(&ring_TX, '\033');
    ring_put(&ring_TX, '[');  
    ring_put(&ring_TX, '?'); 
    ring_put(&ring_TX, '2');   
    ring_put(&ring_TX, '5');   
    ring_put(&ring_TX, 'l');  
}
void cursor_show(){
    //affiche le curseur ESC[?25h
    ring_put(&ring_TX, '\033');
    ring_put(&ring_TX, '[');
    ring_put(&ring_TX, '?');
    ring_put(&ring_TX, '2');   
    ring_put(&ring_TX, '5');   
    ring_put(&ring_TX, 'h');
}

//change la couleur du texte ou du fond
void console_color(uint8_t color){
    //change la couleur ESC[color m
    ring_put(&ring_TX, '\033'); 
    ring_put(&ring_TX, '['); 
    ring_put_int(&ring_TX, color);
    ring_put(&ring_TX, 'm');   
}

//efface l'ecran
void console_clear(){
    //efface l'ecran ESC[2J
    ring_put(&ring_TX, '\033');
    ring_put(&ring_TX, '['); 
    ring_put(&ring_TX, '2'); 
    ring_put(&ring_TX, 'J'); 
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
        ring_put(&ring_TX, '\n'); // affiche un retour à la ligne
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
        ring_put(&ring_TX, '\b'); // Efface le caractère à l'écran
        ring_put(&ring_TX, ' ');
        return;
    }
    
    if (byte >= 32 && byte <= 126) { // Caractère imprimable : ajoute à la ligne en cours
        if (buffer_pos < sizeof(input_buffer) - 1) {
            input_buffer[buffer_pos++] = byte;
            input_buffer[buffer_pos] = '\0';
            ring_put(&ring_TX, byte);
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
    ring_RX.head = 0;
    ring_RX.tail = 0;
    ring_TX.head = 0;
    ring_TX.tail = 0;    
    

    irqs_setup();
    irqs_enable();
        
    irq_enable(UART0_IRQ, (void(*)(uint32_t, void*))uart_handler, NULL); //active l'interuption uart0 RX sur le VIC

    console_init(NULL);
    mmio_write32(UART0, 0x38, (1 << 4) | (1 << 5)); //active les interuption TX et RX sur l'uart0
    mmio_write32(TIMER0, 0x00, 100000); //set la valeur du timer0
    mmio_write32(TIMER0, 0x08, ((1<<5)|(1<<7)| (1<<6))); //active les interuption timer, le timer0 et le mode periodique
    irq_enable(TIMER0_IRQ, (void(*)(uint32_t, void*))timer_handler, NULL); //active l'interuption timer0 sur le VIC
    TX_ON = 1;

    //boucle incipale
    while (1) {
        if(!ring_empty(&ring_RX)) { 
            uint8_t c = ring_get(&ring_RX);
            console_echo(c);
        }
        if(change_color) {
            change_color = 0;
            funny_cursor(0);
        }
        if(TX_ON && !ring_empty(&ring_TX)) {
            irqs_disable(); //pour ne pas avoir un nouveau caractere à envoyer avant d'avoir fini d'envoyer le caractere en cours
            uart_send(UART0, ring_get(&ring_TX));
            TX_ON = 0;
            irqs_enable();
        }
        irqs_disable(); //pour ne pas avoir un nouveau caractere avant de dormire
        if(ring_empty(&ring_RX)&& ring_empty(&ring_TX)) { //si il n'y a pas de caractere à envoyer ou à recevoir on dort
            wfi();            
        }
        irqs_enable();
    }
}

