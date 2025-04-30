#include <img_lib.h>
#include <jpeg_image.h>
#include <ppm_image.h>
#include <bmp_image.h>

#include <filesystem>
#include <string_view>
#include <iostream>

using namespace std;

// Определение перечисления Format
enum class Format {
    UNKNOWN,
    PPM,
    JPEG, 
    BMP
};

// Класс ImageFormatInterface
class ImageFormatInterface {
public:

    virtual bool SaveImage(const img_lib::Path& file, const img_lib::Image& image) const = 0;
    virtual img_lib::Image LoadImage(const img_lib::Path& file) const = 0;
};

namespace FormatInterfaces {
    class PPM : public ImageFormatInterface {
    public:
        bool SaveImage(const img_lib::Path& file, const img_lib::Image& image) const override {
            return img_lib::SavePPM(file, image);
        }
        img_lib::Image LoadImage(const img_lib::Path& file) const override {
            return img_lib::LoadPPM(file);
        }
    };

    class JPEG : public ImageFormatInterface {
    public:
        bool SaveImage(const img_lib::Path& file, const img_lib::Image& image) const override {
            return img_lib::SaveJPEG(file, image);
        }
        img_lib::Image LoadImage(const img_lib::Path& file) const override {
            return img_lib::LoadJPEG(file);
        }
    };

    class BMP : public ImageFormatInterface {
    public:
        bool SaveImage(const img_lib::Path& file, const img_lib::Image& image) const override {
            return img_lib::SaveBMP(file, image);
        }

        img_lib::Image LoadImage(const img_lib::Path& file) const override {
            return img_lib::LoadBMP(file);
        }
    };

} // namespace FormatInterfaces

Format GetFormatByExtension(const img_lib::Path& input_file) {
    const string ext = input_file.extension().string();
    if (ext == ".jpg"sv || ext == ".jpeg"sv) {
        return Format::JPEG;
    }

    if (ext == ".ppm"sv) {
        return Format::PPM;
    }

    if (ext == ".bmp"sv) {
        return Format::BMP;
    }

    return Format::UNKNOWN;
}

ImageFormatInterface* GetFormatInterface(const img_lib::Path& path) {
    static const FormatInterfaces::PPM ppmInterface;
    static const FormatInterfaces::JPEG jpegInterface;
    static const FormatInterfaces::BMP bmpInterface;

    switch (GetFormatByExtension(path)) {
    case Format::PPM:
        return const_cast<FormatInterfaces::PPM*>(&ppmInterface);
    case Format::JPEG:
        return const_cast<FormatInterfaces::JPEG*>(&jpegInterface);
    case Format::BMP:
        return const_cast<FormatInterfaces::BMP*>(&bmpInterface);
    default:
        return nullptr;
    }
}

int main(int argc, const char** argv) {
    if (argc != 3) {
        cerr << "Usage: "sv << argv[0] << " <in_file> <out_file>"sv << endl;
        return 1;
    }

    img_lib::Path in_path = argv[1];
    img_lib::Path out_path = argv[2];

    ImageFormatInterface* input_format = GetFormatInterface(in_path);
    if (!input_format) {
        cerr << "Unknown format of the input file" << endl;
        return 2;
    }

    ImageFormatInterface* output_format = GetFormatInterface(out_path);
    if (!output_format) {
        cerr << "Unknown format of the output file" << endl;
        return 3;
    }

    img_lib::Image image = input_format->LoadImage(in_path);
    if (!image) {
        cerr << "Loading failed " << in_path << endl;
        return 4;
    }

    if (!output_format->SaveImage(out_path, image)) {
        cerr << "Saving failed " << out_path << endl;
        return 5;
    }

    cout << "Successfully converted"sv << endl;
}