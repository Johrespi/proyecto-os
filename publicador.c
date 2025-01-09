#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <string.h>
#include <unistd.h>
#include "common.h"

/* Función principal del Publicador */
int main() {
    // Crear y mapear la memoria compartida
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shm_fd, SHM_SIZE) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    SharedData *shared = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // Abrir semáforos
    sem_t *sem_image_ready = sem_open(SEM_IMAGE_READY, O_CREAT, 0666, 0);
    sem_t *sem_desenfocar_done = sem_open(SEM_DESENFOCAR_DONE, O_CREAT, 0666, 0);
    sem_t *sem_realzar_done = sem_open(SEM_REALZAR_DONE, O_CREAT, 0666, 0);

    if (sem_image_ready == SEM_FAILED || sem_desenfocar_done == SEM_FAILED || sem_realzar_done == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    char image_path[256];
    while (1) {
        printf("Ingrese la ruta de la imagen BMP (o 'exit' para salir): ");
        scanf("%s", image_path);

        if (strcmp(image_path, "exit") == 0) {
            break;
        }

        FILE *source = fopen(image_path, "rb");
        if (source == NULL) {
            perror("fopen");
            continue;
        }

        // Crear la imagen
        BMP_Image *image = createBMPImage();
        readImage(source, image);
        if (image == NULL) {
            printf("Error al crear la imagen BMP.\n");
            fclose(source);
            continue;
        }

        // Asignar la imagen cargada a la memoria compartida
        shared->image = *image;
        freeImage(image); // Liberar la memoria temporal

        fclose(source);

        // Señalar que una nueva imagen está lista
        sem_post(sem_image_ready);
        printf("Imagen cargada en memoria compartida.\n");
    }

    // Cerrar y eliminar semáforos y memoria compartida
    sem_close(sem_image_ready);
    sem_close(sem_desenfocar_done);
    sem_close(sem_realzar_done);

    sem_unlink(SEM_IMAGE_READY);
    sem_unlink(SEM_DESENFOCAR_DONE);
    sem_unlink(SEM_REALZAR_DONE);

    munmap(shared, SHM_SIZE);
    shm_unlink(SHM_NAME);

    return 0;
}