#ifndef _BMP_H_
#define _BMP_H_

#include <stdint.h>
#include <stdio.h>

/*
 * Constantes de error
 */
#define ARGUMENT_ERROR 1
#define FILE_ERROR 2
#define MEMORY_ERROR 3
#define VALID_ERROR 4

/*
 * Encabezado BMP (24/32 bits)
 */
#pragma pack(1)
typedef struct __attribute__((packed)) BMP_Header {
    uint16_t type;           // 'BM' (0x4D42)
    uint32_t size;           // Tamaño completo del archivo
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset;         // Offset a la zona de datos
    uint32_t header_size;    // Tamaño del DIB header (40 normalmente)
    int32_t  width_px;
    int32_t  height_px;      // Negativo si es top-down
    uint16_t planes;         // Debe ser 1
    uint16_t bits_per_pixel; // 24 o 32 en nuestro caso
    uint32_t compression;    // 0 (BI_RGB)
    uint32_t imagesize;      // Normalmente 0 para BI_RGB
    int32_t  xresolution;
    int32_t  yresolution;
    uint32_t ncolours;
    uint32_t importantcolours;
} BMP_Header;
#pragma pack()

/*
 * Estructura para un pixel
 * Si la imagen es de 24 bits, alpha se forzará a 255
 */
typedef struct __attribute__((packed)) Pixel {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
    uint8_t alpha;
} Pixel;

/* Funciones principales */
void printError(int error);
int checkBMPValid(BMP_Header* header);

/* Leer y escribir la imagen BMP al recurso compartido */
int readImage(FILE *srcFile, void* sharedVoid);
int writeImage(char* destFileName, void* sharedVoid);

#endif // _BMP_H_