#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

char *inputImage;

#pragma pack(push)
#pragma pack(2)

typedef struct {
    uint16_t type;
    uint32_t size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset;
} BMP_header;

typedef struct {
    uint32_t header_size;
    uint32_t width;
    uint32_t height;
    uint16_t color_planes;
    uint16_t bits_per_px;
    uint32_t compression_method;
    uint32_t image_size_bytes;
    uint32_t x_resolution_ppm;
    uint32_t y_resolution_ppm;
    uint32_t number_of_colors;
    uint32_t important_colors;
} DIB_header;

typedef struct {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
} RGB;

typedef struct {
    int height;
    int width;
    RGB **rgb;
} Image;

#pragma pack(pop)

Image readImage(FILE *fp, int height, int width) {
    Image image;
    image.rgb = (RGB**)malloc(height * sizeof(void*));
    image.height = height;
    image.width = width;
    int bytesToRead = ((24 * width + 31) / 32) * 4;
    int numberOfRGB = bytesToRead / sizeof(RGB) + 1;                // extra bit for cases like- 16 % 3 = 1 -> need to store in that part

    for (int i = height - 1; i >= 0; --i) {
        image.rgb[i] = (RGB*)malloc(numberOfRGB * sizeof(RGB));
        (void)!fread(image.rgb[i], 1, bytesToRead, fp);             // fread -> inArea, elementSize, count, fp
    }
    return image;
}

void freeImage(Image image) {
    int i;
    for (i = image.height - 1; i >= 0; --i) {
        free(image.rgb[i]);
    }
    free(image.rgb);
}

uint8_t grayscale(RGB rgb) {
    return (0.3 * rgb.red) + (0.6 * rgb.green) + (0.1 * rgb.blue);
}

void convertRGBToGrayscale(Image image) {
    for (int i = 0; i < image.height; ++i) {
        for (int j = 0; j < image.width; ++j) {
            image.rgb[i][j].red = image.rgb[i][j].green = image.rgb[i][j].blue = grayscale(image.rgb[i][j]);
        }
    }
}

void imageToText(Image image) {
    uint8_t gs;
    uint8_t textPixel[] = { '@', '#', '%', 'O', 'a', '-', '.', ' ' };

    for (int i = 0; i < image.height; ++i) {
        for (int j = 0; j < image.width; ++j) {
            gs = grayscale(image.rgb[i][j]);
            printf("%c", textPixel[7 - gs / 32]);
        }
        printf("\n");
    }
}

int createBlackWhiteImage(BMP_header bmp_header, DIB_header dib_header, Image image) {
    FILE *fpw = fopen("new.bmp", "w");
    if (!fpw) return 1;

    convertRGBToGrayscale(image);

    (void)!fwrite(&bmp_header, sizeof(BMP_header), 1, fpw);
    (void)!fwrite(&dib_header, sizeof(DIB_header), 1, fpw);

    for (int i = image.height - 1; i >= 0; --i) {
        (void)!fwrite(image.rgb[i], ((24 * image.width + 31) / 32) * 4, 1, fpw);
    }

    fclose(fpw);
    return 0;
}

void printHeader(BMP_header bmp_header, DIB_header dib_header) {
    int wide = 30;
    for (int i = 0; i < wide; ++i) { printf("-"); if (i == wide - 1) printf("\n"); }
    printf("Type: %c%c\n", bmp_header.type, (bmp_header.type >> 8));
    printf("Size: %d\n", bmp_header.size);
    printf("Offset: %d\n", bmp_header.offset);
    printf("Header size: %d\n", dib_header.header_size);
    printf("Width: %d\n", dib_header.width);
    printf("Height: %d\n", dib_header.height);
    printf("Color planes: %d\n", dib_header.color_planes);
    printf("Bits per pixel: %d\n", dib_header.bits_per_px);
    printf("Compression method: %d\n", dib_header.compression_method);
    printf("Image size(in bytes): %d\n", dib_header.image_size_bytes);
    for (int i = 0; i < wide; ++i) { printf("-"); if (i == wide - 1) printf("\n"); }
}

void openImage() {
    FILE *fp = fopen(inputImage, "rb");
    BMP_header bmp_header;
    (void)!fread(&bmp_header, sizeof(BMP_header), 1, fp);               // Args: structure variable, size of that variable, no. of record, file pointer
    if (bmp_header.type != 0x4D42) { 
        printf("Not a BMP image\n");
        fclose(fp);
        return;
    }

    DIB_header dib_header;
    (void)!fread(&dib_header, sizeof(DIB_header), 1, fp);
    if ((dib_header.header_size != 40) || (dib_header.compression_method) || (dib_header.bits_per_px != 24)) {
        printf("!! Error in DIB header !!");
        fclose(fp);
        return;
    }

    // debugging
    printHeader(bmp_header, dib_header);

    fseek(fp, bmp_header.offset, SEEK_SET);
    Image image = readImage(fp, dib_header.height, dib_header.width);
    // imageToText(image);

    if (createBlackWhiteImage(bmp_header, dib_header, image)) {
        printf("!! Error in creating BW image !!\n");
    } else {
        printf("✔ Created BW image of %s\n", inputImage);
    }

    fclose(fp);
    freeImage(image);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Wrong command!\n");
        return 1;
    } 
    inputImage = (char*)malloc(sizeof(char*));
    strcpy(inputImage, argv[1]);
    openImage();
    return 0;
}
