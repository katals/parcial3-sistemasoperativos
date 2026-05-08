#include "gap_buffer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#define PAGE_SIZE 4096

/* Alinea tamaño hacia arriba al múltiplo de PAGE_SIZE más cercano */
static inline size_t align_page(size_t sz) {
    return (sz + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
}

/* Crece el buffer manteniendo alineación y posición del gap */
static bool gb_grow(GapBuffer *gb, size_t required) {
    size_t new_cap = align_page(gb->capacity * 2);
    if (new_cap < required) new_cap = align_page(required + PAGE_SIZE);

    char *new_data = NULL;
    int ret = posix_memalign((void **)&new_data, PAGE_SIZE, new_cap);
    if (ret != 0 || !new_data) {
        fprintf(stderr, "[gap_buffer] posix_memalign failed: %s\n", strerror(errno));
        return false;
    }

    size_t after_len = gb->capacity - gb->gap_end;

    /* Copia segmento izquierdo */
    memcpy(new_data, gb->data, gb->gap_start);
    /* Copia segmento derecho al final del nuevo bloque para preservar gap */
    if (after_len > 0)
        memcpy(new_data + (new_cap - after_len), gb->data + gb->gap_end, after_len);

    free(gb->data);
    gb->data = new_data;
    gb->capacity = new_cap;
    gb->gap_end = new_cap - after_len;
    return true;
}

GapBuffer* gb_init(size_t initial_cap) {
    GapBuffer *gb = calloc(1, sizeof(GapBuffer));
    if (!gb) return NULL;

    size_t cap = align_page(initial_cap ? initial_cap : PAGE_SIZE);
    int ret = posix_memalign((void **)&gb->data, PAGE_SIZE, cap);
    if (ret != 0 || !gb->data) {
        free(gb);
        return NULL;
    }

    gb->capacity = cap;
    gb->gap_start = 0;
    gb->gap_end   = cap; /* Gap ocupa todo el buffer inicialmente */
    gb->length    = 0;
    return gb;
}

void gb_insert(GapBuffer *gb, const char *text, size_t len) {
    if (!gb || !text || len == 0) return;

    size_t gap_size = gb->gap_end - gb->gap_start;
    if (len > gap_size) {
        if (!gb_grow(gb, gb->length + len)) return;
    }

    memcpy(gb->data + gb->gap_start, text, len);
    gb->gap_start += len;
    gb->length    += len;
}

void gb_delete(GapBuffer *gb, size_t count) {
    if (!gb || count == 0) return;
    if (count > gb->length) count = gb->length;

    /* Reduce gap hacia la izquierda */
    gb->gap_start -= count;
    gb->length    -= count;
}

void gb_move_cursor(GapBuffer *gb, size_t pos) {
    if (!gb || pos == gb->gap_start) return;
    if (pos > gb->length) pos = gb->length;

    size_t gap_size = gb->gap_end - gb->gap_start;

    if (pos < gb->gap_start) {
        /* Mover gap a la izquierda: texto [pos, gap_start) -> [pos+gap_size, gap_end) */
        size_t move_len = gb->gap_start - pos;
        memmove(gb->data + pos + gap_size, gb->data + pos, move_len);
        gb->gap_start = pos;
        gb->gap_end   = pos + gap_size;
    } else {
        /* Mover gap a la derecha: texto [gap_end, pos) -> [gap_start, gap_start+(pos-gap_start)) */
        size_t move_len = pos - gb->gap_start;
        memmove(gb->data + gb->gap_start, gb->data + gb->gap_end, move_len);
        gb->gap_start = pos;
        gb->gap_end   = pos + gap_size;
    }
}

bool gb_get_text(const GapBuffer *gb, char **out_buf, size_t *out_len) {
    if (!gb || !out_buf || !out_len) return false;

    size_t left_len  = gb->gap_start;
    size_t right_len = gb->capacity - gb->gap_end;
    size_t total     = left_len + right_len;

    char *buf = malloc(total + 1); /* +1 para seguridad, aunque no usamos \0 */
    if (!buf) return false;

    memcpy(buf, gb->data, left_len);
    memcpy(buf + left_len, gb->data + gb->gap_end, right_len);

    *out_buf = buf;
    *out_len = total;
    return true;
}

bool gb_load_text(GapBuffer *gb, const char *data, size_t len) {
    if (!gb) return false;

    /* Reset lógico */
    gb->length    = 0;
    gb->gap_start = 0;

    if (len > gb->capacity) {
        /* Liberar actual y realinear desde cero */
        free(gb->data);
        size_t new_cap = align_page(len + PAGE_SIZE);
        if (posix_memalign((void **)&gb->data, PAGE_SIZE, new_cap) != 0) {
            gb->data = NULL;
            gb->capacity = 0;
            return false;
        }
        gb->capacity = new_cap;
    }

    if (data && len > 0)
        memcpy(gb->data, data, len);

    gb->gap_start = len;
    gb->gap_end   = gb->capacity;
    gb->length    = len;
    return true;
}

void gb_destroy(GapBuffer *gb) {
    if (!gb) return;
    free(gb->data);
    free(gb);
}