#include "io_manager.h"
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>

#define MAGIC 0x45444954  // "EDIT"

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint32_t version;
    uint64_t original_size;
    uint64_t compressed_size;
} FileHeader;

bool io_write_compressed(const char *filename, 
                         const void *compressed_data, size_t compressed_len,
                         size_t original_len) {
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) return false;

    FileHeader header = {
        .magic = MAGIC,
        .version = 1,
        .original_size = original_len,
        .compressed_size = compressed_len
    };

    // Escribir header + payload en una sola syscall (demostración)
    ssize_t written = write(fd, &header, sizeof(header));
    if (written != sizeof(header)) {
        close(fd);
        return false;
    }
    
    written = write(fd, compressed_data, compressed_len);
    close(fd);
    return written == (ssize_t)compressed_len;
}

bool io_read_compressed(const char *filename,
                        void **out_compressed, size_t *out_compressed_len,
                        size_t *out_original_len) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) return false;

    FileHeader header;
    ssize_t rd = read(fd, &header, sizeof(header));
    if (rd != sizeof(header) || header.magic != MAGIC) {
        close(fd);
        errno = EINVAL;
        return false;
    }

    *out_compressed = malloc(header.compressed_size);
    if (!*out_compressed) {
        close(fd);
        return false;
    }

    rd = read(fd, *out_compressed, header.compressed_size);
    close(fd);
    
    if (rd != (ssize_t)header.compressed_size) {
        free(*out_compressed);
        *out_compressed = NULL;
        return false;
    }

    *out_compressed_len = header.compressed_size;
    *out_original_len = header.original_size;
    return true;
}