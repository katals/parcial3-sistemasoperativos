#ifndef COMPRESSOR_H
#define COMPRESSOR_H

#include <stddef.h>
#include <stdbool.h>

/** Comprime buffer en User Space. Caller libera `out_buf`. */
bool compress_buffer(const void *in_buf, size_t in_len, 
                     void **out_buf, size_t *out_len);

/** Descomprime buffer en User Space. Caller provee `out_buf` con tamaño exacto. */
bool decompress_buffer(const void *in_buf, size_t in_len,
                       char *out_buf, size_t out_len_expected);

#endif /* COMPRESSOR_H */