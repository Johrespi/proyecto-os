#ifndef COMMON_H
#define COMMON_H

#include "bmp.h"   
#include <semaphore.h>
#include <stdint.h>
#include <sys/types.h>

#define SHM_NAME "/bmp_shared_mem"
#define MAX_WIDTH 1920
#define MAX_HEIGHT 1080

// Nombres de los semáforos que se van a utilizar durante el proceso
#define SEM_DESENFOCAR_READY  "/sem_desenfocar_ready"
#define SEM_REALZAR_READY     "/sem_realzar_ready"
#define SEM_DESENFOCAR_DONE  "/sem_desenfocar_done"
#define SEM_REALZAR_DONE     "/sem_realzar_done"

// Estructura a mapear en la memoria compartida
typedef struct {
    BMP_Header header;
    Pixel pixels[MAX_HEIGHT][MAX_WIDTH];
} SharedData;

#endif 