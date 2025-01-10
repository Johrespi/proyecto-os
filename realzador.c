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

/* Función simple de realce de bordes */
void apply_edge_enhance(SharedData *shared) {
    for (int y =1; y < shared->header.height_px -1; y++) {
        for (int x =1; x < shared->header.width_px -1; x++) {
            // Simple realce: new = 2*current - vecinos promedio
            int sum_red = 0, sum_green = 0, sum_blue = 0;
            for (int dy = -1; dy <=1; dy++) {
                for (int dx = -1; dx <=1; dx++) {
                    if (!(dy ==0 && dx ==0)) { // Excluir píxel central
                        sum_red += shared->pixels[y + dy][x + dx].red;
                        sum_green += shared->pixels[y + dy][x + dx].green;
                        sum_blue += shared->pixels[y + dy][x + dx].blue;
                    }
                }
            }
            int avg_red = sum_red /8;
            int avg_green = sum_green /8;
            int avg_blue = sum_blue /8;

            int enhanced_red = 2 * shared->pixels[y][x].red - avg_red;
            int enhanced_green = 2 * shared->pixels[y][x].green - avg_green;
            int enhanced_blue = 2 * shared->pixels[y][x].blue - avg_blue;

            // Limitar a 0-255
            shared->pixelsOutReal[y][x].red = (enhanced_red >255) ? 255 : ((enhanced_red <0) ? 0 : enhanced_red);
            shared->pixelsOutReal[y][x].green = (enhanced_green >255) ? 255 : ((enhanced_green <0) ? 0 : enhanced_green);
            shared->pixelsOutReal[y][x].blue = (enhanced_blue >255) ? 255 : ((enhanced_blue <0) ? 0 : enhanced_blue);
            shared->pixelsOutReal[y][x].alpha = shared->pixels[y][x].alpha;
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
    sem_t *sem_realzar_done = sem_open(SEM_REALZAR_DONE, 0);

    if (sem_image_ready == SEM_FAILED || sem_realzar_done == SEM_FAILED) {
        printError(FILE_ERROR);
        if (sem_image_ready != SEM_FAILED) sem_close(sem_image_ready);
        if (sem_realzar_done != SEM_FAILED) sem_close(sem_realzar_done);
        munmap(shared, sizeof(SharedData));
        return EXIT_FAILURE;
    }

    // Esperar a que la imagen esté lista
    if (sem_wait(sem_image_ready) == -1) {
        printError(FILE_ERROR);
        // Cerrar semáforos y desmapear memoria
        sem_close(sem_image_ready);
        sem_close(sem_realzar_done);
        munmap(shared, sizeof(SharedData));
        return EXIT_FAILURE;
    }

    // Aplicar realce
    apply_edge_enhance(shared);

    // Señalar que el realce ha terminado
    if (sem_post(sem_realzar_done) == -1) {
        printError(FILE_ERROR);
        // Continuar
    }

    // Cerrar semáforos y desmapear memoria
    sem_close(sem_image_ready);
    sem_close(sem_realzar_done);
    munmap(shared, sizeof(SharedData));

    return EXIT_SUCCESS;
}