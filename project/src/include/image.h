#pragma once

#include "include/utils.h"

enum class Sampling : unsigned int {
    NEAREST_NEIGHBOR,
    BILINEAR,
    GAUSSIAN,
};

class Image {
    public:
        Image();
        Image(int w, int h);
        Image(const std::string filename);
        Image(const Image& src);
        ~Image();

        bool LoadImage(const std::string& filename);
        void SaveImage(const std::string& filename);

        int Width() const { return width_; }
        int Height() const { return height_; }
        glm::vec4 GetPixel(int r, int c) const { return pixels_[r*width_ + c]; }
        void SetPixel(int r, int c, glm::vec4 p) { pixels_[r*width_ + c] = p; }
        glm::vec4* GetData() const { return pixels_; }

        // Image processing features
        
        // factor = [0, 1]. 0 = original, 1 = total noise
        void AddNoise(float factor);
        // factor = [0, inf]
        void Brighten(float factor);
        // factor = [0, inf]. 0 = no contrast
        void Contrast(float factor);
        // factor = [-inf, inf]. < 0 = inverted colors,
        //   0 = grayscale, > 1 = saturated
        void Saturate(float factor);
        void Blur(int n) {}
        void Sharpen(int n) {}
        void EdgeDetect() {}
        // Image* Scale(float sx, float sy) {}
        Image* Crop(int r, int c, int w, int h);
        // Image* Rotate(float angle) {}
        void ExtractChannel(int channel) {}
        void Quantize(int nbits) {}
        void RandomDither(int nbits) {}
        void OrdereredDither(int nbits) {}
        void FloydSteinbergDither(int nbits) {}

        static glm::vec4 ClampPixel(glm::vec4 p);
        static float ClampFloat(float f);
        static float Luminance(glm::vec4 p);

    protected:
        // vec4 Sample(float u, float v) {}
        Sampling sampling_method_;
        int width_;
        int height_;
        glm::vec4* pixels_;
};