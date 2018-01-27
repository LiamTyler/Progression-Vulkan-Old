#define STB_IMAGE_IMPLEMENTATION
#include "include/stb_image.h"
#include "include/image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "include/stb_image_write.h"

#include <iostream>
#include <iomanip>

Image::Image() {
    sampling_method_ = Sampling::NEAREST_NEIGHBOR;
    width_ = 0;
    height_ = 0;
    pixels_ = nullptr;
}

Image::Image(int w, int h) {
    sampling_method_ = Sampling::NEAREST_NEIGHBOR;
    width_ = w;
    height_ = h;
    pixels_ = new vec4[w*h];
    for (int i = 0; i < w*h; i++)
        pixels_[i] = vec4(0,0,0,0);
}

Image::Image(const std::string fname) {
    sampling_method_ = Sampling::NEAREST_NEIGHBOR;
    LoadImage(fname);
}

Image::Image(const Image& src) {
    width_ = src.width_;
    height_ = src.height_;
    pixels_ = new vec4[width_*height_];
    for (int i = 0; i < width_*height_; i++)
        pixels_[i] = src.pixels_[i];
}

Image::~Image() {
    if (pixels_)
        delete [] pixels_;
}

void Image::LoadImage(const std::string& fname) {
    int nC;
    unsigned char *raw = stbi_load(fname.c_str(), &width_, &height_, &nC, 4);
    if (raw == NULL) {
        std::cout << "Failed to load : " << fname << std::endl;
    } else {
        pixels_ = new vec4[width_*height_];
        int I = 0;
        for (int r = 0; r < height_; r++) {
            for (int c = 0; c < width_; c++) {
                float rr = raw[I++] / 255.0f;
                float gg = raw[I++] / 255.0f;
                float bb = raw[I++] / 255.0f;
                float aa = raw[I++] / 255.0f;
                SetPixel(r, c, vec4(rr, gg, bb, aa));
            }
        }
    }
}

void Image::SaveImage(const std::string& fname) {
    unsigned char* out = new unsigned char[width_*height_*4];
    int b = 0;
    for (int r = 0; r < height_; r++) {
        for (int c = 0; c < width_; c++) {
            vec4 p = GetPixel(r,c);
            p = ClampPixel(p);
            out[b++] = (unsigned char) 255*p.r;
            out[b++] = (unsigned char) 255*p.g;
            out[b++] = (unsigned char) 255*p.b;
            out[b++] = (unsigned char) 255*p.a;
        }
    }


    int i = fname.length();
    while (fname[--i] != '.' && i >= 0);
    if (i < 0) {
        std::cout << "Invalid image file name: " << fname << std::endl;
        delete [] out;
        return;
    }

    switch(fname[i+1]) {
        case 'p':
            stbi_write_png(fname.c_str(), width_, height_, 4, out, width_*4);
            break;
        case 'j':
            stbi_write_jpg(fname.c_str(), width_, height_, 4, out, 95);
            break;
        case 'b':
            stbi_write_bmp(fname.c_str(), width_, height_, 4, out);
            break;
        case 't':
            stbi_write_tga(fname.c_str(), width_, height_, 4, out);
            break;
        default:
            std::cout << "Invalid image file name: " << fname << std::endl;
    }
    delete [] out;
}

void Image::AddNoise(float factor) {
    float rMaxInv = 1.0f / RAND_MAX;
    for (int r = 0; r < height_; ++r) {
        for (int c = 0; c < width_; ++c) {
            vec4 p = GetPixel(r, c);
            float rr = rand() * rMaxInv;
            float rg = rand() * rMaxInv;
            float rb = rand() * rMaxInv;
            vec4 newP = vec4(
                    (1 - factor) * p.r + factor * rr,
                    (1 - factor) * p.g + factor * rg,
                    (1 - factor) * p.b + factor * rb,
                    p.w);
            SetPixel(r, c, newP);
        }
    }
}

void Image::Brighten(float factor) {
    for (int r = 0; r < height_; ++r) {
        for (int c = 0; c < width_; ++c) {
            vec4 p = GetPixel(r, c);
            float nr = p.r*factor;
            float ng = p.g*factor;
            float nb = p.b*factor;
            SetPixel(r, c, vec4(nr, ng, nb, p.a));
        }
    }
}

void Image::Contrast(float factor) {
    for (int r = 0; r < height_; ++r) {
        for (int c = 0; c < width_; ++c) {
            vec4 p = GetPixel(r, c);
            float nr = .5 + factor*(p.r - .5);
            float ng = .5 + factor*(p.g - .5);
            float nb = .5 + factor*(p.b - .5);
            SetPixel(r, c, vec4(nr, ng, nb, p.a));
        }
    }
}

void Image::Saturate(float factor) {
    for (int r = 0; r < height_; ++r) {
        for (int c = 0; c < width_; ++c) {
            vec4 p = GetPixel(r, c);
            float L = Luminance(p);
            float nr = (1 - factor) * L + factor * p.r;
            float ng = (1 - factor) * L + factor * p.g;
            float nb = (1 - factor) * L + factor * p.b;
            SetPixel(r, c, vec4(nr, ng, nb, p.a));
        }
    }
}

vec4 Image::ClampPixel(vec4 p) {
    return vec4(
            ClampFloat(p.r),
            ClampFloat(p.g),
            ClampFloat(p.b),
            ClampFloat(p.a));
}

float Image::ClampFloat(float f) {
    return std::fmax(0.0f, std::fmin(1.0f, f));
}

float Image::Luminance(vec4 p) {
    return .2126f * p.r + .7152f * p.g + .0722f * p.b;
}
