#include "main.h"
#include "uart.h"

static void (*line_callback)(char*) = NULL;
static char input_buffer[1024];
static int buffer_pos = 0;
typedef struct {
    void (*callback)(uint32_t, void*);
    int time;
    uint32_t cookie;
} event_t;

static event_t events[100];
static int event_count = 0;

void ajouter_event(uint32_t delay, void (*callback)(uint32_t, void*),uint32_t cookie) {
    if (event_count >= 100) {
        return;
    }

    int insert_pos = event_count;
    for (int i = 0; i < event_count; i++) {
        if (events[i].time > (int)delay) {
            insert_pos = i;
            break;
        }
    }

    for (int j = event_count; j > insert_pos; j--) {
        events[j] = events[j - 1];
    }

    events[insert_pos].callback = callback;
    events[insert_pos].time = (int)delay;
    events[insert_pos].cookie = cookie;
    event_count++;
}

void time_step() {
    for (int i = 0; i < event_count; i++) {
        if (events[i].time > 0) {
            events[i].time--;
        }
    }
}

void executer_events() {
    for (int i = 0; i < event_count; i++) {
        if (events[i].time == 0) {
            events[i].callback(events[i].cookie, NULL);
            for (int j = i; j < event_count - 1; j++) {
                events[j] = events[j + 1];
            }
            event_count--;
            i--;
        }
    }
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

//initialise la console 
void console_init(void (*callback)(char*)) {
    line_callback = callback;
    buffer_pos = 0;
    input_buffer[0] = '\0';
    console_clear();
    cursor_at(0, 0);
    cursor_show();
}

uint8_t state = 0;

// gère les entrées clavier et les commande spéciales
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

uint32_t cursor_timer = 0;
char chars[8]= { '|', '/', '-', '\\', '|', '/', '-', '\\', };
uint8_t pos = 0;

void funny_cursor(uint32_t cookie, void* data) {
    pos = (pos + 1) % 8;
    kprintf("\b%c", chars[pos]);
    ajouter_event(500000, funny_cursor, 0);
}

void console_echo_event(uint32_t cookie, void* data) {
        (void)data;
        console_echo((uint8_t)cookie);
}

void _start() {
    console_init(NULL);
    ajouter_event(500000, funny_cursor, 0);
    while (1) {
        uint8_t c;
        if (0 != uart_receive(UART0,&c)){
                ajouter_event(0, console_echo_event, (uint32_t)c);
        }
        time_step();
        executer_events();
    }
}