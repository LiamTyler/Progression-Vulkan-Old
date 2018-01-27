#ifndef SRC_INCLUDE_IMAGE_H_
#define SRC_INCLUDE_IMAGE_H_

#include <string>
#include "glm/glm.hpp"

using glm::vec4;
using glm::vec3;

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

        void LoadImage(const std::string& filename);
        void SaveImage(const std::string& filename);

        int Width() { return width_; }
        int Height() { return height_; }
        vec4 GetPixel(int r, int c) { return pixels_[r*width_ + c]; }
        void SetPixel(int r, int c, vec4 p) { pixels_[r*width_ + c] = p; }

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
        // Image* Crop(int x, int y, int w, int h) {}
        // Image* Rotate(float angle) {}
        void ExtractChannel(int channel) {}
        void Quantize(int nbits) {}
        void RandomDither(int nbits) {}
        void OrdereredDither(int nbits) {}
        void FloydSteinbergDither(int nbits) {}

        static vec4 ClampPixel(vec4 p);
        static float ClampFloat(float f);
        static float Luminance(vec4 p);

    protected:
        // vec4 Sample(float u, float v) {}
        Sampling sampling_method_;
        int width_;
        int height_;
        glm::vec4* pixels_;
};

#endif  // SRC_INCLUDE_IMAGE_H_
