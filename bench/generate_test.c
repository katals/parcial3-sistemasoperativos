#include "../src/io_manager.h"
#include "../src/compressor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ORIGINAL_SIZE (50 * 1024 * 1024)  // 50 MB

int main() {
    char *text = malloc(ORIGINAL_SIZE);
    if (!text) {
        fprintf(stderr, "Error: OOM al alojar memoria para test.\n");
        return 1;
    }

    // Llenar el buffer con un patrón altamente compresible (RLE-friendly)
    // Usamos rachas largas del mismo carácter intercaladas con algunos saltos de línea
    for (size_t i = 0; i < ORIGINAL_SIZE; i++) {
        if (i % 120 < 110) {
            text[i] = 'A';
        } else {
            text[i] = 'B';
        }
    }
    
    void *compressed = NULL;
    size_t compressed_len = 0;
    if (!compress_buffer(text, ORIGINAL_SIZE, &compressed, &compressed_len)) {
        fprintf(stderr, "Error al comprimir el buffer.\n");
        free(text);
        return 1;
    }
    
    if (!io_write_compressed("bench/test_input.bin", compressed, compressed_len, ORIGINAL_SIZE)) {
        fprintf(stderr, "Error al escribir el archivo comprimido.\n");
        free(text);
        free(compressed);
        return 1;
    }
    
    printf("=================================================\n");
    printf("✅ Archivo de test RLE generado con éxito\n");
    printf("   Ruta: bench/test_input.bin\n");
    printf("   Tamaño original: %d bytes\n", ORIGINAL_SIZE);
    printf("   Tamaño en disco (RLE): %zu bytes\n", compressed_len);
    printf("=================================================\n");
    
    free(text);
    free(compressed);
    return 0;
}