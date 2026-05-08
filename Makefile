# Compilador y estándar
CC      = gcc
CSTD    = -std=c11
POSIX   = -D_GNU_SOURCE

# Flags de compilación (Rúbrica: rigor técnico + debug)
CFLAGS  = $(CSTD) $(POSIX) -Wall -Wextra -O2 -g \
          -fno-omit-frame-pointer \
          -MMD -MP

# Enlazado y librerías
LDFLAGS =

# Directorios y fuentes
SRC_DIR = src
SRCS    = $(SRC_DIR)/main.c \
          $(SRC_DIR)/terminal.c \
          $(SRC_DIR)/editor.c \
          $(SRC_DIR)/gap_buffer.c \
          $(SRC_DIR)/compressor.c \
          $(SRC_DIR)/io_manager.c

OBJS    = $(SRCS:.c=.o)
DEPS    = $(OBJS:.o=.d)
TARGET  = editor

# Targets públicos
.PHONY: all clean bench valgrind format

all: $(TARGET)

# Enlazado final
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compilación por módulo (genera dependencias automáticas)
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Limpieza de artefactos
clean:
	rm -f $(OBJS) $(DEPS) $(TARGET)
	rm -rf *.dSYM core.* vgcore.*

# Benchmark integral (eje 4 de la rúbrica)
bench: $(TARGET)
	@echo "🔍 Ejecutando profiling empírico..."
	@bash bench/profile.sh

# Validación de memoria (eje 2)
valgrind: $(TARGET)
	valgrind --leak-check=full \
	         --track-origins=yes \
	         --show-leak-kinds=all \
	         --errors-for-leak-kinds=all \
	         ./editor test_input.txt

# Dependencias automáticas
-include $(DEPS)