#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdbool.h>
#include <stdint.h>

/* Códigos de teclas especiales (ANSI/escape) */
typedef enum {
    KEY_NONE = 0,
    KEY_ENTER = '\r',
    KEY_BACKSPACE = '\x7f',
    KEY_ESCAPE = '\x1b',
    KEY_ARROW_UP = 1000,
    KEY_ARROW_DOWN,
    KEY_ARROW_LEFT,
    KEY_ARROW_RIGHT,
} KeyCode;

/* Inicializa terminal en modo raw (sin buffering, sin eco) */
bool term_init(void);

/* Restaura terminal a modo normal (llamar en cleanup) */
void term_restore(void);

/* Lee un carácter/tecla especial de stdin (no bloqueante) */
KeyCode term_read_key(void);

/* Imprime texto en stdout con manejo seguro de secuencias */
void term_print(const char *text);
void term_println(const char *text);

/* Limpia pantalla y mueve cursor a home */
void term_clear_screen(void);
void term_move_cursor(unsigned int row, unsigned int col);

#endif /* TERMINAL_H */