#pragma once
#include <cstring>
#include <string>

namespace Progression
{

class Pixel
{
public:
    Pixel();
    Pixel( unsigned char r, unsigned char g, unsigned char b, unsigned char a );

    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
};

class Image
{
public:
    Image() = default;
    Image( int w, int h, int nc, unsigned char* pixels = nullptr );
    ~Image();

    Image( const Image& src );
    Image& operator=( const Image& image );
    Image( Image&& src );
    Image& operator=( Image&& src );

    bool Load( const std::string& fname, bool flipVertically = true );
    bool Save( const std::string& fname, bool flipVertically = true ) const;

    // to / from  a binary file
    bool Serialize( std::ofstream& outFile ) const;
    bool Deserialize( std::ifstream& in );

    int Width() const;
    int Height() const;
    int NumComponents() const;
    unsigned char* Pixels() const;
    Pixel GetPixel( int r, int c ) const;
    void SetPixel( int r, int c, const Pixel& p );

protected:
    int m_width             = 0;
    int m_height            = 0;
    int m_numComponents     = 0;
    unsigned char* m_pixels = nullptr;
};

} // namespace Progression
