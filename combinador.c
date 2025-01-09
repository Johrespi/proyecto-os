#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <string.h>
#include <unistd.h>
#include "common.h"

/* Función para combinar las mitades desenfocada y realzada */
void combinarMitades(SharedData *shared) {
    int halfHeight = shared->header.height_px / 2;
    for (int y = 0; y < shared->header.height_px; y++) {
        for (int x = 0; x < shared->header.width_px; x++) {
            if (y < halfHeight) {
                // Tomar píxeles de la imagen desenfocada
                shared->pixels[y][x] = shared->pixelsOutDes[y][x];
            } else {
                // Tomar píxeles de la imagen realzada
                shared->pixels[y][x] = shared->pixelsOutReal[y][x];
            }
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Uso: %s <ruta_salida_imagen>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *output_path = argv[1];

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
    sem_t *sem_desenfocar_done = sem_open(SEM_DESENFOCAR_DONE, 0);
    sem_t *sem_realzar_done = sem_open(SEM_REALZAR_DONE, 0);

    if (sem_desenfocar_done == SEM_FAILED || sem_realzar_done == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    while (1) {
        // Esperar a que ambos procesos hayan terminado
        sem_wait(sem_desenfocar_done);
        sem_wait(sem_realzar_done);

        printf("Combinador: Combinando mitades de la imagen...\n");

        // Combinar las mitades desenfocada y realzada
        combinarMitades(shared);

        // Escribir la imagen combinada en disco
        FILE *destFile = fopen(output_path, "wb");
        if (destFile == NULL) {
            perror("fopen");
            continue;
        }

        // Escribir el encabezado
        if (fwrite(&(shared->header), sizeof(BMP_Header), 1, destFile) != 1) {
            printError(FILE_ERROR);
            fclose(destFile);
            continue;
        }

        // Saltar al offset de los datos de imagen
        if (fseek(destFile, shared->header.offset, SEEK_SET) != 0) {
            printError(FILE_ERROR);
            fclose(destFile);
            continue;
        }

        // Escribir los píxeles
        for (int y = 0; y < shared->header.height_px; y++) {
            if (fwrite(shared->pixels[y], sizeof(Pixel), shared->header.width_px, destFile) != shared->header.width_px) {
                printError(FILE_ERROR);
                break;
            }
        }

        fclose(destFile);

        printf("Combinador: Imagen combinada guardada en %s\n", output_path);
    }

    // Cerrar semáforos y memoria compartida
    sem_close(sem_desenfocar_done);
    sem_close(sem_realzar_done);
    munmap(shared, SHM_SIZE);
    close(shm_fd);

    return 0;
}