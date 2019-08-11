#pragma once
#include <cstring>
#include <string>

namespace Progression
{

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
    bool Deserialize2( char*& buffer );

    int Width() const;
    int Height() const;
    int NumComponents() const;
    unsigned char* Pixels() const;

protected:
    int m_width             = 0;
    int m_height            = 0;
    int m_numComponents     = 0;
    unsigned char* m_pixels = nullptr;
};

} // namespace Progression
