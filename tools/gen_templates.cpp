// Generates placeholder template images for Mac memes
// Compile: g++ -std=c++17 -I../include tools/gen_templates.cpp -o gen_templates && ./gen_templates

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"
#include <vector>
#include <cstring>

void fillRect(unsigned char* img, int imgW, int imgH, int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b) {
    for (int row = y; row < y + h && row < imgH; row++) {
        for (int col = x; col < x + w && col < imgW; col++) {
            int idx = (row * imgW + col) * 4;
            img[idx] = r; img[idx+1] = g; img[idx+2] = b; img[idx+3] = 255;
        }
    }
}

void createTwoPanel(const char* path) {
    int w = 600, h = 600;
    std::vector<unsigned char> img(w * h * 4);
    // Top panel - light red
    fillRect(img.data(), w, h, 0, 0, w, h/2, 200, 100, 100);
    // Bottom panel - light green
    fillRect(img.data(), w, h, 0, h/2, w, h/2, 100, 200, 100);
    // Divider
    fillRect(img.data(), w, h, 0, h/2-2, w, 4, 40, 40, 40);
    stbi_write_png(path, w, h, 4, img.data(), w * 4);
}

void createThreePanel(const char* path) {
    int w = 800, h = 500;
    std::vector<unsigned char> img(w * h * 4);
    fillRect(img.data(), w, h, 0, 0, w/3, h, 180, 140, 200);
    fillRect(img.data(), w, h, w/3, 0, w/3, h, 200, 180, 140);
    fillRect(img.data(), w, h, 2*w/3, 0, w/3, h, 140, 200, 180);
    // Dividers
    fillRect(img.data(), w, h, w/3-2, 0, 4, h, 40, 40, 40);
    fillRect(img.data(), w, h, 2*w/3-2, 0, 4, h, 40, 40, 40);
    stbi_write_png(path, w, h, 4, img.data(), w * 4);
}

void createBottomText(const char* path) {
    int w = 600, h = 400;
    std::vector<unsigned char> img(w * h * 4);
    fillRect(img.data(), w, h, 0, 0, w, h, 100, 150, 200);
    // Bottom text area
    fillRect(img.data(), w, h, 0, h*2/3, w, h/3, 240, 240, 240);
    fillRect(img.data(), w, h, 0, h*2/3-2, w, 4, 40, 40, 40);
    stbi_write_png(path, w, h, 4, img.data(), w * 4);
}

void createBlank(const char* path) {
    int w = 600, h = 600;
    std::vector<unsigned char> img(w * h * 4);
    fillRect(img.data(), w, h, 0, 0, w, h, 255, 255, 255);
    stbi_write_png(path, w, h, 4, img.data(), w * 4);
}

int main() {
    createTwoPanel("../assets/templates/two_panel.png");
    createThreePanel("../assets/templates/three_panel.png");
    createBottomText("../assets/templates/bottom_text.png");
    createBlank("../assets/templates/blank.png");
    printf("Generated 4 template images.\n");
    return 0;
}
