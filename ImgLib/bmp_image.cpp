#include "bmp_image.h"
#include "pack_defines.h"

#include <array>
#include <fstream>
#include <string_view>

using namespace std;

namespace img_lib {

// деление, а затем умножение на 4 округляет до числа, кратного четырём
// прибавление тройки гарантирует, что округление будет вверх
int GetBMPStride(int w) {
    return 4 * ((w * 3 + 3) / 4);
} 

PACKED_STRUCT_BEGIN BitmapFileHeader {
    BitmapFileHeader(int width, int height) {
        bfSize = bfOffBits + GetBMPStride(width) * height;      //размер заголовка + размер данных
    }
    char bfType[2] = {'B', 'M'};    //2
    uint32_t bfSize;                //4
    uint32_t bfReserved = 0;        //4
    uint32_t bfOffBits = 54;        //4     //14+40
}
PACKED_STRUCT_END

PACKED_STRUCT_BEGIN BitmapInfoHeader {
    BitmapInfoHeader(int width, int height)
        : biWidth(width), biHeight(height) {
        biSizeImage = GetBMPStride(width) * height;
    }
    uint32_t biSize = 40;               //4
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
            buff[x * 3 + 0] = static_cast<char>(line[x].b);
            buff[x * 3 + 1] = static_cast<char>(line[x].g);
            buff[x * 3 + 2] = static_cast<char>(line[x].r);
        }
        ofs.write(buff.data(), buff.size());
    }

    return ofs.good();
}

Image LoadBMP(const Path& file) {
    ifstream ifs(file, ios::binary);
    ifs.ignore(18); // игнорируем 18 байт (14 - BitmapFileHeader и 4 - BitmapInfoHeader.biSize)

    int w, h;
    ifs.read(reinterpret_cast<char*>(&w), sizeof(w));
    ifs.read(reinterpret_cast<char*>(&h), sizeof(h));

    ifs.ignore(28); // пропускаем оставшуюся служебную информацию

    int stride = GetBMPStride(w);
    Image result(stride / 3, h, Color::Black());
    
    std::vector<char> buff(w * 3);
    for (int y = result.GetHeight() - 1; y >= 0; --y) {
        Color* line = result.GetLine(y);
        ifs.read(buff.data(), stride);

        for (int x = 0; x < w; ++x) {
            line[x].b = static_cast<byte>(buff[x * 3 + 0]);
            line[x].g = static_cast<byte>(buff[x * 3 + 1]);
            line[x].r = static_cast<byte>(buff[x * 3 + 2]);
        }
    }

    return result;
}

}  // namespace img_lib