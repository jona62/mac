#ifndef MEME_RENDERER_H
#define MEME_RENDERER_H

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

// Do NOT define STB_*_IMPLEMENTATION here -- already in src/stb_impl.cpp
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"
#include "stb/stb_truetype.h"

namespace meme {

    // TextStyle is defined in MacMeme.h to avoid circular dependency

    class MemeRenderer {
    public:
        // Render meme to RGBA pixel buffer.
        // Sets outWidth / outHeight through reference params (via the overload below).
        static std::vector<unsigned char> render(const std::string& imagePath,
                                                  const std::string& topText,
                                                  const std::string& bottomText,
                                                  int targetWidth = 0,
                                                  int targetHeight = 0) {
            int w, h;
            return renderInternal(imagePath, topText, bottomText, targetWidth, targetHeight, w, h);
        }

        // Overload that also returns actual width / height
        static std::vector<unsigned char> render(const std::string& imagePath,
                                                  const std::string& topText,
                                                  const std::string& bottomText,
                                                  int targetWidth,
                                                  int targetHeight,
                                                  int& outWidth,
                                                  int& outHeight,
                                                  const TextStyle& style = TextStyle{}) {
            return renderInternal(imagePath, topText, bottomText, targetWidth, targetHeight, outWidth, outHeight, style);
        }

        // Save rendered RGBA pixels to a PNG or JPG file (detected by extension).
        static bool saveImage(const std::vector<unsigned char>& pixels,
                              int width, int height,
                              const std::string& outputPath) {
            std::string ext = toLower(getExtension(outputPath));
            if (ext == "jpg" || ext == "jpeg") {
                return stbi_write_jpg(outputPath.c_str(), width, height, 4,
                                      pixels.data(), 90) != 0;
            }
            // Default to PNG
            return stbi_write_png(outputPath.c_str(), width, height, 4,
                                  pixels.data(), width * 4) != 0;
        }

        // Assets directory — try binary-relative first, then cwd-relative
        static std::string getAssetsDir() {
            auto binDir = meme::MacMeme::binaryDir();
            auto binAssets = binDir + "/assets";
            if (std::filesystem::exists(binAssets)) return binAssets;
            return "assets";
        }

    private:
        // ---- internal render ----
        static std::vector<unsigned char> renderInternal(
                const std::string& imagePath,
                const std::string& topText,
                const std::string& bottomText,
                int targetWidth,
                int targetHeight,
                int& outWidth,
                int& outHeight,
                const TextStyle& activeStyle = TextStyle{}) {

            // Load template image
            int imgW, imgH, imgC;
            unsigned char* imgData = stbi_load(imagePath.c_str(), &imgW, &imgH, &imgC, 4);
            if (!imgData) {
                throw std::runtime_error("MemeRenderer: cannot load image '" + imagePath + "'");
            }

            int w = (targetWidth > 0) ? targetWidth : imgW;
            int h = (targetHeight > 0) ? targetHeight : imgH;

            // Build RGBA buffer -- resize to target by simple nearest-neighbor if needed
            std::vector<unsigned char> pixels(w * h * 4);
            for (int y = 0; y < h; ++y) {
                int srcY = y * imgH / h;
                for (int x = 0; x < w; ++x) {
                    int srcX = x * imgW / w;
                    int srcIdx = (srcY * imgW + srcX) * 4;
                    int dstIdx = (y * w + x) * 4;
                    pixels[dstIdx + 0] = imgData[srcIdx + 0];
                    pixels[dstIdx + 1] = imgData[srcIdx + 1];
                    pixels[dstIdx + 2] = imgData[srcIdx + 2];
                    pixels[dstIdx + 3] = imgData[srcIdx + 3];
                }
            }
            stbi_image_free(imgData);

            // Load font
            std::string fontPath = getAssetsDir() + "/fonts/meme-font.ttf";
            std::vector<unsigned char> fontData = readFile(fontPath);
            if (fontData.empty()) {
                throw std::runtime_error("MemeRenderer: cannot load font '" + fontPath + "'");
            }

            stbtt_fontinfo fontInfo;
            if (!stbtt_InitFont(&fontInfo, fontData.data(),
                                stbtt_GetFontOffsetForIndex(fontData.data(), 0))) {
                throw std::runtime_error("MemeRenderer: cannot init font");
            }

            // Draw top text (upper half)
            if (!topText.empty()) {
                int regionY = 0;
                int regionH = h / 2;
                drawMemeText(pixels, w, h, fontInfo, fontData, topText, regionY, regionH, activeStyle);
            }

            // Draw bottom text (lower half)
            if (!bottomText.empty()) {
                int regionY = h / 2;
                int regionH = h / 2;
                drawMemeText(pixels, w, h, fontInfo, fontData, bottomText, regionY, regionH, activeStyle);
            }

            outWidth = w;
            outHeight = h;
            return pixels;
        }

