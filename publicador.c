#define _POSIX_C_SOURCE 200809L
#include "common.h"
#include "bmp.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

/*
 * Mapea la memoria compartida.
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

/*
 * Espera (con timeout) a que un semáforo aumente.
 * Devuelve:
 *   1 si se obtuvo la señal (proceso sí respondió).
 *   0 si no (timeout, no corrió).
 *  -1 si ocurrió un error distinto.
 */
static int wait_with_timeout(sem_t* sem, const char* name, int seconds) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += seconds;

    if (sem_timedwait(sem, &ts) == -1) {
        if (errno == ETIMEDOUT) {
            printf("[Publicador] No hay %s en ejecución (timeout).\n", name);
            return 0;
        } else {
            perror("[Publicador] Error esperando semáforo");
            return -1;
        }
    }
    printf("[Publicador] %s completado\n", name);
    return 1;
}

int main() {
    printf("[Publicador+Combinador] Iniciando.\n");

    // Crear/mapear memoria compartida
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
        munmap(shared, sizeof(SharedData));
        return EXIT_FAILURE;
    }

    while (1) {
        // 1) Solicitar ruta BMP
        printf("\n[Publicador] Ingrese ruta BMP (o 'exit' para terminar): ");
        fflush(stdout);

        char pathBMP[256];
        if (!fgets(pathBMP, sizeof(pathBMP), stdin)) {
            break; // fin de entrada
        }
        pathBMP[strcspn(pathBMP, "\n")] = 0;
        if (strcmp(pathBMP, "exit") == 0) {
            printf("[Publicador] Saliendo.\n");
            break;
        }

        // 2) Cargar la imagen
        FILE *f = fopen(pathBMP, "rb");
        if (!f) {
            printError(FILE_ERROR);
            continue;
        }
        printf("[Publicador] Leyendo %s...\n", pathBMP);
        if (readImage(f, shared) == -1) {
            fclose(f);
            continue;
        }
        fclose(f);
        printf("[Publicador] Imagen cargada.\n");

        // 3) Señalar a Desenfocador y Realzador
        printf("[Publicador] Señalizando procesos...\n");
        sem_post(sem_desenfocar_ready);
        sem_post(sem_realzar_ready);

        // 4) Esperar a que cada uno termine con timeout
        printf("[Publicador] Esperando desenfocador...\n");
        int desenfocado = wait_with_timeout(sem_desenfocar_done, "Desenfocador", 60);

        printf("[Publicador] Esperando realzador...\n");
        int realzado = wait_with_timeout(sem_realzar_done, "Realzador", 60);

        // 5) Solo guardar si ambos respondieron
        if (desenfocado == 1 && realzado == 1) {
            char pathOut[256];
            printf("[Combinador] Ingrese ruta para guardar la imagen final: ");
            fflush(stdout);
            if (!fgets(pathOut, sizeof(pathOut), stdin)) {
                break; // fin de entrada
            }
            pathOut[strcspn(pathOut, "\n")] = 0;
            if (!strlen(pathOut)) {
                strcpy(pathOut, "salida_final.bmp");
            }

            printf("[Combinador] Desenfoque y Realce completados. Guardando en: %s\n", pathOut);
            if (writeImage(pathOut, shared) == -1) {
                printError(FILE_ERROR);
            } else {
                printf("[Combinador] Imagen final guardada en %s\n", pathOut);
            }
        } else {
            printf("[Publicador] No se aplicó desenfoque/realce. Se omite guardado.\n");
        }

        printf("[Publicador] Listo para siguiente imagen.\n");
    }

    // Cerrar semáforos y liberar recursos
    sem_close(sem_desenfocar_ready);
    sem_close(sem_realzar_ready);
    sem_close(sem_desenfocar_done);
    sem_close(sem_realzar_done);

    sem_unlink(SEM_DESENFOCAR_READY);
    sem_unlink(SEM_REALZAR_READY);
    sem_unlink(SEM_DESENFOCAR_DONE);
    sem_unlink(SEM_REALZAR_DONE);

    munmap(shared, sizeof(SharedData));
    shm_unlink(SHM_NAME);

    printf("[Publicador+Combinador] Finalizado.\n");
    return EXIT_SUCCESS;
}