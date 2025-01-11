#include "common.h"
#include "bmp.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>

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
                              MAP_SHARED, shm_fd, 0);
    if (shared == MAP_FAILED) {
        printError(MEMORY_ERROR);
        close(shm_fd);
        return NULL;
    }
    close(shm_fd);
    return shared;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Uso: %s <ruta_salida_final.bmp>\n", argv[0]);
        return EXIT_FAILURE;
    }
    char* outFile = argv[1];
    printf("[Combinador] Iniciando.\n");

    // Mapear memoria compartida
    SharedData* shared = map_shared_memory();
    if (!shared) {
        printf("[Combinador] Error al mapear memoria compartida.\n");
        return EXIT_FAILURE;
    }

    // Abrir semáforos de desenfoque y realce
    sem_t* sem_desenfocar_done = sem_open(SEM_DESENFOCAR_DONE, 0);
    sem_t* sem_realzar_done    = sem_open(SEM_REALZAR_DONE, 0);

    if (sem_desenfocar_done == SEM_FAILED || sem_realzar_done == SEM_FAILED) {
        printError(FILE_ERROR);
        munmap(shared, sizeof(SharedData));
        return EXIT_FAILURE;
    }

    while (1) {
        printf("[Combinador] Esperando que Desenfocador y Realzador completen...\n");

        // Debug print antes de esperar cada semáforo
        printf("[Combinador] Esperando señal del Desenfocador...\n");
        if (sem_wait(sem_desenfocar_done) == -1) {
            perror("[Combinador] Error al esperar SEM_DESENFOCAR_DONE");
            break;
        }
        printf("[Combinador] Señal del Desenfocador recibida.\n");

        printf("[Combinador] Esperando señal del Realzador...\n");
        if (sem_wait(sem_realzar_done) == -1) {
            perror("[Combinador] Error al esperar SEM_REALZAR_DONE");
            break;
        }
        printf("[Combinador] Señal del Realzador recibida.\n");

        printf("[Combinador] Desenfoque y Realce completados. Guardando la imagen final.\n");
        
        // Escribir la imagen final
        if (writeImage(outFile, shared) == -1) {
            printError(FILE_ERROR);
        } else {
            printf("[Combinador] Imagen final guardada en %s\n", outFile);
        }
    }

    // Cerrar semáforos
    sem_close(sem_desenfocar_done);
    sem_close(sem_realzar_done);

    // Desmapear memoria compartida
    munmap(shared, sizeof(SharedData));

    printf("[Combinador] Finalizado.\n");
    return EXIT_SUCCESS;
}