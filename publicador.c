#include "common.h"
#include "bmp.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <string.h>
#include <unistd.h>

// --- Código de mapeo a memoria compartida ---
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

// --- Hilo que actúa como Combinador ---
static void* combinador_thread(void* arg) {
    SharedData* shared = (SharedData*)arg;

    // Abrir semáforos de desenfoque y realce
    sem_t* sem_desenfocar_done = sem_open(SEM_DESENFOCAR_DONE, 0);
    sem_t* sem_realzar_done    = sem_open(SEM_REALZAR_DONE, 0);

    if (sem_desenfocar_done == SEM_FAILED || sem_realzar_done == SEM_FAILED) {
        printError(FILE_ERROR);
        return NULL;
    }

    // Bucle infinito, esperando que Desenfocador y Realzador acaben
    while (1) {
        printf("[Combinador] Esperando que Desenfocador y Realzador completen...\n");
        if (sem_wait(sem_desenfocar_done) == -1) {
            perror("[Combinador] Error al esperar SEM_DESENFOCAR_DONE");
            break;
        }
        if (sem_wait(sem_realzar_done) == -1) {
            perror("[Combinador] Error al esperar SEM_REALZAR_DONE");
            break;
        }

        // Guardar la imagen final cada vez que haya terminado
        printf("[Combinador] Desenfoque y Realce completados. Guardando la imagen final.\n");
        if (writeImage("salida_final.bmp", shared) == -1) {
            printError(FILE_ERROR);
        } else {
            printf("[Combinador] Imagen final guardada en salida_final.bmp\n");
        }
    }

    sem_close(sem_desenfocar_done);
    sem_close(sem_realzar_done);
    return NULL;
}

// --- Main que actúa como Publicador ---
int main() {
    printf("[Publicador+Combinador] Iniciando.\n");

    // Crear memoria compartida
    SharedData* shared = map_shared_memory();
    if (!shared) return EXIT_FAILURE;

    // Crear/abrir semáforos
    sem_t* sem_desenfocar_ready = sem_open(SEM_DESENFOCAR_READY, O_CREAT, 0666, 0);
    sem_t* sem_realzar_ready    = sem_open(SEM_REALZAR_READY,  O_CREAT, 0666, 0);
    sem_t* sem_desenfocar_done  = sem_open(SEM_DESENFOCAR_DONE, O_CREAT, 0666, 0);
    sem_t* sem_realzar_done     = sem_open(SEM_REALZAR_DONE,    O_CREAT, 0666, 0);

    if (sem_desenfocar_ready == SEM_FAILED ||
        sem_realzar_ready    == SEM_FAILED ||
        sem_desenfocar_done  == SEM_FAILED ||
        sem_realzar_done     == SEM_FAILED) {
        printError(FILE_ERROR);
        return EXIT_FAILURE;
    }

    // Lanzar el hilo combinador en paralelo
    pthread_t combThread;
    pthread_create(&combThread, NULL, combinador_thread, shared);

    // Bucle principal de publicación
    while (1) {
        printf("[Publicador] Ingrese ruta BMP (o 'exit' para terminar): ");
        fflush(stdout);

        char path[256];
        if (!fgets(path, sizeof(path), stdin)) {
            break;
        }
        path[strcspn(path, "\n")] = 0;
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

        // Notificar a Desenfocador y Realzador
        printf("[Publicador] Señalizando procesos...\n");
        sem_post(sem_desenfocar_ready);
        sem_post(sem_realzar_ready);

        // Esperar señales para que publicador sepa que terminó
        printf("[Publicador] Esperando desenfocador...\n");
        if (sem_wait(sem_desenfocar_done) == -1) {
            perror("[Publicador] Error esperando desenfocador");
            continue;
        }
        printf("[Publicador] Desenfocador completado\n");

        printf("[Publicador] Esperando realzador...\n");
        if (sem_wait(sem_realzar_done) == -1) {
            perror("[Publicador] Error esperando realzador");
            continue;
        }
        printf("[Publicador] Realzador completado\n");

        printf("[Publicador] Listo para siguiente imagen.\n\n");
    }

    // Cerrar hilos y semáforos al finalizar
    pthread_cancel(combThread);
    pthread_join(combThread, NULL);

    sem_close(sem_desenfocar_ready);
    sem_close(sem_realzar_ready);
    sem_close(sem_desenfocar_done);
    sem_close(sem_realzar_done);

    sem_unlink(SEM_DESENFOCAR_READY);
    sem_unlink(SEM_REALZAR_READY);
    sem_unlink(SEM_DESENFOCAR_DONE);
    sem_unlink(SEM_REALZAR_DONE);

    // Liberar memoria
    munmap(shared, sizeof(SharedData));
    shm_unlink(SHM_NAME);

    printf("[Publicador+Combinador] Finalizado.\n");
    return EXIT_SUCCESS;
}