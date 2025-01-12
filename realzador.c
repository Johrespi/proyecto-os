#include "common.h"
#include "bmp.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
#include <pthread.h>

// Estructura que define los limites de la imagen a procesar para cada hilo al aplicar el realce
typedef struct {
    SharedData* shared;
    int startY;
    int endY;
} RealceTask;

// Función que realza bordes con los límites (startY y endY) establecidos, desde la mitad de la imagen hacia abajo
static void realce_chunk(SharedData* shared, int startY, int endY) {
    int width  = shared->header.width_px;
    for (int y = startY; y < endY - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
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

            shared->pixels[y][x].blue  = (eB>255)?255:((eB<0)?0:eB);
            shared->pixels[y][x].green = (eG>255)?255:((eG<0)?0:eG);
            shared->pixels[y][x].red   = (eR>255)?255:((eR<0)?0:eR);
        }
    }
}

// Función que ejecutarán los hilos
static void* realce_thread(void* arg) {
    RealceTask* task = (RealceTask*)arg;
    realce_chunk(task->shared, task->startY, task->endY);
    return NULL;
}

// Función que abre la memoria compartida
static SharedData* map_shared_memory() {
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

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Uso: %s <numero_hilos>\n", argv[0]);
        return EXIT_FAILURE;
    }
    int numThreads = atoi(argv[1]);
    if (numThreads < 1) numThreads = 1;

    printf("[Realzador] Iniciando. Threads=%d\n", numThreads);
    SharedData* shared = map_shared_memory();
    if (!shared) {
        return EXIT_FAILURE;
    }

    sem_t* sem_realzar_ready = sem_open(SEM_REALZAR_READY, 0);
    sem_t* sem_realzar_done  = sem_open(SEM_REALZAR_DONE, 0);

    if (sem_realzar_ready == SEM_FAILED || sem_realzar_done == SEM_FAILED) {
        printError(FILE_ERROR);
        munmap(shared, sizeof(SharedData));
        return EXIT_FAILURE;
    }

    while (1) {
        printf("[Realzador] Esperando imagen...\n");
        sem_wait(sem_realzar_ready);
        printf("[Realzador] Imagen recibida. Realzando...\n");

        // Crear hilos para el procesamiento en paralelo
        pthread_t threads[numThreads];
        RealceTask tasks[numThreads];

        int height = shared->header.height_px;
        int start  = height / 2;  // Realce en mitad inferior (desde la mitad de la imagen hacia abajo)
        int lines  = height - start;
        int chunk  = lines / numThreads;

        // Asignar cada chunk a un hilo
        for (int i = 0; i < numThreads; i++) {
            tasks[i].shared = shared;
            tasks[i].startY = start + i * chunk;
            // Último hilo hasta el final
            tasks[i].endY   = (i == numThreads - 1) ? height: (start + (i+1)*chunk);
            pthread_create(&threads[i], NULL, realce_thread, &tasks[i]);
        }
        // Esperar a que terminen todos
        for (int i = 0; i < numThreads; i++) {
            pthread_join(threads[i], NULL);
        }

        printf("[Realzador] Realce completado.\n");

        // Avisar que ha terminado, DOS veces
        sem_post(sem_realzar_done);
        sem_post(sem_realzar_done);
    }

    sem_close(sem_realzar_ready);
    sem_close(sem_realzar_done);
    munmap(shared, sizeof(SharedData));
    printf("[Realzador] Finalizado.\n");
    return EXIT_SUCCESS;
}