**Matriz de Diseño del Pipeline I/O**
<img width="1360" height="1160" alt="pipeline_io_matriz" src="https://github.com/user-attachments/assets/dccdb303-88e1-4fe8-afce-9cdda965b642" />

**Gestión de Memoria en C:**
Para evitar el desperdicio de memoria por alineamiento de hardware (padding) al volcar estructuras a disco, empleamos el atributo __attribute__((packed)) de GCC en la estructura FileHeader. Esto garantiza que la metadata binaria ocupe de forma contigua exactamente sus 24 bytes requeridos, sin fragmentación ni espacios vacíos en la memoria RAM o en el archivo de salida.

Asimismo, para prevenir cualquier fuga de memoria (memory leak), la gestión de los recursos dinámicos está centralizada en el ciclo de vida del Editor y el Gap Buffer. Mediante la asignación de bloques de memoria alineados a páginas (posix_memalign), garantizamos que todas las alocaciones se liberen a través de una estricta secuencia de cierre. Esta rutina de liberación se ejecuta incluso al capturar señales de interrupción del TTY, garantizando una salida segura. La integridad de este manejo de memoria ha sido rigurosamente validada mediante pruebas continuas con la directiva make valgrind configurada en el Makefile.

**Manejo de Texto Enriquecido**
Para aislar el contenido de la metadata, el editor no guarda texto plano. En su lugar, genera un archivo binario personalizado cuyo formato consta de:

Header (24 Bytes): Usa una cabecera estricta que precede y da contexto a la lectura:
Magic Number (4 bytes, 0x45444954 / "EDIT") para identificar y prevenir la apertura de archivos corruptos.
Version (4 bytes).
Tamaño Original (8 bytes).
Tamaño Comprimido (8 bytes).
Payload Comprimido: Justo tras el byte 24 comienza el flujo de datos codificado con nuestro formato RLE (usando detección de bit alto - high bit - para distinguir literales de secuencias). Este diseño sienta la base estandarizada, de modo que ampliar a texto enriquecido es tan simple como inyectar tablas de estilos antes o dentro de la cabecera actual.

**Reporte de Profiling (Evidencia de Ingeniería)**

| Métrica | Enfoque Clásico (Fread/Fwrite por byte) | Enfoque Propuesto (Compresión + Write Bloques/POSIX) | Impacto / Rentabilidad |
| :--- | :--- | :--- | :--- |
| **Volumen de Datos a Disco** | 50.0 MB | 1.7 MB (1706 KB) | -97% (Liberación drástica del bus I/O) |
| **Llamadas a `write()`** | ~50,000,000 llamadas (1 por char) o miles | 2 llamadas (Bloques gigantes mapeados) | -99% (Reducción total de Context Switches) |
| **Llamadas al sistema Totales** | Más de 100,000 interrupciones estimadas | 28 llamadas totales | Demuestra máxima eficiencia interactiva. |
| **Tiempo de CPU (User Mode)** | ~0.005 s | 0.103 s (Ciclos invertidos en comprimir RLE en C) | *Aumento esperado* - Transferimos carga I/O a CPU. |
| **Tiempo de SO (Sys Mode)** | ~4.5 s a varios seg | 0.209 s | -90% aprox (El kernel descansa) |

<img width="1115" height="966" alt="image" src="https://github.com/user-attachments/assets/f6cc7742-1772-4d84-9af7-1372c0c0371d" />


# Proyecto 3: Editor de Archivos con Optimización de Bus I/O en C

Este es un editor de texto nativo para entornos Linux/POSIX escrito en C. Su objetivo principal es demostrar la manipulación directa de memoria usando "Gap Buffers" y la optimización extrema del bus I/O del sistema operativo mediante compresión en User Space (RLE) y syscalls eficientes (buffers alineados a tamaño de página de 4KB).

##  Requisitos del Sistema

Para compilar y ejecutar este proyecto correctamente, necesitas un entorno basado en Linux (o WSL en Windows) con las herramientas básicas de compilación y medición.

Instala las dependencias en distribuciones basadas en Ubuntu/Debian con el siguiente comando:

```bash
sudo apt-get update
sudo apt-get install -y build-essential strace valgrind
```

- `build-essential`: Incluye el compilador `gcc` y herramientas como `make`.
- `strace`: Para el perfilado de syscalls y medir reducciones en los "Context Switches".
- `valgrind`: Para el análisis y prevención de fugas de memoria.

##  Compilación

El proyecto utiliza `make` para su orquestación. Todas las reglas de compilación están definidas para usar el estándar `C11` y la bandera `_GNU_SOURCE`.

Para compilar el proyecto completo, sitúate en la raíz del proyecto y ejecuta:

```bash
make
```

Esto generará el ejecutable binario `./editor` y compilará los módulos ubicados en `src/`.

Para limpiar el proyecto (borrar los `.o`, archivos `.d` y los binarios generados), corre:

```bash
make clean
```

## Ejecución del Editor

Para abrir un archivo existente o crear uno nuevo, pasa el nombre del archivo como argumento:

```bash
./editor mi_archivo.txt
```

### Controles Básicos de la Interfaz:
- **Teclas alfanuméricas**: Escribir texto.
- **Flechas**: Mover el cursor.
- **Backspace**: Borrar texto.
- **`:` (Dos puntos)**: Entrar en modo de comandos.
- **`:wq` + ENTER**: Guardar archivo (activa la compresión y guardado empaquetado) y salir del editor.

##  Medición y Profiling (Pruebas de la Rúbrica)

El proyecto incluye scripts automatizados para demostrar la eficiencia del bus I/O y la reducción de llamadas al sistema (Eje 4 de la evaluación).

### 1. Benchmark Empírico (strace + time)
Este comando genera un archivo comprimido de 50MB internamente, ejecuta el editor en modo `bench` aislado (sin requerir I/O humano) y levanta métricas de rendimiento reales:

```bash
make bench
```

**Salida esperada:**
- Cuadro de llamadas a nivel del Kernel validando la escasa cantidad de llamadas a `write()` / `read()`.
- Resultados de `time` mostrando el incremento de CPU `User` (por compresión) con decaída sustancial en modo `Sys`.
- Relación de compresión matemática confirmando liberación de uso en disco físico del más del 90%.

### 2. Validación de Memoria (Valgrind)
Para constatar que la arquitectura en memoria no presenta goteos (memory leaks) ni violaciones de punteros, puedes usar el target especializado que corre el editor bajo las lupas destructivas de `valgrind`:

```bash
make valgrind
```

## Arquitectura y Estructura

- `src/gap_buffer.c`: Gestión de secuencias de texto con estructura Gap (usando alineaciones `posix_memalign`).
- `src/io_manager.c`: Lectura/Escritura mediante cabeceras binarias estrictas (`__attribute__((packed))`) y llamadas asincrónicas limpias POSIX limitando el uso trivial de la suite ineficiente de `stdio.h`.
- `src/compressor.c`: Codec RLE hecho desde cero. Detecta rachas de texto para salvar espacio en disco antes del flujo I/O.
- `bench/`: Código subyacente para generación sintética de tests intensos de uso de memoria y rendimiento en tiempo real. 

