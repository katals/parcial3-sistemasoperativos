/**
 * main.c - Orquestador del editor
 * Responsabilidad única: inicializar módulos, ejecutar event loop, cleanup.
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "terminal.h"
#include "editor.h"

static volatile sig_atomic_t running = 1;

static void signal_handler(int sig) {
    (void)sig;
    running = 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <archivo>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Setup de señales para cleanup limpio
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Inicializar terminal
    if (!term_init()) {
        fprintf(stderr, "[main] Error: no se pudo inicializar terminal\n");
        return EXIT_FAILURE;
    }

    // Crear editor y cargar archivo
    Editor *ed = editor_create(argv[1]);
    if (!ed) {
        term_restore();
        fprintf(stderr, "[main] Error: no se pudo crear editor\n");
        return EXIT_FAILURE;
    }

    if (!editor_load(ed)) {
        editor_destroy(ed);
        term_restore();
        return EXIT_FAILURE;
    }

    // UI inicial
    term_clear_screen();
    term_println("--- EDI-OPTIM v1.0 ---");
    term_println("Flechas: mover | Backspace: borrar | :wq: guardar y salir");
    term_println("----------------------");
    editor_render_status(ed);
    term_print("\r\n");

    // Event Loop Principal
    while (running) {
        KeyCode key = term_read_key();
        if (key == KEY_NONE) continue;
        
        bool should_exit = editor_handle_key(ed, key);
        editor_render_status(ed);
        
        if (should_exit) break;
    }

    // Cleanup ordenado
    editor_destroy(ed);
    term_restore();
    term_println("\r\nEditor cerrado.");
    
    return EXIT_SUCCESS;
}