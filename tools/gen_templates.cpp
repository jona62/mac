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
    fillRect(img.data(), w, h, 0, 0, w, h/2, 58, 58, 58);
    fillRect(img.data(), w, h, 0, h/2, w, h/2, 69, 69, 69);
    fillRect(img.data(), w, h, 0, h/2-2, w, 4, 32, 32, 32);
    stbi_write_png(path, w, h, 4, img.data(), w * 4);
}

void createThreePanel(const char* path) {
    int w = 800, h = 500;
    std::vector<unsigned char> img(w * h * 4);
    fillRect(img.data(), w, h, 0, 0, w/3, h, 56, 56, 56);
    fillRect(img.data(), w, h, w/3, 0, w/3, h, 72, 72, 72);
    fillRect(img.data(), w, h, 2*w/3, 0, w/3, h, 56, 56, 56);
    fillRect(img.data(), w, h, w/3-2, 0, 4, h, 32, 32, 32);
    fillRect(img.data(), w, h, 2*w/3-2, 0, 4, h, 32, 32, 32);
    stbi_write_png(path, w, h, 4, img.data(), w * 4);
}

void createBottomText(const char* path) {
    int w = 600, h = 400;
    std::vector<unsigned char> img(w * h * 4);
    fillRect(img.data(), w, h, 0, 0, w, h, 53, 53, 53);
    fillRect(img.data(), w, h, 0, h*2/3, w, h/3, 240, 240, 240);
    fillRect(img.data(), w, h, 0, h*2/3-2, w, 4, 32, 32, 32);
    stbi_write_png(path, w, h, 4, img.data(), w * 4);
}

void createBlank(const char* path) {
    int w = 600, h = 600;
    std::vector<unsigned char> img(w * h * 4, 0); // fully transparent
    stbi_write_png(path, w, h, 4, img.data(), w * 4);
}

void createCaptionBar(const char* path) {
    int w = 600, h = 500;
    std::vector<unsigned char> img(w * h * 4);
    fillRect(img.data(), w, h, 0, 0, w, h * 7 / 10, 64, 64, 64);
    fillRect(img.data(), w, h, 0, h * 7 / 10, w, h * 3 / 10, 240, 240, 240);
    fillRect(img.data(), w, h, 0, h * 7 / 10 - 2, w, 4, 32, 32, 32);
    stbi_write_png(path, w, h, 4, img.data(), w * 4);
}

void createFourPanel(const char* path) {
    int w = 600, h = 600;
    int half = w / 2;
    std::vector<unsigned char> img(w * h * 4);
    fillRect(img.data(), w, h, 0, 0, half, half, 58, 58, 58);
    fillRect(img.data(), w, h, half, 0, half, half, 72, 72, 72);
    fillRect(img.data(), w, h, 0, half, half, half, 72, 72, 72);
    fillRect(img.data(), w, h, half, half, half, half, 58, 58, 58);
    // Dividers
    fillRect(img.data(), w, h, half - 2, 0, 4, h, 32, 32, 32);
    fillRect(img.data(), w, h, 0, half - 2, w, 4, 32, 32, 32);
    stbi_write_png(path, w, h, 4, img.data(), w * 4);
}

void createWide(const char* path) {
    int w = 1200, h = 675; // 16:9
    std::vector<unsigned char> img(w * h * 4);
    fillRect(img.data(), w, h, 0, 0, w, h/2, 50, 50, 55);
    fillRect(img.data(), w, h, 0, h/2, w, h/2, 62, 62, 68);
    fillRect(img.data(), w, h, 0, h/2-2, w, 4, 30, 30, 35);
    stbi_write_png(path, w, h, 4, img.data(), w * 4);
}

void createTall(const char* path) {
    int w = 675, h = 1200; // 9:16 (stories/reels)
    std::vector<unsigned char> img(w * h * 4);
    fillRect(img.data(), w, h, 0, 0, w, h/3, 50, 50, 55);
    fillRect(img.data(), w, h, 0, h/3, w, h/3, 62, 62, 68);
    fillRect(img.data(), w, h, 0, 2*h/3, w, h/3, 50, 50, 55);
    fillRect(img.data(), w, h, 0, h/3-2, w, 4, 30, 30, 35);
    fillRect(img.data(), w, h, 0, 2*h/3-2, w, 4, 30, 30, 35);
    stbi_write_png(path, w, h, 4, img.data(), w * 4);
}

void createSquare(const char* path) {
    int w = 800, h = 800; // 1:1 (Instagram)
    std::vector<unsigned char> img(w * h * 4);
    fillRect(img.data(), w, h, 0, 0, w, h/2, 45, 45, 50);
    fillRect(img.data(), w, h, 0, h/2, w, h/2, 58, 58, 63);
    fillRect(img.data(), w, h, 0, h/2-2, w, 4, 28, 28, 33);
    stbi_write_png(path, w, h, 4, img.data(), w * 4);
}

void createDark(const char* path) {
    int w = 600, h = 600;
    std::vector<unsigned char> img(w * h * 4);
    fillRect(img.data(), w, h, 0, 0, w, h, 18, 18, 22);
    stbi_write_png(path, w, h, 4, img.data(), w * 4);
}

int main() {
    createTwoPanel("../assets/templates/two_panel.png");
    createThreePanel("../assets/templates/three_panel.png");
    createBottomText("../assets/templates/bottom_text.png");
    createBlank("../assets/templates/blank.png");
    createCaptionBar("../assets/templates/caption_bar.png");
    createFourPanel("../assets/templates/four_panel.png");
    createWide("../assets/templates/wide.png");
    createTall("../assets/templates/tall.png");
    createSquare("../assets/templates/square.png");
    createDark("../assets/templates/dark.png");
    printf("Generated 10 template images.\n");
    return 0;
}
