#include "main.h"
#include "uart.h"

static void (*line_callback)(char*) = NULL;
static char input_buffer[1024];
static int buffer_pos = 0;

void cursor_left(){
    kprintf("\033[1D");
}
void cursor_right(){
    kprintf("\033[1C");
}
void cursor_down(){
    kprintf("\033[1B");
}
void cursor_up(){
    kprintf("\033[1A");
}
void cursor_at(int row, int col){
    kprintf("\033[%d;%dH", row+1, col+1);
}

void wait_for_char(uint8_t* c){
    while (0 == uart_receive(UART0, c)){
        continue;
    }
}

void cursor_position(int* row, int* col){
    kprintf("\033[6n");
    uint8_t c;
    wait_for_char(&c);
    if (c != '\033'){
        return;
    }
    wait_for_char(&c);
    if (c != '['){
        return;
    }
    wait_for_char(&c);
    *row = 0;
    *col = 0;
    while(c != ';'){
        if (c >= '0' && c <= '9'){
            *row = (*row)*10 + (c - '0');
        }
        wait_for_char(&c);
    }
    wait_for_char(&c);
    if (c != ';'){
        return;
    }
    while(c != 'R'){
        if (c >= '0' && c <= '9'){
            *col = (*col)*10 + (c - '0');
        }
        wait_for_char(&c);
    }
}
void cursor_hide(){
    kprintf("\033[?25l");
}
void cursor_show(){
    kprintf("\033[?25h");
}
void console_color(uint8_t color){
    kprintf("\033[%dm", color);
}
void console_clear(){
    kprintf("\033[2J\033[H");
}
void console_init(void (*callback)(char*)) {
    line_callback = callback;
    buffer_pos = 0;
    input_buffer[0] = '\0';
    console_clear();
    cursor_at(0, 0);
    cursor_show();
}
void console_echo(uint8_t byte){
    static int escape_state = 0;
    
    if (escape_state == 0 && byte == '\033') { // ESC
        escape_state = 1;
        return;
    }
    if (escape_state == 1) {
        if (byte == '[') {
            escape_state = 2;
            return;
        }
        escape_state = 0;
        return;
    }
    if (escape_state == 2) {
        escape_state = 0;
        switch(byte) {
            case 'A': cursor_up(); return;
            case 'B': cursor_down(); return;
            case 'C': cursor_right(); return;
            case 'D': cursor_left(); return;
            case '3': // Delete key sequence ESC[3~
                wait_for_char(&byte);
                if (byte == '~') {
                    // Gérer delete si nécessaire
                }
                return;
        }
        return;
    }
    
    if (byte == 3) {
        console_clear();
        buffer_pos = 0;
        input_buffer[0] = '\0';
        return;
    }
    
    if (byte == 10 || byte == 13) {
        input_buffer[buffer_pos] = '\0';
        kprintf("\n");
        if (line_callback != NULL) {
            line_callback(input_buffer);
        }
        buffer_pos = 0;
        input_buffer[0] = '\0';
        return;
    }
    
    if (byte == 127 || byte == 8) {
        if (buffer_pos > 0) {
            buffer_pos--;
            input_buffer[buffer_pos] = '\0';
            kprintf("\b \b"); // Efface le caractère à l'écran
        }
        return;
    }
    
    if (byte >= 32 && byte <= 126) {
        if (buffer_pos < sizeof(input_buffer) - 1) {
            input_buffer[buffer_pos++] = byte;
            input_buffer[buffer_pos] = '\0';
            kprintf("%c", byte);
        }
        return;
    }
}
void _start() {
  console_init(NULL);
  kprintf("\nFor information:\n");
  while (1) {
    uint8_t c;
    //console_clear();
    if (0==uart_receive(UART0,&c))
      continue;
    console_echo(c);
  }
}
