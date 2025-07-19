#ifndef PNG_H_INCLUDED
#define PNG_H_INCLUDED

struct PngImage {
    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* data = nullptr;
};

PngImage load_png(const std::string& filename);
bool save_png(const std::string& filename, const PngImage& img);

#endif // PNG_H_INCLUDED
