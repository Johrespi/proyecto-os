#include "common.h"
#include "bmp.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>

/*
 * En esta versión, desenfoque y realce editan la imagen directamente,
 * por lo que la “combinación” final puede ser trivial.
 * Pero se deja separada la función si se desea mezclar.
 */
static void combineHalves(SharedData* shared) {
    // Aquí no hacemos nada, ya que las mitades fueron procesadas in-place.
    // Podríamos mezclar, si fuese necesario.
    printf("[Combinador] No hay combinación extra requerida.\n");
}

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
        printf("Uso: %s <salida_final.bmp>\n", argv[0]);
        return EXIT_FAILURE;
    }
    char* outFile = argv[1];
    printf("[Combinador] Iniciando.\n");

    SharedData* shared = map_shared_memory();
    if (!shared) return EXIT_FAILURE;

    // Combinar mitades
    combineHalves(shared);

    // Guardar resultado final
    if (writeImage(outFile, shared) == -1) {
        printError(FILE_ERROR);
    } else {
        printf("[Combinador] Imagen final guardada en %s\n", outFile);
    }
    munmap(shared, sizeof(SharedData));
    printf("[Combinador] Finalizado.\n");
    return EXIT_SUCCESS;
}