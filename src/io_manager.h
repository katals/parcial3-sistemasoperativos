#ifndef IO_MANAGER_H
#define IO_MANAGER_H

#include <stddef.h>
#include <stdbool.h>

/** Escribe datos comprimidos con header packed y buffers 4KB alineados */
bool io_write_compressed(const char *filename, 
                         const void *compressed_data, size_t compressed_len,
                         size_t original_len);

/** Lee y parsea header + payload comprimido */
bool io_read_compressed(const char *filename,
                        void **out_compressed, size_t *out_compressed_len,
                        size_t *out_original_len);

#endif /* IO_MANAGER_H */