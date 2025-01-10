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

/* Función para combinar desenfoque y realce */
void combine_images(SharedData *shared) {
    for (int y =0; y < shared->header.height_px; y++) {
        for (int x =0; x < shared->header.width_px; x++) {
            // Combinación simple: media de ambos
            shared->pixels[y][x].red = (shared->pixelsOutDes[y][x].red + shared->pixelsOutReal[y][x].red) /2;
            shared->pixels[y][x].green = (shared->pixelsOutDes[y][x].green + shared->pixelsOutReal[y][x].green) /2;
            shared->pixels[y][x].blue = (shared->pixelsOutDes[y][x].blue + shared->pixelsOutReal[y][x].blue) /2;
            shared->pixels[y][x].alpha = shared->pixelsOutDes[y][x].alpha; // O usar alguno de los dos
        }
    }
}

int writeImage(char* destFileName, SharedData *shared); // Agregar prototipo

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: %s <output_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *output_file = argv[1];

    // Mapear memoria compartida
    SharedData *shared = map_shared_memory();
    if (shared == NULL) {
        return EXIT_FAILURE;
    }

    // Abrir semáforos
    sem_t *sem_desenfocar_done = sem_open(SEM_DESENFOCAR_DONE, 0);
    sem_t *sem_realzar_done = sem_open(SEM_REALZAR_DONE, 0);

    if (sem_desenfocar_done == SEM_FAILED || sem_realzar_done == SEM_FAILED) {
        printError(FILE_ERROR);
        if (sem_desenfocar_done != SEM_FAILED) sem_close(sem_desenfocar_done);
        if (sem_realzar_done != SEM_FAILED) sem_close(sem_realzar_done);
        munmap(shared, sizeof(SharedData));
        return EXIT_FAILURE;
    }

    // Esperar a que desenfocador y realzar terminen
    if (sem_wait(sem_desenfocar_done) == -1) {
        printError(FILE_ERROR);
        // Continuar
    }
    if (sem_wait(sem_realzar_done) == -1) {
        printError(FILE_ERROR);
        // Continuar
    }

    // Combinar las imágenes
    combine_images(shared);

    // Escribir la imagen combinada
    if (writeImage(output_file, shared) == -1) {
        printError(FILE_ERROR);
        // Continuar
    }

    // Cerrar semáforos y desmapear memoria compartida
    sem_close(sem_desenfocar_done);
    sem_close(sem_realzar_done);
    munmap(shared, sizeof(SharedData));

    return EXIT_SUCCESS;
}