#include "terminal.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <errno.h>

static struct termios orig_term;
static bool term_initialized = false;

bool term_init(void) {
    if (term_initialized) return true;
    
    if (tcgetattr(STDIN_FILENO, &orig_term) == -1) {
        perror("[terminal] tcgetattr");
        return false;
    }
    
    struct termios raw = orig_term;
    raw.c_lflag &= ~(ICANON | ECHO | ISIG);  // Modo raw: sin buffering, sin eco, sin señales
    raw.c_iflag &= ~(IXON | ICRNL);          // Sin control de flujo, sin mapeo \r\n
    raw.c_oflag &= ~(OPOST);                 // Sin procesamiento de salida
    raw.c_cc[VMIN] = 1;  // read() bloquea hasta 1 carácter
    raw.c_cc[VTIME] = 0; // Sin timeout
    
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        perror("[terminal] tcsetattr");
        return false;
    }
    
    term_initialized = true;
    atexit(term_restore);
    return true;
}

void term_restore(void) {
    if (!term_initialized) return;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_term);
    term_initialized = false;
}

KeyCode term_read_key(void) {
    char c;
    ssize_t n = read(STDIN_FILENO, &c, 1);
    if (n != 1) return KEY_NONE;
    
    if (c == '\x1b') {  // Escape: posible secuencia de flechas
        char seq[2];
        // Lectura no bloqueante para secuencias
        if (read(STDIN_FILENO, &seq[0], 1) == 1 && seq[0] == '[') {
            if (read(STDIN_FILENO, &seq[1], 1) == 1) {
                switch (seq[1]) {
                    case 'A': return KEY_ARROW_UP;
                    case 'B': return KEY_ARROW_DOWN;
                    case 'C': return KEY_ARROW_RIGHT;
                    case 'D': return KEY_ARROW_LEFT;
                }
            }
        }
        return KEY_ESCAPE;  // Escape solo
    }
    
    return (KeyCode)c;
}

void term_print(const char *text) {
    if (!text) return;
    ssize_t written = write(STDOUT_FILENO, text, strlen(text));  // Syscall directa, sin buffering de stdio
    (void)written;
}

void term_println(const char *text) {
    term_print(text);
    term_print("\r\n");
}

void term_clear_screen(void) {
    term_print("\x1b[2J\x1b[H");  // ANSI: clear screen + home cursor
}

void term_move_cursor(unsigned int row, unsigned int col) {
    char buf[32];
    int len = snprintf(buf, sizeof(buf), "\x1b[%u;%uH", row + 1, col + 1);
    ssize_t written = write(STDOUT_FILENO, buf, (size_t)len);
    (void)written;
}