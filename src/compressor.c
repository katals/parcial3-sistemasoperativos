#include "compressor.h"
#include <stdlib.h>
#include <string.h>

/*
 * Formato de compresión propio:
 * - Paquete literal: [len 0..127][len bytes]
 * - Paquete de repetición: [0x80 | len 1..127][byte repetido]
 *
 * El bit alto distingue ambos tipos. Esto mantiene el codec simple,
 * autocontenido y sin dependencias externas.
 */

static size_t worst_case_size(size_t input_len) {
    /* Cada bloque literal puede cargar hasta 127 bytes y añade 1 byte de header. */
    return input_len + ((input_len + 126) / 127) + 1;
}

bool compress_buffer(const void *in_buf, size_t in_len, 
                     void **out_buf, size_t *out_len) {
    if (!in_buf || !out_buf || !out_len) return false;

    if (in_len == 0) {
        *out_buf = NULL;
        *out_len = 0;
        return true;
    }

    const unsigned char *input = (const unsigned char *)in_buf;
    size_t capacity = worst_case_size(in_len);
    unsigned char *output = malloc(capacity);
    if (!output) return false;

    size_t read_pos = 0;
    size_t write_pos = 0;

    while (read_pos < in_len) {
        size_t run_len = 1;
        while (read_pos + run_len < in_len &&
               input[read_pos + run_len] == input[read_pos] &&
               run_len < 127) {
            run_len++;
        }

        if (run_len >= 4) {
            output[write_pos++] = (unsigned char)(0x80 | run_len);
            output[write_pos++] = input[read_pos];
            read_pos += run_len;
            continue;
        }

        size_t literal_start = read_pos;
        size_t literal_len = 0;

        while (read_pos < in_len) {
            run_len = 1;
            while (read_pos + run_len < in_len &&
                   input[read_pos + run_len] == input[read_pos] &&
                   run_len < 127) {
                run_len++;
            }

            if (run_len >= 4 || literal_len == 127) {
                break;
            }

            read_pos++;
            literal_len++;
        }

        if (literal_len == 0) {
            /* Caso borde: no entró en el literal porque había un run corto al final. */
            output[write_pos++] = 1;
            output[write_pos++] = input[read_pos++];
        } else {
            output[write_pos++] = (unsigned char)literal_len;
            memcpy(output + write_pos, input + literal_start, literal_len);
            write_pos += literal_len;
            if (read_pos < in_len) {
                /* read_pos quedó apuntando al inicio del siguiente token. */
            }
        }
    }

    *out_buf = output;
    *out_len = write_pos;
    return true;
}

bool decompress_buffer(const void *in_buf, size_t in_len,
                       char *out_buf, size_t out_len_expected) {
    if (!in_buf || !out_buf) return false;

    const unsigned char *input = (const unsigned char *)in_buf;
    size_t read_pos = 0;
    size_t write_pos = 0;

    while (read_pos < in_len) {
        unsigned char header = input[read_pos++];
        if (header & 0x80) {
            size_t run_len = (size_t)(header & 0x7f);
            if (run_len == 0 || read_pos >= in_len) return false;
            if (write_pos + run_len > out_len_expected) return false;

            unsigned char value = input[read_pos++];
            memset(out_buf + write_pos, value, run_len);
            write_pos += run_len;
        } else {
            size_t literal_len = (size_t)header;
            if (read_pos + literal_len > in_len) return false;
            if (write_pos + literal_len > out_len_expected) return false;

            memcpy(out_buf + write_pos, input + read_pos, literal_len);
            read_pos += literal_len;
            write_pos += literal_len;
        }
    }

    return write_pos == out_len_expected;
}