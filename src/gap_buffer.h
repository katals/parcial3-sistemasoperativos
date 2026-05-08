#ifndef GAP_BUFFER_H
#define GAP_BUFFER_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * Gap Buffer: Estructura contigua en RAM optimizada para editores de texto.
 * - Todos los punteros y capacidades están alineados a página (4KB).
 * - No usa terminadores \0 para delimitar datos.
 * - Preparado para serialización binaria y compresión en User Space.
 */
typedef struct {
    char   *data;          // Bloque alineado en RAM
    size_t  capacity;      // Tamaño físico reservado (múltiplo de 4096)
    size_t  gap_start;     // Inicio del hueco (índice lógico)
    size_t  gap_end;       // Fin del hueco (índice físico)
    size_t  length;        // Longitud lógica del texto (sin contar gap)
} GapBuffer;

/** Inicializa el buffer con capacidad alineada a 4KB */
GapBuffer* gb_init(size_t initial_cap);

/** Inserta texto en la posición actual del cursor */
void gb_insert(GapBuffer *gb, const char *text, size_t len);

/** Elimina `count` caracteres hacia la izquierda del cursor */
void gb_delete(GapBuffer *gb, size_t count);

/** Mueve el cursor (y el gap) a la posición lógica `pos` */
void gb_move_cursor(GapBuffer *gb, size_t pos);

/** Extrae texto contiguo sin gap. Caller debe liberar con free(). */
bool gb_get_text(const GapBuffer *gb, char **out_buf, size_t *out_len);

/** Carga datos externos en el buffer (reset completo) */
bool gb_load_text(GapBuffer *gb, const char *data, size_t len);

/** Libera toda la memoria asociada */
void gb_destroy(GapBuffer *gb);

#endif /* GAP_BUFFER_H */