#ifndef COMMON_H
#define COMMON_H

#include "bmp.h"

#define SHM_NAME "/bmp_shared_mem"

// Definir dimensiones máximas según tus necesidades
#define MAX_WIDTH 1920
#define MAX_HEIGHT 1080

// Tamaño de la memoria compartida
#define SHM_SIZE (sizeof(SharedData))

// Nombres de los semáforos
#define SEM_IMAGE_READY "/sem_image_ready"
#define SEM_DESENFOCAR_DONE "/sem_desenfocar_done"
#define SEM_REALZAR_DONE "/sem_realzar_done"

// Estructura para compartir datos sin punteros internos
typedef struct {
    BMP_Header header;                           // Encabezado BMP
    Pixel pixels[MAX_HEIGHT][MAX_WIDTH];         // Imagen BMP original
    Pixel pixelsOutDes[MAX_HEIGHT][MAX_WIDTH];   // Imagen desenfocada
    Pixel pixelsOutReal[MAX_HEIGHT][MAX_WIDTH];  // Imagen realzada
} SharedData;

// Declaración de la función para duplicar encabezado (si aún es necesaria)
BMP_Image *duplicateBMPImageHeader(BMP_Image *sourceImage);

#endif // COMMON_H