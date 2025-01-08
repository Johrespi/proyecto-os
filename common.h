#ifndef COMMON_H
#define COMMON_H

#include "bmp.h"

// Nombre de la memoria compartida
#define SHM_NAME "/bmp_shared_mem"

// Tamaño máximo de la imagen en bytes (ajustar según necesidades)
#define SHM_SIZE (50 * 1024 * 1024) // 50 MB

// Nombres de los semáforos
#define SEM_IMAGE_READY "/sem_image_ready"
#define SEM_DESENFOCAR_DONE "/sem_desenfocar_done"
#define SEM_REALZAR_DONE "/sem_realzar_done"

// Estructura para compartir datos
typedef struct {
    BMP_Image image;           // Imagen BMP original
    BMP_Image imageOutDes;     // Imagen procesada por Desenfocador
    BMP_Image imageOutReal;    // Imagen procesada por Realzador
} SharedData;

// Declaración de la función para duplicar encabezado
BMP_Image *duplicateBMPImageHeader(BMP_Image *sourceImage);

#endif // COMMON_H