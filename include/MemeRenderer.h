#ifndef MEME_RENDERER_H
#define MEME_RENDERER_H

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
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
            return renderInternal(imagePath, topText, bottomText, "", targetWidth, targetHeight, w, h);
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
            return renderInternal(imagePath, topText, bottomText, "", targetWidth, targetHeight, outWidth, outHeight, style);
        }

        static std::vector<unsigned char> render(const std::string& imagePath,
                                                  const std::string& topText,
                                                  const std::string& bottomText,
                                                  const std::string& centerText,
                                                  int targetWidth = 0,
                                                  int targetHeight = 0) {
            int w, h;
            return renderInternal(imagePath, topText, bottomText, centerText, targetWidth, targetHeight, w, h);
        }

        static std::vector<unsigned char> render(const std::string& imagePath,
                                                  const std::string& topText,
                                                  const std::string& bottomText,
                                                  const std::string& centerText,
                                                  int targetWidth,
                                                  int targetHeight,
                                                  int& outWidth,
                                                  int& outHeight,
                                                  const TextStyle& style = TextStyle{}) {
            return renderInternal(imagePath, topText, bottomText, centerText, targetWidth, targetHeight, outWidth, outHeight, style);
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
        // ---- caches ----
        struct CachedImage { std::vector<unsigned char> pixels; int w, h; };

        static std::unordered_map<std::string, CachedImage>& imageCache() {
            static std::unordered_map<std::string, CachedImage> cache;
            return cache;
        }

        static const CachedImage& loadImageCached(const std::string& path) {
            auto& cache = imageCache();
            auto it = cache.find(path);
            if (it != cache.end()) return it->second;
            int w, h, c;
            unsigned char* data = stbi_load(path.c_str(), &w, &h, &c, 4);
            if (!data) throw std::runtime_error("MemeRenderer: cannot load image '" + path + "'");
            CachedImage img;
            img.pixels.assign(data, data + w * h * 4);
            img.w = w; img.h = h;
            stbi_image_free(data);
            return cache.emplace(path, std::move(img)).first->second;
        }

        struct CachedFont { std::vector<unsigned char> data; stbtt_fontinfo info; };

        static CachedFont& fontCache() {
            static CachedFont cache;
            if (cache.data.empty()) {
                std::string fontPath = getAssetsDir() + "/fonts/meme-font.ttf";
                cache.data = readFile(fontPath);
                if (cache.data.empty()) throw std::runtime_error("MemeRenderer: cannot load font");
                stbtt_InitFont(&cache.info, cache.data.data(),
                    stbtt_GetFontOffsetForIndex(cache.data.data(), 0));
            }
            return cache;
        }

        // ---- internal render ----
        static std::vector<unsigned char> renderInternal(
                const std::string& imagePath,
                const std::string& topText,
                const std::string& bottomText,
                const std::string& centerText,
                int targetWidth,
                int targetHeight,
                int& outWidth,
                int& outHeight,
                const TextStyle& activeStyle = TextStyle{}) {

            // Load template image (cached)
            const auto& cached = loadImageCached(imagePath);
            int imgW = cached.w, imgH = cached.h;
            const unsigned char* imgData = cached.pixels.data();

            int w = (targetWidth > 0) ? targetWidth : imgW;
            int h = (targetHeight > 0) ? targetHeight : imgH;

            // Build RGBA buffer -- fill with background, then composite template on top
            std::vector<unsigned char> pixels(w * h * 4);

            // Fill with background color or default white
            unsigned char bgR = activeStyle.bgA > 0 ? activeStyle.bgR : 255;
            unsigned char bgG = activeStyle.bgA > 0 ? activeStyle.bgG : 255;
            unsigned char bgB = activeStyle.bgA > 0 ? activeStyle.bgB : 255;
            unsigned char bgA = activeStyle.bgA > 0 ? activeStyle.bgA : 255;
            for (int i = 0; i < w * h; ++i) {
                pixels[i * 4 + 0] = bgR;
                pixels[i * 4 + 1] = bgG;
                pixels[i * 4 + 2] = bgB;
                pixels[i * 4 + 3] = bgA;
            }

            // Alpha-composite template image on top of background
            for (int y = 0; y < h; ++y) {
                int srcY = y * imgH / h;
                for (int x = 0; x < w; ++x) {
                    int srcX = x * imgW / w;
                    int srcIdx = (srcY * imgW + srcX) * 4;
                    if (imgData[srcIdx + 3] == 0) continue; // skip transparent pixels
                    int dstIdx = (y * w + x) * 4;
                    if (imgData[srcIdx + 3] == 255) {
                        // Fully opaque — direct copy (fast path)
                        pixels[dstIdx + 0] = imgData[srcIdx + 0];
                        pixels[dstIdx + 1] = imgData[srcIdx + 1];
                        pixels[dstIdx + 2] = imgData[srcIdx + 2];
                        pixels[dstIdx + 3] = 255;
                    } else {
                        // Semi-transparent — alpha blend
                        float srcA = imgData[srcIdx + 3] / 255.0f;
                        float dstA = pixels[dstIdx + 3] / 255.0f;
                        float outA = srcA + dstA * (1.0f - srcA);
                        if (outA > 0) {
                            pixels[dstIdx+0] = static_cast<unsigned char>(
                                (imgData[srcIdx+0]*srcA + pixels[dstIdx+0]*dstA*(1.0f-srcA)) / outA);
                            pixels[dstIdx+1] = static_cast<unsigned char>(
                                (imgData[srcIdx+1]*srcA + pixels[dstIdx+1]*dstA*(1.0f-srcA)) / outA);
                            pixels[dstIdx+2] = static_cast<unsigned char>(
                                (imgData[srcIdx+2]*srcA + pixels[dstIdx+2]*dstA*(1.0f-srcA)) / outA);
                            pixels[dstIdx+3] = static_cast<unsigned char>(outA * 255.0f);
                        }
                    }
                }
            }
            // Load font (cached — local copy of fontInfo since stbtt may mutate it)
            auto& font = fontCache();
            const auto& fontData = font.data;
            stbtt_fontinfo fontInfo = font.info;

            // Draw top text (upper half) — anchored to top edge
            if (!topText.empty()) {
                int regionY = 0;
                int regionH = h / 2;
                drawMemeText(pixels, w, h, fontInfo, fontData, topText, regionY, regionH, activeStyle, -1);
            }

            if (!centerText.empty()) {
                drawMemeText(pixels, w, h, fontInfo, fontData, centerText, 0, h, activeStyle, 0);
            }

            // Draw bottom text (lower half) — anchored to bottom edge
            if (!bottomText.empty()) {
                int regionY = h / 2;
                int regionH = h / 2;
                drawMemeText(pixels, w, h, fontInfo, fontData, bottomText, regionY, regionH, activeStyle, 1);
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

        // Draw text in a horizontal strip of the image
        // align: -1 = top edge, 0 = center, 1 = bottom edge
        static void drawMemeText(std::vector<unsigned char>& pixels,
                                 int imgW, int imgH,
                                 stbtt_fontinfo& fontInfo,
                                 const std::vector<unsigned char>& fontData,
                                 const std::string& text,
                                 int regionY, int regionH,
                                 const TextStyle& activeStyle = TextStyle{},
                                 int align = 0) {

            std::string upper = toUpper(text);
            float maxWidth = imgW * 0.9f;

            // Resolve fontSize: presets (sm/md/lg/xlg encoded as -1..-4),
            // absolute pixel value (> 0), or auto-size (0)
            float fontSize = 0;
            float fso = activeStyle.fontSizeOverride;
            if (fso == -1)      fontSize = regionH * 0.25f; // sm
            else if (fso == -2) fontSize = regionH * 0.40f; // md
            else if (fso == -3) fontSize = regionH * 0.55f; // lg
            else if (fso == -4) fontSize = regionH * 0.70f; // xlg
            else if (fso > 0)   fontSize = fso;             // absolute px
            else                fontSize = regionH * 0.70f; // auto (default)
            if (fontSize < 10.0f) fontSize = 10.0f;

            float scale = 0;
            std::vector<std::string> lines;

            // Auto-size: shrink until all wrapped lines fit width and block fits height
            while (fontSize >= 8.0f) {
                scale = stbtt_ScaleForPixelHeight(&fontInfo, fontSize);
                lines = wrapText(fontInfo, upper, scale, maxWidth);
                float lineHeight = fontSize * 1.1f;
                float blockHeight = lines.size() * lineHeight;
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
            float lineHeight = fontSize * 1.1f;
            float blockHeight = lines.size() * lineHeight;
            float margin = regionH * 0.08f;
            float blockStartY;
            if (align < 0) {
                // Top-aligned: anchor to top edge with margin
                blockStartY = regionY + margin;
            } else if (align > 0) {
                // Bottom-aligned: anchor to bottom edge with margin
                blockStartY = regionY + regionH - blockHeight - margin;
            } else {
                // Centered (e.g. for center: text)
                blockStartY = regionY + (regionH - blockHeight) / 2.0f;
            }

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
