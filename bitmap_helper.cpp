#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "bitmap_helper.h"

using namespace std;

// Función para cargar un bitmap (equivalente a LoadBitmap en C#)
tuple<vector<int>, int, int> LoadBitmap(const string& filename) {
    int width, height, channels;
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &channels, 4); // Forzamos a 4 canales (BGRA)

    if (!data) {
        throw runtime_error("Failed to load image: " + filename);
    }

    vector<int> result(width * height);
    for (int i = 0; i < width * height; i++) {
        result[i] = (data[i * 4 + 3] << 24) | (data[i * 4 + 0] << 16) | (data[i * 4 + 1] << 8) | data[i * 4 + 2];
    }

    stbi_image_free(data);
    return make_tuple(result, width, height);
}

// Función para guardar un bitmap (equivalente a SaveBitmap en C#)
void SaveBitmap(const vector<int>& data, int width, int height, const string& filename) {
    vector<unsigned char> imageData(width * height * 4); // 4 canales (BGRA)
    for (int i = 0; i < width * height; i++) {
        imageData[i * 4 + 0] = (data[i] >> 16) & 0xFF; // Red
        imageData[i * 4 + 1] = (data[i] >> 8) & 0xFF;  // Green
        imageData[i * 4 + 2] = data[i] & 0xFF;         // Blue
        imageData[i * 4 + 3] = (data[i] >> 24) & 0xFF; // Alpha
    }

    if (!stbi_write_png(filename.c_str(), width, height, 4, imageData.data(), width * 4)) {
        throw runtime_error("Failed to save image: " + filename);
    }
}