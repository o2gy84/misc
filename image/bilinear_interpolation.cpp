#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <jpeglib.h>

struct image_t{
    image_t() : w(0), h(0), pixels(0) {}
    unsigned char *pixels;
    unsigned int w;
    unsigned int h;
};

struct pixel_t {
    unsigned char b;
    unsigned char g;
    unsigned char r;
};

bool read_jpg(const char * path, image_t *image)
{
    FILE *file = fopen(path, "rb");
    if ( file == NULL )
    {
        return false;
    }

    struct jpeg_decompress_struct info;
    struct jpeg_error_mgr err;

    info.err = jpeg_std_error( &err );
    jpeg_create_decompress( &info );

    jpeg_stdio_src( &info, file );
    jpeg_read_header( &info, true );

    jpeg_start_decompress( &info );

    int w = info.output_width;
    int h = info.output_height;
    int numChannels = info.num_components; // 3 = RGB, 4 = RGBA
    unsigned long dataSize = w * h * numChannels;

    // read RGB(A) scanlines one at a time into data[]
    unsigned char *data = (unsigned char *)malloc( dataSize );

    unsigned char* rowptr;
    while ( info.output_scanline < h )
    {
        rowptr = data + info.output_scanline * w * numChannels;
        jpeg_read_scanlines( &info, &rowptr, 1 );
    }

    jpeg_finish_decompress( &info );    
    fclose( file );

    image->pixels = data;
    image->h = h;
    image->w = w;

    return true;
}

void write_jpg(const char *path, image_t *image)
{
    struct jpeg_compress_struct cinfo;
    
    jpeg_create_compress(&cinfo);

    FILE *fp = fopen(path, "w+");
    if (!fp)
    {
        std::cerr << "error open file" << std::endl;
        return;
    }
    jpeg_stdio_dest(&cinfo, fp);

    struct jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr); 
    cinfo.image_width = image->w;
    cinfo.image_height = image->h;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    jpeg_set_defaults(&cinfo);

    jpeg_start_compress(&cinfo, FALSE);

    unsigned char *stride = (unsigned char *)malloc(image->w * 3);
    unsigned char *data = (unsigned char*)image->pixels;

    JSAMPROW row_pointer[1];
    for (int i = 0; i < image->h; ++i) {
        memcpy(stride, data + i * image->w * 3, image->w * 3);
        row_pointer[0] = stride;
        jpeg_write_scanlines(&cinfo, &stride, 1);
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    fclose(fp);
}

pixel_t pixel(image_t *im, int h, int w)
{
    pixel_t p;
    p.b = im->pixels[h*im->w*3 + w*3 + 0];
    p.g = im->pixels[h*im->w*3 + w*3 + 1];
    p.r = im->pixels[h*im->w*3 + w*3 + 2];
    return p;
}

void print_pixel(pixel_t *p)
{
    std::cerr << p->b << ", " << p->g << ", " << p->r << std::endl;
}

void im_print(image_t *im)
{
    for (int i = 0; i < im->h; ++i)
    {
        for (int j = 0; j < im->w; ++j)
        {
            if (j != 0) std::cerr << ", ";
            pixel_t p = pixel(im, i, j);
            std::cerr << "[" << p.r << "," << p.g << "," << p.g << "]";
        }
        std::cerr << std::endl;
    }
}

void im_scale_factor(image_t *src, image_t *dest, float h_factor, float w_factor)
{
    for (int i = 0; i < dest->h; ++i) {
        float fY = ((float) i) / h_factor;
        int iY = (int) fY;

        for (int j = 0; j < dest->w; ++j) {
            float fX = ((float) j) / w_factor;
            int iX = (int) fX;

            pixel_t q11 = pixel(src, iY, iX);
            pixel_t q12 = pixel(src, iY + 1, iX);
            pixel_t q21 = pixel(src, iY, iX + 1);
            pixel_t q22 = pixel(src, iY + 1, iX + 1);

            unsigned char r1_b = (iX + 1 - fX)/(iX + 1 - iX)*q11.b + (fX - iX)/(iX + 1 - iX)*q21.b;
            unsigned char r1_g = (iX + 1 - fX)/(iX + 1 - iX)*q11.g + (fX - iX)/(iX + 1 - iX)*q21.g;
            unsigned char r1_r = (iX + 1 - fX)/(iX + 1 - iX)*q11.r + (fX - iX)/(iX + 1 - iX)*q21.r;

            unsigned char r2_b = (iX + 1 - fX)/(iX + 1 - iX)*q12.b + (fX - iX)/(iX + 1 - iX)*q22.b;
            unsigned char r2_g = (iX + 1 - fX)/(iX + 1 - iX)*q12.g + (fX - iX)/(iX + 1 - iX)*q22.g;
            unsigned char r2_r = (iX + 1 - fX)/(iX + 1 - iX)*q12.r + (fX - iX)/(iX + 1 - iX)*q22.r;

            unsigned char b = (iY + 1 - fY)/(iY + 1 - iY)*r1_b + (fY - iY)/(iY + 1 - iY)*r2_b;
            unsigned char g = (iY + 1 - fY)/(iY + 1 - iY)*r1_g + (fY - iY)/(iY + 1 - iY)*r2_g;
            unsigned char r = (iY + 1 - fY)/(iY + 1 - iY)*r1_r + (fY - iY)/(iY + 1 - iY)*r2_r;

            dest->pixels[i*dest->w*3 + j*3 + 0] = b;
            dest->pixels[i*dest->w*3 + j*3 + 1] = g;
            dest->pixels[i*dest->w*3 + j*3 + 2] = r;
        }
    }
}

void im_scale(image_t *src, image_t *dest, int h, int w)
{
    std::cerr << "scale to : " << w << " x " << h << "\n";

    dest->h = h;
    dest->w = w;
    dest->pixels = (unsigned char*)malloc(h * w * 3);

    float scale_x = w / (float)src->w;
    float scale_y = h / (float)src->h;

    im_scale_factor(src, dest, scale_y, scale_x);
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " in out" << std::endl;
        return -1;
    }

    image_t image;
    if (!read_jpg(argv[1], &image))
    {
        std::cerr << "cannot open file: " << argv[1] << std::endl;
        return -1;
    }

    image_t result;
    //im_scale(&image, &result, 300, 200);
    im_scale(&image, &result, 1280, 720);
   
    std::cerr << "write image: " << result.w << "x" << result.h << " pixels" << std::endl;
    write_jpg(argv[2], &result);
    return 0;
}

