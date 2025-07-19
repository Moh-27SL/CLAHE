#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#define byte win_byte_override
#include <windows.h>
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../header/stb_image.h"
#include "../header/stb_image_write.h"
#include "../header/png.h"

PngImage load_png(const std::string& filename) {
    PngImage img;
    img.data = stbi_load(filename.c_str(), &img.width, &img.height, &img.channels, 0);
    if (!img.data) {
        std::cerr << "Failed to load PNG: " << stbi_failure_reason() << "\n";

        img.data = nullptr;
        img.height = img.width = img.channels = 0;
    }
    return img;
}

bool save_png(const std::string& filename, const PngImage& img) {
    int success = stbi_write_png(filename.c_str(), img.width, img.height, img.channels, img.data, img.width * img.channels);
    return success != 0;
}
