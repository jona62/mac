#ifndef MEME_LAYOUT_H
#define MEME_LAYOUT_H

#include <algorithm>            // min, max
#include <cstring>              // memcpy
#include <string>               // string
#include <vector>               // vector (pixel buffers)

namespace layout {

    // Resize an RGBA buffer using nearest-neighbor
    inline std::vector<unsigned char> resizePixels(const unsigned char* src,
                                                     int srcW, int srcH,
                                                     int dstW, int dstH) {
        std::vector<unsigned char> dst(dstW * dstH * 4);
        for (int y = 0; y < dstH; y++) {
            int srcY = y * srcH / dstH;
            for (int x = 0; x < dstW; x++) {
                int srcX = x * srcW / dstW;
                int si = (srcY * srcW + srcX) * 4;
                int di = (y * dstW + x) * 4;
                dst[di + 0] = src[si + 0];
                dst[di + 1] = src[si + 1];
                dst[di + 2] = src[si + 2];
                dst[di + 3] = src[si + 3];
            }
        }
        return dst;
    }

    // Place side-by-side, scaling to match height of the taller image
    inline std::vector<unsigned char> composeBeside(
            const unsigned char* buf1, int w1, int h1,
            const unsigned char* buf2, int w2, int h2,
            int& outW, int& outH) {
        // Match heights to the taller image
        int targetH = std::max(h1, h2);
        int newW1 = w1, newW2 = w2;

        std::vector<unsigned char> scaled1, scaled2;
        const unsigned char* p1 = buf1;
        const unsigned char* p2 = buf2;

        if (h1 != targetH) {
            float scale = static_cast<float>(targetH) / h1;
            newW1 = static_cast<int>(w1 * scale);
            scaled1 = resizePixels(buf1, w1, h1, newW1, targetH);
            p1 = scaled1.data();
        }
        if (h2 != targetH) {
            float scale = static_cast<float>(targetH) / h2;
            newW2 = static_cast<int>(w2 * scale);
            scaled2 = resizePixels(buf2, w2, h2, newW2, targetH);
            p2 = scaled2.data();
        }

        outW = newW1 + newW2;
        outH = targetH;
        std::vector<unsigned char> result(outW * outH * 4);

        for (int y = 0; y < outH; y++) {
            // Copy left image row
            std::memcpy(&result[(y * outW) * 4],
                       &p1[(y * newW1) * 4],
                       newW1 * 4);
            // Copy right image row
            std::memcpy(&result[(y * outW + newW1) * 4],
                       &p2[(y * newW2) * 4],
                       newW2 * 4);
        }

        return result;
    }

    // Stack vertically, scaling to match width of the wider image
    inline std::vector<unsigned char> composeStack(
            const unsigned char* buf1, int w1, int h1,
            const unsigned char* buf2, int w2, int h2,
            int& outW, int& outH) {
        int targetW = std::max(w1, w2);
        int newH1 = h1, newH2 = h2;

        std::vector<unsigned char> scaled1, scaled2;
        const unsigned char* p1 = buf1;
        const unsigned char* p2 = buf2;

        if (w1 != targetW) {
            float scale = static_cast<float>(targetW) / w1;
            newH1 = static_cast<int>(h1 * scale);
            scaled1 = resizePixels(buf1, w1, h1, targetW, newH1);
            p1 = scaled1.data();
        }
        if (w2 != targetW) {
            float scale = static_cast<float>(targetW) / w2;
            newH2 = static_cast<int>(h2 * scale);
            scaled2 = resizePixels(buf2, w2, h2, targetW, newH2);
            p2 = scaled2.data();
        }

        outW = targetW;
        outH = newH1 + newH2;
        std::vector<unsigned char> result(outW * outH * 4);

        // Copy top image
        std::memcpy(result.data(), p1, targetW * newH1 * 4);
        // Copy bottom image
        std::memcpy(&result[targetW * newH1 * 4], p2, targetW * newH2 * 4);

        return result;
    }

    // Add padding (white) around the image
    inline std::vector<unsigned char> addPadding(
            const unsigned char* buf, int w, int h,
            int padPx,
            int& outW, int& outH) {
        outW = w + padPx * 2;
        outH = h + padPx * 2;
        std::vector<unsigned char> result(outW * outH * 4, 255); // white fill

        for (int y = 0; y < h; y++) {
            std::memcpy(&result[((y + padPx) * outW + padPx) * 4],
                       &buf[(y * w) * 4],
                       w * 4);
        }
        return result;
    }

    // Add border (black) around the image
    inline std::vector<unsigned char> addBorder(
            const unsigned char* buf, int w, int h,
            int borderPx,
            int& outW, int& outH) {
        outW = w + borderPx * 2;
        outH = h + borderPx * 2;
        // Fill with black (but alpha = 255)
        std::vector<unsigned char> result(outW * outH * 4, 0);
        for (int i = 3; i < outW * outH * 4; i += 4) {
            result[i] = 255;
        }

        for (int y = 0; y < h; y++) {
            std::memcpy(&result[((y + borderPx) * outW + borderPx) * 4],
                       &buf[(y * w) * 4],
                       w * 4);
        }
        return result;
    }

    // Grid layout: arrange images in cols x rows grid
    inline std::vector<unsigned char> composeGrid(
            const std::vector<std::pair<const unsigned char*, std::pair<int, int>>>& images,
            int cols,
            int& outW, int& outH) {
        if (images.empty() || cols <= 0) {
            outW = outH = 0;
            return {};
        }

        int rows = (static_cast<int>(images.size()) + cols - 1) / cols;

        // Find max cell size
        int cellW = 0, cellH = 0;
        for (auto& img : images) {
            cellW = std::max(cellW, img.second.first);
            cellH = std::max(cellH, img.second.second);
        }

        outW = cellW * cols;
        outH = cellH * rows;
        std::vector<unsigned char> result(outW * outH * 4, 255); // white background
        // Set alpha to 255
        for (int i = 3; i < outW * outH * 4; i += 4) {
            result[i] = 255;
        }

        for (size_t i = 0; i < images.size(); i++) {
            int row = static_cast<int>(i) / cols;
            int col = static_cast<int>(i) % cols;
            int imgW = images[i].second.first;
            int imgH = images[i].second.second;

            // Resize to cell if needed
            std::vector<unsigned char> scaled;
            const unsigned char* src = images[i].first;
            if (imgW != cellW || imgH != cellH) {
                scaled = resizePixels(src, imgW, imgH, cellW, cellH);
                src = scaled.data();
            }

            int offsetX = col * cellW;
            int offsetY = row * cellH;
            for (int y = 0; y < cellH; y++) {
                std::memcpy(&result[((offsetY + y) * outW + offsetX) * 4],
                           &src[(y * cellW) * 4],
                           cellW * 4);
            }
        }

        return result;
    }

} // namespace layout

#endif // MEME_LAYOUT_H
