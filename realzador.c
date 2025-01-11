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
 * Abre la memoria compartida
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
 * Realza bordes en la mitad inferior [height/2..height).
 * Aplica kernel simple: 2*pixel - avg_vecinos
 */
static void apply_realce(SharedData* shared) {
    int width  = shared->header.width_px;
    int height = shared->header.height_px;
    int start  = height / 2;

    printf("[Realzador] Procesando filas %d..%d\n", start, height-1);
    for (int y = start+1; y < height-1; y++) {
        for (int x = 1; x < width-1; x++) {
            int sumB=0, sumG=0, sumR=0;
            for (int dy=-1; dy<=1; dy++) {
                for (int dx=-1; dx<=1; dx++) {
                    if (!(dy==0 && dx==0)) {
                        sumB += shared->pixels[y+dy][x+dx].blue;
                        sumG += shared->pixels[y+dy][x+dx].green;
                        sumR += shared->pixels[y+dy][x+dx].red;
                    }
                }
            }
            int avgB = sumB / 8;
            int avgG = sumG / 8;
            int avgR = sumR / 8;

            int eB = 2 * shared->pixels[y][x].blue  - avgB;
            int eG = 2 * shared->pixels[y][x].green - avgG;
            int eR = 2 * shared->pixels[y][x].red   - avgR;

            shared->pixels[y][x].blue  = (eB > 255) ? 255 : ((eB < 0) ? 0 : eB);
            shared->pixels[y][x].green = (eG > 255) ? 255 : ((eG < 0) ? 0 : eG);
            shared->pixels[y][x].red   = (eR > 255) ? 255 : ((eR < 0) ? 0 : eR);
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc > 1) {
        numThreads = atoi(argv[1]);
        if (numThreads < 1) numThreads = 1;
    }
    printf("[Realzador] Iniciando. Threads=%d\n", numThreads);

    SharedData* shared = map_shared_memory();
    if (!shared) {
        return EXIT_FAILURE;
    }

    sem_t* sem_image_ready  = sem_open(SEM_IMAGE_READY, 0);
    sem_t* sem_realzar_done = sem_open(SEM_REALZAR_DONE, 0);
    if (sem_image_ready == SEM_FAILED || sem_realzar_done == SEM_FAILED) {
        printError(FILE_ERROR);
        munmap(shared, sizeof(SharedData));
        return EXIT_FAILURE;
    }

    printf("[Realzador] Esperando imagen...\n");
    sem_wait(sem_image_ready);
    printf("[Realzador] Imagen recibida. Realzando...\n");

    apply_realce(shared);

    printf("[Realzador] Realce completado.\n");

    // Avisar que terminamos
    sem_post(sem_realzar_done);

    sem_close(sem_image_ready);
    sem_close(sem_realzar_done);
    munmap(shared, sizeof(SharedData));
    printf("[Realzador] Finalizado.\n");
    return EXIT_SUCCESS;
}