        // Word-wrap text to fit within maxWidth
        static std::vector<std::string> wrapText(stbtt_fontinfo& fontInfo,
                                                  const std::string& text,
                                                  float scale, float maxWidth) {
            std::vector<std::string> lines;
            std::vector<std::string> words;
            // Split on spaces
            std::string word;
            for (char c : text) {
                if (c == ' ') {
                    if (!word.empty()) { words.push_back(word); word.clear(); }
                } else {
                    word += c;
                }
            }
            if (!word.empty()) words.push_back(word);
            if (words.empty()) return lines;

            std::string line = words[0];
            for (size_t i = 1; i < words.size(); ++i) {
                std::string candidate = line + " " + words[i];
                if (measureText(fontInfo, candidate, scale) <= maxWidth) {
                    line = candidate;
                } else {
                    lines.push_back(line);
                    line = words[i];
                }
            }
            lines.push_back(line);
            return lines;
        }

        // Draw text centered in a horizontal strip of the image
        static void drawMemeText(std::vector<unsigned char>& pixels,
                                 int imgW, int imgH,
                                 stbtt_fontinfo& fontInfo,
                                 const std::vector<unsigned char>& fontData,
                                 const std::string& text,
                                 int regionY, int regionH,
                                 const TextStyle& activeStyle = TextStyle{}) {

            std::string upper = toUpper(text);
            float maxWidth = imgW * 0.9f;
            float fontSize = regionH * 0.7f;
            if (fontSize < 10.0f) fontSize = 10.0f;

            float scale = 0;
            std::vector<std::string> lines;

            // Auto-size: shrink until all wrapped lines fit width and block fits height
            while (fontSize >= 8.0f) {
                scale = stbtt_ScaleForPixelHeight(&fontInfo, fontSize);
                lines = wrapText(fontInfo, upper, scale, maxWidth);
                float lineHeight = fontSize * 1.2f;
                float blockHeight = lines.size() * lineHeight;
                // Check: widest line fits and total block fits in region
                bool fits = (blockHeight <= regionH * 0.85f);
                if (fits) {
                    for (auto& l : lines) {
                        if (measureText(fontInfo, l, scale) > maxWidth) { fits = false; break; }
                    }
                }
                if (fits) break;
                fontSize -= 2.0f;
            }

            if (fontSize < 8.0f) {
                fontSize = 8.0f;
                scale = stbtt_ScaleForPixelHeight(&fontInfo, fontSize);
                lines = wrapText(fontInfo, upper, scale, maxWidth);
            }

            // Vertical metrics
            int ascent, descent, lineGap;
            stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);
            float ascentPx = ascent * scale;
            float lineHeight = fontSize * 1.2f;
            float blockHeight = lines.size() * lineHeight;
            float blockStartY = regionY + (regionH - blockHeight) / 2.0f;

            // Draw each line
            for (size_t li = 0; li < lines.size(); ++li) {
                float lineWidth = measureText(fontInfo, lines[li], scale);
                int startX = static_cast<int>((imgW - lineWidth) / 2.0f);
                int startY = static_cast<int>(blockStartY + li * lineHeight + ascentPx);

                // Draw shadow if enabled
                if (activeStyle.shadowOffsetX != 0 || activeStyle.shadowOffsetY != 0) {
                    drawTextLine(pixels, imgW, imgH, fontInfo, lines[li], scale,
                                 startX + activeStyle.shadowOffsetX,
                                 startY + activeStyle.shadowOffsetY,
                                 activeStyle.shadowR, activeStyle.shadowG,
                                 activeStyle.shadowB, activeStyle.shadowA);
                }

                // Draw outline
                int r = activeStyle.outlineWidth;
                for (int ox = -r; ox <= r; ++ox) {
                    for (int oy = -r; oy <= r; ++oy) {
                        if (ox == 0 && oy == 0) continue;
                        if (ox * ox + oy * oy > r * r) continue;
                        drawTextLine(pixels, imgW, imgH, fontInfo, lines[li], scale,
                                     startX + ox, startY + oy,
                                     activeStyle.outlineR, activeStyle.outlineG,
                                     activeStyle.outlineB, 255);
                    }
                }

                // Draw text
                drawTextLine(pixels, imgW, imgH, fontInfo, lines[li], scale,
                             startX, startY,
                             activeStyle.textR, activeStyle.textG,
                             activeStyle.textB, activeStyle.textA);
            }
        }

