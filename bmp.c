#include "bmp.h"
#include "common.h"
#include <stdlib.h>
#include <string.h>

// Función que muestra mensajes de errores según sea el caso.
void printError(int error) {
    switch(error) {
        case ARGUMENT_ERROR:
            printf("Error: Argumento inválido.\n");
            break;
        case FILE_ERROR:
            printf("Error: Archivo no accesible o no encontrado.\n");
            break;
        case MEMORY_ERROR:
            printf("Error: Fallo de memoria.\n");
            break;
        case VALID_ERROR:
            printf("Error: Archivo BMP inválido o no soportado.\n");
            break;
        default:
            printf("Error desconocido.\n");
    }
}

// Función que verifica que el BMP sea válido para procesar.
int checkBMPValid(BMP_Header* header) {
    if (header->type != 0x4D42) return 0;
    if (header->compression != 0) return 0;
    if (header->bits_per_pixel != 24 && header->bits_per_pixel != 32) return 0;
    return 1;
}


// Función que lee la imagen BMP mediante el recurso compartido, maneja las imagenes de 24/32 bits según
// sea el caso y corrige si la imagen está invertida (flip).
static int readImageData(FILE *srcFile, SharedData* shared, int inverted) {
    int width = shared->header.width_px;
    int height = shared->header.height_px;
    int bpp = shared->header.bits_per_pixel;

    for (int i = 0; i < height; i++) {
        // Si era bottom-up (height>0), la primera línea que leemos va al final
        // Si era top-down (negativo), invertido=1 y leemos en orden normal.
        int rowIndex = inverted ? i : (height - 1 - i);

        for (int x = 0; x < width; x++) {
            if (bpp == 24) {
                unsigned char bgr[3];
                if (fread(bgr, 3, 1, srcFile) != 1) {
                    printError(FILE_ERROR);
                    return -1;
                }
                shared->pixels[rowIndex][x].blue  = bgr[0];
                shared->pixels[rowIndex][x].green = bgr[1];
                shared->pixels[rowIndex][x].red   = bgr[2];
                shared->pixels[rowIndex][x].alpha = 255;
            } else {
                // 32 bits (BGRA)
                unsigned char bgra[4];
                if (fread(bgra, 4, 1, srcFile) != 1) {
                    printError(FILE_ERROR);
                    return -1;
                }
                shared->pixels[rowIndex][x].blue  = bgra[0];
                shared->pixels[rowIndex][x].green = bgra[1];
                shared->pixels[rowIndex][x].red   = bgra[2];
                shared->pixels[rowIndex][x].alpha = bgra[3];
            }
        }
        // Padding en 24 bits a múltiplos de 4
        if (bpp == 24) {
            int rowPad = (4 - ((width * 3) % 4)) % 4;
            if (rowPad) fseek(srcFile, rowPad, SEEK_CUR);
        }
    }
    return 0;
}

int readImage(FILE *srcFile, void* sharedVoid) {
    SharedData* shared = (SharedData*)sharedVoid;

    // Leer encabezado
    if (fread(&(shared->header), sizeof(BMP_Header), 1, srcFile) != 1) {
        printError(FILE_ERROR);
        return -1;
    }
    // Validar
    if (!checkBMPValid(&(shared->header))) {
        printError(VALID_ERROR);
        return -1;
    }

    // Detectar si está invertido (top-down)
    int inverted = 0;
    if (shared->header.height_px < 0) {
        inverted = 1;
        shared->header.height_px = -shared->header.height_px;
    }

    // Valida tamaño de la imagen (ancho y alto) según los límites establecidos en common.h
    if (shared->header.width_px > MAX_WIDTH || shared->header.height_px > MAX_HEIGHT) {
        printError(VALID_ERROR);
        return -1;
    }

    // Ir a offset
    if (fseek(srcFile, shared->header.offset, SEEK_SET) != 0) {
        printError(FILE_ERROR);
        return -1;
    }

    // Leer data de la imagen
    return readImageData(srcFile, shared, inverted);
}


// Escribe la imagen al disco en formato bottom-up (height>0).
int writeImage(char* destFileName, void* sharedVoid) {
    SharedData* shared = (SharedData*)sharedVoid;
    FILE* out = fopen(destFileName, "wb");
    if (!out) {
        printError(FILE_ERROR);
        return -1;
    }

    // Escribir encabezado
    if (fwrite(&(shared->header), sizeof(BMP_Header), 1, out) != 1) {
        printError(FILE_ERROR);
        fclose(out);
        return -1;
    }

    // Posicionarnos en offset
    if (fseek(out, shared->header.offset, SEEK_SET) != 0) {
        printError(FILE_ERROR);
        fclose(out);
        return -1;
    }

    int width = shared->header.width_px;
    int height= shared->header.height_px;
    int bpp   = shared->header.bits_per_pixel;

    // Guardar en orden bottom-up
    for (int i = 0; i < height; i++) {
        int rowIndex = (height - 1 - i);

        for (int x = 0; x < width; x++) {
            Pixel p = shared->pixels[rowIndex][x];
            if (bpp == 24) {
                unsigned char bgr[3];
                bgr[0] = p.blue;
                bgr[1] = p.green;
                bgr[2] = p.red;
                if (fwrite(bgr, 3, 1, out) != 1) {
                    printError(FILE_ERROR);
                    fclose(out);
                    return -1;
                }
            } else {
                unsigned char bgra[4];
                bgra[0] = p.blue;
                bgra[1] = p.green;
                bgra[2] = p.red;
                bgra[3] = p.alpha;
                if (fwrite(bgra, 4, 1, out) != 1) {
                    printError(FILE_ERROR);
                    fclose(out);
                    return -1;
                }
            }
        }
        // Padding si la imagen es de 24 bits
        if (bpp == 24) {
            int rowPad = (4 - ((width * 3) % 4)) % 4;
            if (rowPad) {
                unsigned char pad[3] = {0};
                fwrite(pad, rowPad, 1, out);
            }
        }
    }

    fclose(out);
    return 0;
}