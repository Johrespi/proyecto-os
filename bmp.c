#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "bmp.h"

/* USE THIS FUNCTION TO PRINT ERROR MESSAGES
   DO NOT MODIFY THIS FUNCTION
*/
void printError(int error)
{
  switch (error)
  {
  case ARGUMENT_ERROR:
    printf("Usage:ex5 <source> <destination>\n");
    break;
  case FILE_ERROR:
    printf("Unable to open file!\n");
    break;
  case MEMORY_ERROR:
    printf("Unable to allocate memory!\n");
    break;
  case VALID_ERROR:
    printf("BMP file not valid!\n");
    break;
  default:
    break;
  }
}

/* The input argument is the source file pointer. The function will first construct a BMP_Image image by allocating memory to it.
 * Then the function read the header from source image to the image's header.
 * Compute data size, width, height, and bytes_per_pixel of the image and stores them as image's attributes.
 * Finally, allocate memory for image's data according to the image size.
 * Return image;
 */
BMP_Image *createBMPImage()
{
  BMP_Image *image = (BMP_Image *)malloc(sizeof(BMP_Image));
  if (image == NULL)
  {
    printError(MEMORY_ERROR);
    exit(EXIT_FAILURE);
  }

  return image;
}

/* The input arguments are the source file pointer, the image data pointer, and the size of image data.
 * The functions reads data from the source into the image data matrix of pixels.
 */
void readImageData(FILE *srcFile, BMP_Image *dataImage, int dataSize)
{
  fseek(srcFile, dataImage->header.offset, SEEK_SET);
  
  int row_size = dataImage->header.width_px * dataImage->bytes_per_pixel;
  
  // Calcular padding 
  int padding = (4 - (row_size) % 4) % 4;
  
  int height = dataImage->norm_height;

  int is_flipped = dataImage->header.height_px > 0; // Verificar si la imagen está invertida.

  for (int i = 0; i < height; i++)
  {
    int row_index = is_flipped ? (height - 1 - i) : i;
    fread(dataImage->pixels[row_index], row_size, 1, srcFile);

    fseek(srcFile, padding, SEEK_CUR);
  }
}

/* The input arguments are the pointer of the binary file, and the image data pointer.
 * The functions open the source file and call to CreateBMPImage to load the data image.
 */
void readImage(FILE *srcFile, BMP_Image *dataImage)
{
  // Coloca el puntero del archivo al principio
  fseek(srcFile, 0, SEEK_SET);

  // Leer el encabezado BMP directamente en dataImage
  size_t bytesRead = fread(&dataImage->header, sizeof(BMP_Header), 1, srcFile);
  if (bytesRead != 1)
  {
    printError(VALID_ERROR);
    freeImage(dataImage);
    exit(EXIT_FAILURE);
  }

  // Verificar si el archivo es un BMP válido
  if (!checkBMPValid(&dataImage->header))
  {
    printError(VALID_ERROR);
    freeImage(dataImage);
    exit(EXIT_FAILURE);
  }
  
  // Calcular la altura absoluta y los bytes por pixel
  dataImage->norm_height = abs(dataImage->header.height_px);
  dataImage->bytes_per_pixel = dataImage->header.bits_per_pixel / 8;

  // Asignar memoria para las filas de píxeles
  dataImage->pixels = (Pixel **)malloc(dataImage->norm_height * sizeof(Pixel *));

  if (dataImage->pixels == NULL)
  {
    freeImage(dataImage);
    printError(MEMORY_ERROR);
    exit(EXIT_FAILURE);
  }

  // Asignar memoria para cada fila de pixeles
  for (int i = 0; i < dataImage->norm_height; i++)
  {
    dataImage->pixels[i] = (Pixel *)malloc(dataImage->header.width_px * sizeof(Pixel));
    if (dataImage->pixels[i] == NULL)
    {
      for (int j = 0; j < i; j++)
      {
        free(dataImage->pixels[j]);
      }
      free(dataImage->pixels);
      free(dataImage);
      printError(MEMORY_ERROR);
      exit(EXIT_FAILURE);
    }
  }

  // Leer los datos de los píxeles
  int dataSize = dataImage->header.imagesize ? dataImage->header.imagesize : (dataImage->header.size - dataImage->header.offset);
  readImageData(srcFile, dataImage, dataSize);
}

