#include "editor.h"
#include "compressor.h"
#include "io_manager.h"
#include "terminal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

Editor* editor_create(const char *filename) {
    Editor *ed = calloc(1, sizeof(Editor));
    if (!ed) return NULL;
    
    ed->buffer = gb_init(4 * 4096);  // 16 KB iniciales, alineados internamente
    if (!ed->buffer) {
        free(ed);
        return NULL;
    }
    
    ed->filename = filename;
    ed->mode = EDITOR_MODE_NORMAL;
    ed->dirty = false;
    return ed;
}

void editor_destroy(Editor *ed) {
    if (!ed) return;
    gb_destroy(ed->buffer);
    free(ed);
}

static bool parse_command(const char *buf, size_t len) {
    return (len == 3 && buf[0] == ':' && buf[1] == 'w' && buf[2] == 'q');
}

bool editor_save(Editor *ed) {
    if (!ed || !ed->buffer) return false;
    
    // Extraer texto contiguo del Gap Buffer
    char *text = NULL;
    size_t text_len = 0;
    if (!gb_get_text(ed->buffer, &text, &text_len)) {
        fprintf(stderr, "[editor] Error: no se pudo extraer texto\n");
        return false;
    }
    
    // Pipeline: Compresión en User Space
    void *compressed = NULL;
    size_t compressed_len = 0;
    if (!compress_buffer(text, text_len, &compressed, &compressed_len)) {
        fprintf(stderr, "[editor] Error: compresión fallida\n");
        free(text);
        return false;
    }
    free(text);  // Ya no necesitamos texto plano
    
    // I/O: Escritura con header packed + buffers alineados
    bool ok = io_write_compressed(ed->filename, compressed, compressed_len, text_len);
    
    free(compressed);
    if (ok) ed->dirty = false;
    return ok;
}

bool editor_load(Editor *ed) {
    if (!ed || !ed->filename) return false;
    
    void *compressed = NULL;
    size_t compressed_len = 0, original_len = 0;
    
    if (!io_read_compressed(ed->filename, &compressed, &compressed_len, &original_len)) {
        if (errno == ENOENT) return true;  // Archivo nuevo: OK
        fprintf(stderr, "[editor] Error: no se pudo leer '%s': %s\n", 
                ed->filename, strerror(errno));
        return false;
    }
    
    // Descompresión en User Space
    char *decompressed = malloc(original_len);
    if (!decompressed) {
        free(compressed);
        return false;
    }
    
    if (!decompress_buffer(compressed, compressed_len, decompressed, original_len)) {
        fprintf(stderr, "[editor] Error: descompresión fallida\n");
        free(compressed);
        free(decompressed);
        return false;
    }
    free(compressed);
    
    // Cargar en Gap Buffer
    bool ok = gb_load_text(ed->buffer, decompressed, original_len);
    free(decompressed);
    return ok;
}

bool editor_handle_key(Editor *ed, KeyCode key) {
    if (!ed) return false;
    
    // Manejo de modo comando
    if (ed->mode == EDITOR_MODE_COMMAND) {
        if (key == KEY_ENTER) {
            if (parse_command(ed->command_buf, ed->command_len)) {
                if (editor_save(ed)) {
                    term_println("\r\n✓ Guardado. Saliendo...");
                    return true;  // Salir del loop principal
                } else {
                    term_println("\r\n✗ Error al guardar");
                }
            } else {
                // Comando no reconocido: insertar como texto
                gb_insert(ed->buffer, ":", 1);
                gb_insert(ed->buffer, ed->command_buf, ed->command_len);
                ed->dirty = true;
            }
            ed->mode = EDITOR_MODE_NORMAL;
            ed->command_len = 0;
            memset(ed->command_buf, 0, sizeof(ed->command_buf));
            return false;
        } else if (key == KEY_BACKSPACE) {
            if (ed->command_len > 0) ed->command_len--;
            return false;
        } else if (key >= 32 && key <= 126 && ed->command_len < sizeof(ed->command_buf) - 1) {
            ed->command_buf[ed->command_len++] = (char)key;
            return false;
        }
        // Ignorar otras teclas en modo comando
        return false;
    }
    
    // Modo normal: edición
    if (key == ':') {
        ed->mode = EDITOR_MODE_COMMAND;
        ed->command_len = 0;
        return false;
    } else if (key == KEY_BACKSPACE) {
        gb_delete(ed->buffer, 1);
        ed->dirty = true;
    } else if (key == KEY_ENTER) {
        gb_insert(ed->buffer, "\n", 1);
        ed->dirty = true;
    } else if (key == KEY_ARROW_LEFT) {
        // Simplificado: mover cursor lógico (Gap Buffer maneja posición interna)
        // En una versión completa, se trackearía cursor_pos explícitamente
    } else if (key == KEY_ARROW_RIGHT) {
        // Similar para derecha
    } else if (key >= 32 && key <= 126) {
        char c = (char)key;
        gb_insert(ed->buffer, &c, 1);
        ed->dirty = true;
    }
    // Ignorar teclas no manejadas
    
    return false;
}

void editor_render_status(const Editor *ed) {
    if (!ed) return;
    char status[128];
    int len = snprintf(status, sizeof(status), 
                      "\r%s [%s] %zu bytes | :wq para guardar",
                      ed->filename ?: "(nuevo)",
                      ed->dirty ? "MOD" : "OK",
                      ed->buffer->length);
    ssize_t written = write(STDOUT_FILENO, status, (size_t)len);
    (void)written;
}