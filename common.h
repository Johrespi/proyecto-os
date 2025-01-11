#ifndef COMMON_H
#define COMMON_H

#include "bmp.h"      // Aquí se definen BMP_Header y Pixel
#include <semaphore.h>
#include <stdint.h>
#include <sys/types.h>

#define SHM_NAME "/bmp_shared_mem"
#define MAX_WIDTH 1920
#define MAX_HEIGHT 1080

// Nombres de semáforos
#define SEM_DESENFOCAR_READY  "/sem_desenfocar_ready"  // New
#define SEM_REALZAR_READY     "/sem_realzar_ready"     // New
#define SEM_DESENFOCAR_DONE  "/sem_desenfocar_done"
#define SEM_REALZAR_DONE     "/sem_realzar_done"

// Estructura mapeada en memoria compartida
typedef struct {
    // Encabezado completo del BMP
    BMP_Header header;
    // Datos de la imagen
    Pixel pixels[MAX_HEIGHT][MAX_WIDTH];
} SharedData;

#endif // COMMON_H