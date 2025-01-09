#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include "common.h"

// Estructura de argumentos para los hilos
typedef struct {
    SharedData *shared;
    int startRow;
    int endRow;
    int filter[3][3];
} ThreadArgs;

// Función del hilo para aplicar el filtro de realce de bordes
void *filterThreadWorkerReal(void *args) {
    ThreadArgs *threadArgs = (ThreadArgs *)args;
    SharedData *shared = threadArgs->shared;
    int (*filter)[3] = threadArgs->filter;

    for (int y = threadArgs->startRow; y < threadArgs->endRow; y++) {
        for (int x = 0; x < shared->header.width_px; x++) {
            int red = 0, green = 0, blue = 0;

            for (int fy = -1; fy <= 1; fy++) {
                for (int fx = -1; fx <= 1; fx++) {
                    int ny = y + fy;
                    int nx = x + fx;

                    if (ny < 0) ny = 0;
                    if (ny >= shared->header.height_px) ny = shared->header.height_px - 1;
                    if (nx < 0) nx = 0;
                    if (nx >= shared->header.width_px) nx = shared->header.width_px - 1;

                    Pixel neighbor = shared->pixels[ny][nx];
                    red += neighbor.red * filter[fy + 1][fx + 1];
                    green += neighbor.green * filter[fy + 1][fx + 1];
                    blue += neighbor.blue * filter[fy + 1][fx + 1];
                }
            }

            // Asegurarse de que los valores de color estén dentro del rango [0, 255]
            red = (red > 255) ? 255 : ((red < 0) ? 0 : red);
            green = (green > 255) ? 255 : ((green < 0) ? 0 : green);
            blue = (blue > 255) ? 255 : ((blue < 0) ? 0 : blue);

            shared->pixelsOutReal[y][x].red = red;
            shared->pixelsOutReal[y][x].green = green;
            shared->pixelsOutReal[y][x].blue = blue;
            shared->pixelsOutReal[y][x].alpha = shared->pixels[y][x].alpha;
        }
    }

    return NULL;
}

int main(int argc, char **argv) {
    // Abrir memoria compartida
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    SharedData *shared = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // Abrir semáforos
    sem_t *sem_image_ready = sem_open(SEM_IMAGE_READY, 0);
    sem_t *sem_realzar_done = sem_open(SEM_REALZAR_DONE, 0);

    if (sem_image_ready == SEM_FAILED || sem_realzar_done == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    // Definir el filtro de realce de bordes (edge detection)
    int edgeFilter[3][3] = {
        { -1, -1, -1 },
        { -1,  8, -1 },
        { -1, -1, -1 }
    };

    // Configurar número de hilos (puede ser parametrizable)
    int numThreads = 4;

    while (1) {
        // Esperar a que una nueva imagen esté disponible
        sem_wait(sem_image_ready);

        printf("Realzador: Procesando imagen...\n");

        // Definir las filas a procesar (segunda mitad)
        int halfHeight = shared->header.height_px / 2;
        int rowsPerThread = (shared->header.height_px - halfHeight) / numThreads;

        pthread_t threads[numThreads];
        ThreadArgs threadArgs[numThreads];

        for (int i = 0; i < numThreads; i++) {
            threadArgs[i].shared = shared;
            threadArgs[i].startRow = halfHeight + i * rowsPerThread;
            threadArgs[i].endRow = (i == numThreads - 1) ? shared->header.height_px : halfHeight + (i + 1) * rowsPerThread;
            memcpy(threadArgs[i].filter, edgeFilter, sizeof(edgeFilter));

            if (pthread_create(&threads[i], NULL, filterThreadWorkerReal, &threadArgs[i]) != 0) {
                perror("pthread_create");
                exit(EXIT_FAILURE);
            }
        }

        // Unir hilos
        for (int i = 0; i < numThreads; i++) {
            pthread_join(threads[i], NULL);
        }

        printf("Realzador: Procesamiento completado.\n");

        // Señalar que el realce ha finalizado
        sem_post(sem_realzar_done);
    }

    // Cerrar semáforos y memoria compartida
    sem_close(sem_image_ready);
    sem_close(sem_realzar_done);
    munmap(shared, SHM_SIZE);
    close(shm_fd);

    return 0;
}