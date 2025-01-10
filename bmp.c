#include "bmp.h"
#include "common.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void printError(int error) {
    switch(error){
        case ARGUMENT_ERROR:
            printf("Error: Argumento inválido.\n");
            break;
        case FILE_ERROR:
            printf("Error: Archivo no encontrado o no accesible.\n");
            break;
        case MEMORY_ERROR:
            printf("Error: Fallo en la asignación de memoria.\n");
            break;
        case VALID_ERROR:
            printf("Error: Archivo BMP inválido.\n");
            break;
        default:
            printf("Error desconocido.\n");
    }
}

BMP_Image* createBMPImage() {
    BMP_Image *image = malloc(sizeof(BMP_Image));
    if (image == NULL) {
        printError(MEMORY_ERROR);
        return NULL;
    }
    memset(image, 0, sizeof(BMP_Image));
    return image;
}

int readImageData(FILE *srcFile, SharedData *shared, int dataSize) {
    for (int y = 0; y < shared->header.height_px; y++) {
        if (fread(shared->pixels[y], sizeof(Pixel), shared->header.width_px, srcFile) != shared->header.width_px) {
            printError(FILE_ERROR);
            return -1;
        }
    }
    return 0;
}

int readImage(FILE *srcFile, SharedData *shared) {
    if (fread(&(shared->header), sizeof(BMP_Header), 1, srcFile) != 1) {
        printError(FILE_ERROR);
        return -1;
    }

    if (!checkBMPValid(&(shared->header))) {
        printError(VALID_ERROR);
        return -1;
    }

    shared->header.height_px = shared->header.height_px < 0 ? -shared->header.height_px : shared->header.height_px;

    if (shared->header.width_px > MAX_WIDTH || shared->header.height_px > MAX_HEIGHT) {
        printError(VALID_ERROR);
        return -1;
    }

    if (fseek(srcFile, shared->header.offset, SEEK_SET) != 0) {
        printError(FILE_ERROR);
        return -1;
    }

    return readImageData(srcFile, shared, shared->header.width_px * shared->header.height_px);
}

int writeImage(char* destFileName, SharedData *shared) {
    FILE *destFile = fopen(destFileName, "wb");
    if (destFile == NULL) {
        printError(FILE_ERROR);
        return -1;
    }

    if (fwrite(&(shared->header), sizeof(BMP_Header), 1, destFile) != 1) {
        printError(FILE_ERROR);
        fclose(destFile);
        return -1;
    }

    if (fseek(destFile, shared->header.offset, SEEK_SET) != 0) {
        printError(FILE_ERROR);
        fclose(destFile);
        return -1;
    }

    for (int y = 0; y < shared->header.height_px; y++) {
        if (fwrite(shared->pixels[y], sizeof(Pixel), shared->header.width_px, destFile) != shared->header.width_px) {
            printError(FILE_ERROR);
            fclose(destFile);
            return -1;
        }
    }

    fclose(destFile);
    return 0;
}

void freeImage(SharedData* shared) {
    // No dynamic memory to free
}

int checkBMPValid(BMP_Header* header) {
    return (header->type == 0x4D42);
}

void printBMPHeader(BMP_Header* header) {
    printf("Tipo: %c%c\n", header->type & 0xFF, (header->type >> 8) & 0xFF);
    printf("Tamaño: %u bytes\n", header->size);
    printf("Offset de datos: %u bytes\n", header->offset);
    printf("Ancho: %d px\n", header->width_px);
    printf("Alto: %d px\n", header->height_px);
    printf("Planes: %u\n", header->planes);
    printf("Bits por pixel: %u\n", header->bits_per_pixel);
}

void printBMPImage(SharedData* shared) {
    printBMPHeader(&(shared->header));
}