#ifndef EDITOR_H
#define EDITOR_H

#include <stdbool.h>
#include "terminal.h"
#include "gap_buffer.h"

/** Estado del editor (máquina de estados simple) */
typedef enum {
    EDITOR_MODE_NORMAL,
    EDITOR_MODE_COMMAND,  // Esperando ":wq"
} EditorMode;

/** Contexto del editor: coordina buffer, modo, comandos */
typedef struct {
    GapBuffer *buffer;
    EditorMode mode;
    char command_buf[32];
    size_t command_len;
    const char *filename;
    bool dirty;  // ¿Hay cambios sin guardar?
} Editor;

/** Crea y inicializa el editor */
Editor* editor_create(const char *filename);

/** Libera recursos del editor */
void editor_destroy(Editor *ed);

/** Procesa una tecla y actualiza estado. Retorna true si debe salir. */
bool editor_handle_key(Editor *ed, KeyCode key);

/** Guarda el archivo (coordina compresión + I/O) */
bool editor_save(Editor *ed);

/** Carga un archivo existente (coordina I/O + descompresión) */
bool editor_load(Editor *ed);

/** Renderiza estado mínimo en terminal (para feedback visual) */
void editor_render_status(const Editor *ed);
void editor_render(Editor *ed);

#endif /* EDITOR_H */