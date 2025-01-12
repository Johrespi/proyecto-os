#ifndef _BMP_H_
#define _BMP_H_

#include <stdint.h>
#include <stdio.h>

#define ARGUMENT_ERROR 1
#define FILE_ERROR 2
#define MEMORY_ERROR 3
#define VALID_ERROR 4

#pragma pack(1)
typedef struct __attribute__((packed)) BMP_Header {
    uint16_t type;         
    uint32_t size;      
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset;       
    uint32_t header_size;  
    int32_t  width_px;
    int32_t  height_px;    
    uint16_t planes;         
    uint16_t bits_per_pixel; 
    uint32_t compression;    
    uint32_t imagesize;      
    int32_t  xresolution;
    int32_t  yresolution;
    uint32_t ncolours;
    uint32_t importantcolours;
} BMP_Header;
#pragma pack()

typedef struct __attribute__((packed)) Pixel {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
    uint8_t alpha;
} Pixel;

void printError(int error);
int checkBMPValid(BMP_Header* header);
int readImage(FILE *srcFile, void* sharedVoid);
int writeImage(char* destFileName, void* sharedVoid);

#endif 