#include "common.h"
#include "bmp.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>

static int numThreads = 1;

/*
 * Abre la memoria compartida para leer la imagen.
 */
static SharedData* map_shared_memory() {
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        printError(FILE_ERROR);
        return NULL;
    }
    SharedData* shared = mmap(NULL, sizeof(SharedData),
                              PROT_READ | PROT_WRITE,
                              MAP_SHARED,
                              shm_fd,
                              0);
    if (shared == MAP_FAILED) {
        printError(MEMORY_ERROR);
        close(shm_fd);
        return NULL;
    }
    close(shm_fd);
    return shared;
}

/*
 * Desenfoca la mitad superior (filas 0..height/2)
 * Kernel 3x3 simple.
 */
static void apply_blur(SharedData* shared) {
    int width  = shared->header.width_px;
    int height = shared->header.height_px;
    int half   = height / 2;

    printf("[Desenfocador] Procesando filas 0..%d\n", half - 1);
    for (int y = 1; y < half - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            int sumB=0, sumG=0, sumR=0;
            for (int dy=-1; dy<=1; dy++) {
                for (int dx=-1; dx<=1; dx++) {
                    sumB += shared->pixels[y+dy][x+dx].blue;
                    sumG += shared->pixels[y+dy][x+dx].green;
                    sumR += shared->pixels[y+dy][x+dx].red;
                }
            }
            shared->pixels[y][x].blue  = sumB/9;
            shared->pixels[y][x].green = sumG/9;
            shared->pixels[y][x].red   = sumR/9;
            // alpha no se modifica
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Uso: %s <salida_desenfocado.bmp> [num_hilos]\n", argv[0]);
        return EXIT_FAILURE;
    }
    char* outFile = argv[1];
    if (argc > 2) {
        numThreads = atoi(argv[2]);
        if (numThreads < 1) numThreads = 1;
    }

    printf("[Desenfocador] Iniciando. Threads=%d\n", numThreads);
    SharedData* shared = map_shared_memory();
    if (!shared) return EXIT_FAILURE;

    // Abrir semáforos
    sem_t* sem_image_ready     = sem_open(SEM_IMAGE_READY, 0);
    sem_t* sem_desenfocar_done = sem_open(SEM_DESENFOCAR_DONE, 0);

    if (sem_image_ready == SEM_FAILED || sem_desenfocar_done == SEM_FAILED) {
        printError(FILE_ERROR);
        munmap(shared, sizeof(SharedData));
        return EXIT_FAILURE;
    }

    printf("[Desenfocador] Esperando imagen...\n");
    sem_wait(sem_image_ready);
    printf("[Desenfocador] Imagen recibida. Desenfocando...\n");

    apply_blur(shared);

    printf("[Desenfocador] Desenfoque completado.\n");
    if (writeImage(outFile, shared) == -1) {
        printError(FILE_ERROR);
    } else {
        printf("[Desenfocador] Guardado intermedio en %s\n", outFile);
    }

    // Avisar que ha terminado
    sem_post(sem_desenfocar_done);

    // Limpieza
    sem_close(sem_image_ready);
    sem_close(sem_desenfocar_done);
    munmap(shared, sizeof(SharedData));
    printf("[Desenfocador] Finalizado.\n");
    return EXIT_SUCCESS;
}