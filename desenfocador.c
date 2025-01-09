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
    BMP_Image *imageIn;
    BMP_Image *imageOut;
    int startRow;
    int endRow;
    int filter[3][3];
    int normalizationFactor;
} ThreadArgs;

// Función del hilo para aplicar el filtro de desenfoque
void *filterThreadWorker(void *args) {
    ThreadArgs *threadArgs = (ThreadArgs *)args;
    BMP_Image *imageIn = threadArgs->imageIn;
    BMP_Image *imageOut = threadArgs->imageOut;
    int (*filter)[3] = threadArgs->filter;
    int norm = threadArgs->normalizationFactor;

    for (int y = threadArgs->startRow; y < threadArgs->endRow; y++) {
        for (int x = 0; x < imageIn->header.width_px; x++) {
            int red = 0, green = 0, blue = 0;

            for (int fy = -1; fy <=1; fy++) {
                for (int fx = -1; fx <=1; fx++) {
                    int ny = y + fy;
                    int nx = x + fx;

                    if (ny < 0) ny = 0;
                    if (ny >= imageIn->norm_height) ny = imageIn->norm_height -1;
                    if (nx < 0) nx = 0;
                    if (nx >= imageIn->header.width_px) nx = imageIn->header.width_px -1;

                    Pixel neighbor = imageIn->pixels[ny][nx];
                    red += neighbor.red * filter[fy+1][fx+1];
                    green += neighbor.green * filter[fy+1][fx+1];
                    blue += neighbor.blue * filter[fy+1][fx+1];
                }
            }

            imageOut->pixels[y][x].red = red / norm;
            imageOut->pixels[y][x].green = green / norm;
            imageOut->pixels[y][x].blue = blue / norm;
            imageOut->pixels[y][x].alpha = imageIn->pixels[y][x].alpha;
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
    sem_t *sem_desenfocar_done = sem_open(SEM_DESENFOCAR_DONE, 0);

    if (sem_image_ready == SEM_FAILED || sem_desenfocar_done == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    // Definir el filtro de desenfoque (box filter)
    int boxFilter[3][3] = {
        {1, 1, 1},
        {1, 1, 1},
        {1, 1, 1}
    };
    int normalizationFactor = 9;

    // Configurar número de hilos (puede ser parametrizable)
    int numThreads = 4;

    while (1) {
        // Esperar a que una nueva imagen esté disponible
        sem_wait(sem_image_ready);

        printf("Desenfocador: Procesando imagen...\n");

        // Crear una imagen de salida para la primera mitad
        BMP_Image *imageOutDes = duplicateBMPImageHeader(&shared->image);
        if (imageOutDes == NULL) {
            printf("Desenfocador: Error al crear la imagen de salida.\n");
            continue;
        }

        // Definir las filas a procesar (primera mitad)
        int halfHeight = shared->image.norm_height / 2;
        int rowsPerThread = halfHeight / numThreads;

        pthread_t threads[numThreads];
        ThreadArgs threadArgs[numThreads];

        for (int i = 0; i < numThreads; i++) {
            threadArgs[i].imageIn = &shared->image;
            threadArgs[i].imageOut = imageOutDes;
            threadArgs[i].startRow = i * rowsPerThread;
            threadArgs[i].endRow = (i == numThreads -1) ? halfHeight : (i +1) * rowsPerThread;
            memcpy(threadArgs[i].filter, boxFilter, sizeof(boxFilter));
            threadArgs[i].normalizationFactor = normalizationFactor;

            if (pthread_create(&threads[i], NULL, filterThreadWorker, &threadArgs[i]) != 0) {
                perror("pthread_create");
                exit(EXIT_FAILURE);
            }
        }

        // Unir hilos
        for (int i = 0; i < numThreads; i++) {
            pthread_join(threads[i], NULL);
        }

        // Guardar la imagen desenfocada en la memoria compartida
        shared->imageOutDes = *imageOutDes;
        freeImage(imageOutDes); // Liberar la memoria temporal

        printf("Desenfocador: Procesamiento completado.\n");

        // Señalar que el desenfoque ha finalizado
        sem_post(sem_desenfocar_done);
    }

    // Cerrar semáforos y memoria compartida
    sem_close(sem_image_ready);
    sem_close(sem_desenfocar_done);
    munmap(shared, SHM_SIZE);
    close(shm_fd);

    return 0;
}