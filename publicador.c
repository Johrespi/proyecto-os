#include "common.h"
#include "bmp.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <string.h>
#include <unistd.h>

/*
 * Crea o abre la memoria compartida para colocar la imagen.
 */
static SharedData* map_shared_memory() {
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        printError(FILE_ERROR);
        return NULL;
    }
    if (ftruncate(shm_fd, sizeof(SharedData)) == -1) {
        printError(MEMORY_ERROR);
        close(shm_fd);
        return NULL;
    }
    SharedData* shared = mmap(NULL, sizeof(SharedData),
                              PROT_READ | PROT_WRITE,
                              MAP_SHARED,
                              shm_fd, 0);
    if (shared == MAP_FAILED) {
        printError(MEMORY_ERROR);
        close(shm_fd);
        return NULL;
    }
    close(shm_fd);
    return shared;
}

int main() {
    printf("[Publicador] Iniciando.\n");

    // Crear memoria compartida
    SharedData* shared = map_shared_memory();
    if (!shared) return EXIT_FAILURE;

    // Crear/abrir semáforos
    sem_t* sem_image_ready     = sem_open(SEM_IMAGE_READY, O_CREAT, 0666, 0);
    sem_t* sem_desenfocar_done = sem_open(SEM_DESENFOCAR_DONE, O_CREAT, 0666, 0);
    sem_t* sem_realzar_done    = sem_open(SEM_REALZAR_DONE, O_CREAT, 0666, 0);

    if (sem_image_ready == SEM_FAILED ||
        sem_desenfocar_done == SEM_FAILED ||
        sem_realzar_done == SEM_FAILED) {
        printError(FILE_ERROR);
        if (sem_image_ready     != SEM_FAILED) sem_close(sem_image_ready);
        if (sem_desenfocar_done!= SEM_FAILED) sem_close(sem_desenfocar_done);
        if (sem_realzar_done   != SEM_FAILED) sem_close(sem_realzar_done);
        munmap(shared, sizeof(SharedData));
        return EXIT_FAILURE;
    }

    while (1) {
        printf("[Publicador] Ingrese ruta BMP (o 'exit' para terminar): ");
        char path[256];
        if (!fgets(path, sizeof(path), stdin)) {
            break;
        }
        path[strcspn(path, "\n")] = 0; // Quitar salto de línea
        if (strcmp(path, "exit") == 0) {
            printf("[Publicador] Saliendo.\n");
            break;
        }

        FILE *f = fopen(path, "rb");
        if (!f) {
            printError(FILE_ERROR);
            continue;
        }
        printf("[Publicador] Leyendo %s...\n", path);
        if (readImage(f, shared) == -1) {
            fclose(f);
            continue;
        }
        fclose(f);
        printf("[Publicador] Imagen cargada.\n");

        // Avisar que la imagen está lista
        sem_post(sem_image_ready);

        // Esperar a desenfocador y realzador
        sem_wait(sem_desenfocar_done);
        sem_wait(sem_realzar_done);
        printf("[Publicador] Desenfoque y realce completados.\n");
    }

    // Cerrar semáforos
    sem_close(sem_image_ready);
    sem_close(sem_desenfocar_done);
    sem_close(sem_realzar_done);

    // Liberar memoria
    munmap(shared, sizeof(SharedData));

    // Eliminar recursos al finalizar
    shm_unlink(SHM_NAME);
    sem_unlink(SEM_IMAGE_READY);
    sem_unlink(SEM_DESENFOCAR_DONE);
    sem_unlink(SEM_REALZAR_DONE);

    printf("[Publicador] Finalizado.\n");
    return EXIT_SUCCESS;
}