#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>      // Para shm_open
#include <sys/mman.h>   // Para mmap
#include <semaphore.h>
#include <string.h>
#include <unistd.h>

/* Función para mapear la memoria compartida */
SharedData* map_shared_memory() {
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

    SharedData* shared = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared == MAP_FAILED) {
        printError(MEMORY_ERROR);
        close(shm_fd);
        return NULL;
    }

    close(shm_fd);
    return shared;
}

int main() {
    // Mapear memoria compartida
    SharedData *shared = map_shared_memory();
    if (shared == NULL) {
        return EXIT_FAILURE;
    }

    // Inicializar semáforos
    sem_t *sem_image_ready = sem_open(SEM_IMAGE_READY, O_CREAT, 0666, 0);
    sem_t *sem_desenfocar_done = sem_open(SEM_DESENFOCAR_DONE, O_CREAT, 0666, 0);
    sem_t *sem_realzar_done = sem_open(SEM_REALZAR_DONE, O_CREAT, 0666, 0);

    if (sem_image_ready == SEM_FAILED || sem_desenfocar_done == SEM_FAILED || sem_realzar_done == SEM_FAILED) {
        printError(FILE_ERROR);
        // Cerrar semáforos que se hayan abierto correctamente
        if (sem_image_ready != SEM_FAILED) sem_close(sem_image_ready);
        if (sem_desenfocar_done != SEM_FAILED) sem_close(sem_desenfocar_done);
        if (sem_realzar_done != SEM_FAILED) sem_close(sem_realzar_done);
        munmap(shared, sizeof(SharedData));
        return EXIT_FAILURE;
    }

    // Abrir imagen de entrada
    FILE *source = fopen("/testcases/test.bmp", "rb");
    if (!source) {
        printError(FILE_ERROR);
        // Cerrar semáforos y desmapear memoria compartida
        sem_close(sem_image_ready);
        sem_close(sem_desenfocar_done);
        sem_close(sem_realzar_done);
        munmap(shared, sizeof(SharedData));
        return EXIT_FAILURE;
    }

    // Leer imagen en SharedData
    if (readImage(source, shared) == -1) { // readImage ahora trabaja con SharedData
        fclose(source);
        freeImage(shared);
        // Cerrar semáforos y desmapear memoria compartida
        sem_close(sem_image_ready);
        sem_close(sem_desenfocar_done);
        sem_close(sem_realzar_done);
        munmap(shared, sizeof(SharedData));
        return EXIT_FAILURE;
    }

    fclose(source);

    // Notificar que la imagen está lista
    if (sem_post(sem_image_ready) == -1) {
        printError(FILE_ERROR);
        // Continuar, pero registrar el error
    }

    // Esperar a que los otros procesos terminen
    if (sem_wait(sem_desenfocar_done) == -1) {
        printError(FILE_ERROR);
        // Continuar
    }
    if (sem_wait(sem_realzar_done) == -1) {
        printError(FILE_ERROR);
        // Continuar
    }

    // Combinar los resultados (esto lo maneja el proceso combinador)

    // Limpieza de recursos
    sem_close(sem_image_ready);
    sem_close(sem_desenfocar_done);
    sem_close(sem_realzar_done);
    munmap(shared, sizeof(SharedData));

    return EXIT_SUCCESS;
}