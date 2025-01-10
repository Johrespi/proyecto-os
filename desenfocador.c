#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>      // Para shm_open
#include <sys/mman.h>   // Para mmap
#include <semaphore.h>
#include <unistd.h>

/* Función para mapear la memoria compartida */
SharedData* map_shared_memory() {
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        printError(FILE_ERROR);
        return NULL;
    }

    SharedData* shared = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared == MAP_FAILED) {
        printError(MEMORY_ERROR);
        close(shm_fd);
        return NULL;
    }

    close(shm_fd);
    return shared;
}

/* Función simple de desenfoque (promedio de vecinos) */
void apply_blur(SharedData *shared) {
    for (int y = 1; y < shared->header.height_px -1; y++) {
        for (int x = 1; x < shared->header.width_px -1; x++) {
            int sum_red = 0, sum_green = 0, sum_blue = 0;
            for (int dy = -1; dy <=1; dy++) {
                for (int dx = -1; dx <=1; dx++) {
                    sum_red += shared->pixels[y + dy][x + dx].red;
                    sum_green += shared->pixels[y + dy][x + dx].green;
                    sum_blue += shared->pixels[y + dy][x + dx].blue;
                }
            }
            shared->pixelsOutDes[y][x].red = sum_red /9;
            shared->pixelsOutDes[y][x].green = sum_green /9;
            shared->pixelsOutDes[y][x].blue = sum_blue /9;
            shared->pixelsOutDes[y][x].alpha = shared->pixels[y][x].alpha;
        }
    }
}

int main() {
    // Mapear memoria compartida
    SharedData *shared = map_shared_memory();
    if (shared == NULL) {
        return EXIT_FAILURE;
    }

    // Abrir semáforos
    sem_t *sem_image_ready = sem_open(SEM_IMAGE_READY, 0);
    sem_t *sem_desenfocar_done = sem_open(SEM_DESENFOCAR_DONE, 0);

    if (sem_image_ready == SEM_FAILED || sem_desenfocar_done == SEM_FAILED) {
        printError(FILE_ERROR);
        if (sem_image_ready != SEM_FAILED) sem_close(sem_image_ready);
        if (sem_desenfocar_done != SEM_FAILED) sem_close(sem_desenfocar_done);
        munmap(shared, sizeof(SharedData));
        return EXIT_FAILURE;
    }

    // Esperar a que la imagen esté lista
    if (sem_wait(sem_image_ready) == -1) {
        printError(FILE_ERROR);
        // Cerrar semáforos y desmapear memoria
        sem_close(sem_image_ready);
        sem_close(sem_desenfocar_done);
        munmap(shared, sizeof(SharedData));
        return EXIT_FAILURE;
    }

    // Aplicar desenfoque
    apply_blur(shared);

    // Señalar que el desenfoque ha terminado
    if (sem_post(sem_desenfocar_done) == -1) {
        printError(FILE_ERROR);
        // Continuar
    }

    // Cerrar semáforos y desmapear memoria
    sem_close(sem_image_ready);
    sem_close(sem_desenfocar_done);
    munmap(shared, sizeof(SharedData));

    return EXIT_SUCCESS;
}