        // Measure total width of a string in pixels
        static float measureText(stbtt_fontinfo& fontInfo, const std::string& text, float scale) {
            float width = 0;
            for (size_t i = 0; i < text.size(); ++i) {
                int advW, lsb;
                stbtt_GetCodepointHMetrics(&fontInfo, text[i], &advW, &lsb);
                width += advW * scale;
                if (i + 1 < text.size()) {
                    int kern = stbtt_GetCodepointKernAdvance(&fontInfo, text[i], text[i + 1]);
                    width += kern * scale;
                }
            }
            return width;
        }

        // Draw a single line of text onto the RGBA buffer
        static void drawTextLine(std::vector<unsigned char>& pixels,
                                 int imgW, int imgH,
                                 stbtt_fontinfo& fontInfo,
                                 const std::string& text,
                                 float scale,
                                 int x0, int y0,
                                 unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
            float xPos = static_cast<float>(x0);
            for (size_t i = 0; i < text.size(); ++i) {
                int cw, ch, xoff, yoff;
                unsigned char* bitmap = stbtt_GetCodepointBitmap(&fontInfo, scale, scale,
                                                                  text[i], &cw, &ch, &xoff, &yoff);
                if (bitmap) {
                    int cx = static_cast<int>(xPos) + xoff;
                    int cy = y0 + yoff;
                    for (int py = 0; py < ch; ++py) {
                        for (int px = 0; px < cw; ++px) {
                            int dx = cx + px;
                            int dy = cy + py;
                            if (dx < 0 || dx >= imgW || dy < 0 || dy >= imgH) continue;
                            unsigned char alpha = bitmap[py * cw + px];
                            if (alpha == 0) continue;
                            int idx = (dy * imgW + dx) * 4;
                            // Alpha-blend
                            float srcA = (alpha * a) / (255.0f * 255.0f);
                            float dstA = pixels[idx + 3] / 255.0f;
                            float outA = srcA + dstA * (1.0f - srcA);
                            if (outA > 0) {
                                pixels[idx + 0] = static_cast<unsigned char>(
                                    (r * srcA + pixels[idx + 0] * dstA * (1.0f - srcA)) / outA);
                                pixels[idx + 1] = static_cast<unsigned char>(
                                    (g * srcA + pixels[idx + 1] * dstA * (1.0f - srcA)) / outA);
                                pixels[idx + 2] = static_cast<unsigned char>(
                                    (b * srcA + pixels[idx + 2] * dstA * (1.0f - srcA)) / outA);
                                pixels[idx + 3] = static_cast<unsigned char>(outA * 255.0f);
                            }
                        }
                    }
                    stbtt_FreeBitmap(bitmap, nullptr);
                }

                int advW, lsb;
                stbtt_GetCodepointHMetrics(&fontInfo, text[i], &advW, &lsb);
                xPos += advW * scale;
                if (i + 1 < text.size()) {
                    int kern = stbtt_GetCodepointKernAdvance(&fontInfo, text[i], text[i + 1]);
                    xPos += kern * scale;
                }
            }
        }

        // ---- utilities ----

        static std::vector<unsigned char> readFile(const std::string& path) {
            std::ifstream f(path, std::ios::binary | std::ios::ate);
            if (!f.is_open()) return {};
            auto size = f.tellg();
            f.seekg(0, std::ios::beg);
            std::vector<unsigned char> buf(static_cast<size_t>(size));
            f.read(reinterpret_cast<char*>(buf.data()), size);
            return buf;
        }

        static std::string getExtension(const std::string& path) {
            auto pos = path.rfind('.');
            if (pos == std::string::npos) return "";
            return path.substr(pos + 1);
        }

        static std::string toLower(const std::string& s) {
            std::string r = s;
            std::transform(r.begin(), r.end(), r.begin(),
                           [](unsigned char c) { return std::tolower(c); });
            return r;
        }

        static std::string toUpper(const std::string& s) {
            std::string r = s;
            std::transform(r.begin(), r.end(), r.begin(),
                           [](unsigned char c) { return std::toupper(c); });
            return r;
        }
    };

} // namespace meme

#endif // MEME_RENDERER_H