/* The input argument is the destination file name, and BMP_Image pointer.
 * The function write the header and image data into the destination file.
 */
void writeImage(char *destFileName, BMP_Image *dataImage)
{
  FILE *destFile = fopen(destFileName, "wb");
  if (destFile == NULL)
  {
    printError(FILE_ERROR);
    exit(EXIT_FAILURE);
  }

  fwrite(&dataImage->header, sizeof(BMP_Header), 1, destFile);

  int row_size = dataImage->header.width_px * dataImage->bytes_per_pixel;

  int padding = (4 - (row_size) % 4) % 4;
  char pad[3] = {0, 0, 0};

  int height = dataImage->norm_height;
  int is_flipped = dataImage->header.height_px > 0;

  for (int i = 0; i < dataImage->norm_height; i++)
  {
    int row_index = is_flipped ? (height - 1 - i) : i;
    fwrite(dataImage->pixels[row_index], row_size, 1, destFile);
    fwrite(pad, padding, 1, destFile);
  }

  fclose(destFile);
}

/* The input argument is the BMP_Image pointer. The function frees memory of the BMP_Image.
 */
void freeImage(BMP_Image *image)
{
  if (image != NULL)
  {
    if (image->pixels != NULL)
    {
      for (int i = 0; i < image->norm_height; i++)
      {
        if (image->pixels[i] != NULL)
        {
          free(image->pixels[i]);
        }
      }
      free(image->pixels);
    }
    free(image);
  }
}

/* The functions checks if the source image has a valid format.
 * It returns TRUE if the image is valid, and returns FALSE if the image is not valid.
 * DO NOT MODIFY THIS FUNCTION
 */
int checkBMPValid(BMP_Header *header)
{
  if (header->type != 0x4d42)
  {
    return FALSE;
  }
  if (header->bits_per_pixel != 32 && header->bits_per_pixel != 24)
  {
    return FALSE;
  }
  if (header->planes != 1)
  {
    return FALSE;
  }
  if (header->compression != 0)
  {
    return FALSE;
  }
  return TRUE;
}

/* The function prints all information of the BMP_Header.
   DO NOT MODIFY THIS FUNCTION
*/
void printBMPHeader(BMP_Header *header)
{
  printf("file type (should be 0x4d42): %x\n", header->type);
  printf("file size: %d\n", header->size);
  printf("offset to image data: %d\n", header->offset);
  printf("header size: %d\n", header->header_size);
  printf("width_px: %d\n", header->width_px);
  printf("height_px: %d\n", header->height_px);
  printf("planes: %d\n", header->planes);
  printf("bits: %d\n", header->bits_per_pixel);
}

/* The function prints information of the BMP_Image.
   DO NOT MODIFY THIS FUNCTION
*/
void printBMPImage(BMP_Image *image)
{
  printf("data size is %ld\n", sizeof(image->pixels));
  printf("norm_height size is %d\n", image->norm_height);
  printf("bytes per pixel is %d\n", image->bytes_per_pixel);
}

/* Implementación de duplicateBMPImageHeader */
BMP_Image *duplicateBMPImageHeader(BMP_Image *sourceImage) {
    if (sourceImage == NULL) {
        return NULL;
    }

    BMP_Image *newImage = (BMP_Image *)malloc(sizeof(BMP_Image));
    if (newImage == NULL) {
        printError(MEMORY_ERROR);
        exit(EXIT_FAILURE);
    }

    // Copiar el encabezado
    newImage->header = sourceImage->header;
    newImage->norm_height = sourceImage->norm_height;
    newImage->bytes_per_pixel = sourceImage->bytes_per_pixel;

    // Asignar memoria para los píxeles
    newImage->pixels = (Pixel **)malloc(newImage->norm_height * sizeof(Pixel *));
    if (newImage->pixels == NULL) {
        freeImage(newImage);
        printError(MEMORY_ERROR);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < newImage->norm_height; i++) {
        newImage->pixels[i] = (Pixel *)malloc(newImage->header.width_px * sizeof(Pixel));
        if (newImage->pixels[i] == NULL) {
            for (int j = 0; j < i; j++) {
                free(newImage->pixels[j]);
            }
            free(newImage->pixels);
            free(newImage);
            printError(MEMORY_ERROR);
            exit(EXIT_FAILURE);
        }
    }

    return newImage;
}