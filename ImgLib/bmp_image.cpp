#include "bmp_image.h"
#include "pack_defines.h"

#include <array>
#include <fstream>
#include <stdexcept>
#include <string_view>

using namespace std;

namespace img_lib {

static const int BMP_BYTES_PER_PIXEL = 3;   //RGB
static const int BMP_PADDING_MULTIPLE = 4;

static const int BMP_HEADER_SIZE = 14;
static const int BMP_INFO_HEADER_SIZE = 40;

// деление, а затем умножение на 4 округляет до числа, кратного четырём
// прибавление тройки гарантирует, что округление будет вверх
int GetBMPStride(int w) {
    return BMP_PADDING_MULTIPLE * ((w * BMP_BYTES_PER_PIXEL + BMP_PADDING_MULTIPLE - 1) / BMP_PADDING_MULTIPLE);
} 

PACKED_STRUCT_BEGIN BitmapFileHeader {
    BitmapFileHeader() = default;
    BitmapFileHeader(int width, int height) {
        bfSize = bfOffBits + GetBMPStride(width) * height;      //размер заголовка + размер данных
    }
    char bfType[2] = {'B', 'M'};    //2
    uint32_t bfSize;                //4
    uint32_t bfReserved = 0;        //4
    uint32_t bfOffBits = BMP_HEADER_SIZE + BMP_INFO_HEADER_SIZE;        //4     //14+40
}
PACKED_STRUCT_END

PACKED_STRUCT_BEGIN BitmapInfoHeader {
    BitmapInfoHeader() = default;
    BitmapInfoHeader(int width, int height)
        : biWidth(width), biHeight(height) {
        biSizeImage = GetBMPStride(width) * height;
    }
    uint32_t biSize = BMP_INFO_HEADER_SIZE;               //4
    int32_t biWidth;                    //4
    int32_t biHeight;                   //4
    uint16_t biPlanes = 1;              //2
    uint16_t biBitCount = 24;           //2   // bpp
    uint32_t biCompression = 0;         //4
    uint32_t biSizeImage;               //4
    int32_t biXPelsPerMeter = 11811;    //4
    int32_t biYPelsPerMeter = 11811;    //4
    int32_t biClrUsed = 0;              //4
    int32_t biClrImportant = 0x1000000; //4
}
PACKED_STRUCT_END

bool SaveBMP(const Path& file, const Image& image) {
    int w = image.GetWidth();
    int h = image.GetHeight();

    BitmapFileHeader bitmap_file_header(w, h);
    BitmapInfoHeader bitmap_info_header(w, h);
    
    ofstream ofs(file, ios::binary);
    ofs.write(reinterpret_cast<const char*>(&bitmap_file_header), sizeof(bitmap_file_header));
    ofs.write(reinterpret_cast<const char*>(&bitmap_info_header), sizeof(bitmap_info_header));
    
    vector<char> buff(GetBMPStride(w));  
    for (int y = h - 1; y >= 0; --y) {
        const Color* line = image.GetLine(y);
        for (int x = 0; x < image.GetWidth(); ++x) {
            buff[x * BMP_BYTES_PER_PIXEL + 0] = static_cast<char>(line[x].b);
            buff[x * BMP_BYTES_PER_PIXEL + 1] = static_cast<char>(line[x].g);
            buff[x * BMP_BYTES_PER_PIXEL + 2] = static_cast<char>(line[x].r);
        }
        ofs.write(buff.data(), buff.size());
    }

    return ofs.good();
}

Image LoadBMP(const Path& file) {
    ifstream ifs(file, ios::binary);

    BitmapFileHeader bitmap_file_header;
    BitmapInfoHeader bitmap_info_header;

    ifs.read(reinterpret_cast<char*>(&bitmap_file_header), sizeof(bitmap_file_header));
    ifs.read(reinterpret_cast<char*>(&bitmap_info_header), sizeof(bitmap_info_header));
    if (bitmap_file_header.bfType[0] != 'B' || bitmap_file_header.bfType[1] != 'M') {
        throw runtime_error("incorrect file format");
    }
    if (bitmap_file_header.bfOffBits != BMP_HEADER_SIZE + BMP_INFO_HEADER_SIZE) {
        throw runtime_error("the file is corrupted");
    }
    if (bitmap_info_header.biSize != BMP_INFO_HEADER_SIZE || bitmap_info_header.biPlanes != 1 || bitmap_info_header.biBitCount != BMP_BYTES_PER_PIXEL*8) {
        throw runtime_error("the file is corrupted");
    }
    
    int w = bitmap_info_header.biWidth;
    int h = bitmap_info_header.biHeight;
    int stride = GetBMPStride(w);
    Image result(stride / BMP_BYTES_PER_PIXEL, h, Color::Black());
    
    std::vector<char> buff(w * BMP_BYTES_PER_PIXEL);
    for (int y = result.GetHeight() - 1; y >= 0; --y) {
        Color* line = result.GetLine(y);
        ifs.read(buff.data(), stride);

        for (int x = 0; x < w; ++x) {
            line[x].b = static_cast<byte>(buff[x * BMP_BYTES_PER_PIXEL + 0]);
            line[x].g = static_cast<byte>(buff[x * BMP_BYTES_PER_PIXEL + 1]);
            line[x].r = static_cast<byte>(buff[x * BMP_BYTES_PER_PIXEL + 2]);
        }
    }

    if(!ifs.good()) {
        throw runtime_error("in reading image");
    }

    return result;
}

}  // namespace img_lib