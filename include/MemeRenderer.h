#ifndef MEME_RENDERER_H
#define MEME_RENDERER_H

#include <algorithm>            // min, max, transform
#include <cctype>               // toupper (uppercase text transform)
#include <cmath>                // round, sqrt
#include <cstdint>              // uint8_t
#include <cstdio>               // snprintf
#include <cstring>              // memcpy, memset
#include <filesystem>           // path, exists (font/image file resolution)
#include <fstream>              // ifstream (file reading)
#include <memory>               // shared_ptr
#include <sstream>              // ostringstream
#include <stdexcept>            // runtime_error
#include <string>               // string
#include <unordered_map>        // unordered_map (image/font/glyph/render caches)
#include <utility>              // pair, move
#include <vector>               // vector (pixel buffers)

// Do NOT define STB_*_IMPLEMENTATION here -- already in src/stb_impl.cpp
#include "RenderSurface.h"      // meme::RenderSurface (pixel buffer output)
#include "stb/stb_image.h"      // stbi_load, stbi_image_free
#include "stb/stb_image_write.h" // stbi_write_png
#include "stb/stb_truetype.h"   // stbtt_InitFont, stbtt_GetCodepointBitmap

namespace meme {

    // TextStyle is defined in MacMeme.h to avoid circular dependency

    class MemeRenderer {
    public:
        static std::shared_ptr<RenderSurface> renderSurface(const std::string& imagePath,
                                                            const std::string& topText,
                                                            const std::string& bottomText,
                                                            const std::string& centerText,
                                                            int targetWidth,
                                                            int targetHeight,
                                                            const TextStyle& style = TextStyle{}) {
            std::string key = renderCacheKey(imagePath, topText, bottomText, centerText,
                                             targetWidth, targetHeight, style);
            auto& cache = renderCache();
            auto it = cache.find(key);
            if (it != cache.end()) return it->second;

            int outWidth = 0;
            int outHeight = 0;
            auto pixels = renderInternal(imagePath, topText, bottomText, centerText,
                                         targetWidth, targetHeight, outWidth, outHeight, style);
            auto surface = std::make_shared<RenderSurface>(std::move(pixels), outWidth, outHeight);
            cache.emplace(std::move(key), surface);
            return surface;
        }

        // Render meme to RGBA pixel buffer.
        // Sets outWidth / outHeight through reference params (via the overload below).
        static std::vector<unsigned char> render(const std::string& imagePath,
                                                  const std::string& topText,
                                                  const std::string& bottomText,
                                                  int targetWidth = 0,
                                                  int targetHeight = 0) {
            auto surface = renderSurface(imagePath, topText, bottomText, "", targetWidth, targetHeight);
            return surface->pixels;
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
            auto surface = renderSurface(imagePath, topText, bottomText, "", targetWidth, targetHeight, style);
            outWidth = surface->width;
            outHeight = surface->height;
            return surface->pixels;
        }

        static std::vector<unsigned char> render(const std::string& imagePath,
                                                  const std::string& topText,
                                                  const std::string& bottomText,
                                                  const std::string& centerText,
                                                  int targetWidth = 0,
                                                  int targetHeight = 0) {
            auto surface = renderSurface(imagePath, topText, bottomText, centerText, targetWidth, targetHeight);
            return surface->pixels;
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
            auto surface = renderSurface(imagePath, topText, bottomText, centerText, targetWidth, targetHeight, style);
            outWidth = surface->width;
            outHeight = surface->height;
            return surface->pixels;
        }

        // Render with positioned text entries at absolute x,y coordinates
        static std::vector<unsigned char> renderWithPositions(
                const std::string& imagePath, const std::string& topText,
                const std::string& bottomText, const std::string& centerText,
                int targetWidth, int targetHeight, int& outWidth, int& outHeight,
                const TextStyle& style, const std::vector<PositionedText>& posTexts) {
            return renderInternal(imagePath, topText, bottomText, centerText,
                                   targetWidth, targetHeight, outWidth, outHeight, style, posTexts);
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
        struct CachedGlyph {
            std::vector<unsigned char> bitmap;
            int w = 0;
            int h = 0;
            int xoff = 0;
            int yoff = 0;
            float advance = 0.0f;
        };
        struct TextLayoutCache {
            std::unordered_map<std::string, float> widths;
            std::unordered_map<std::string, std::vector<std::string>> wraps;
        };

        static std::unordered_map<std::string, CachedImage>& imageCache() {
            static std::unordered_map<std::string, CachedImage> cache;
            return cache;
        }

        static std::unordered_map<std::string, std::shared_ptr<RenderSurface>>& renderCache() {
            static std::unordered_map<std::string, std::shared_ptr<RenderSurface>> cache;
            return cache;
        }

        static std::unordered_map<std::uint64_t, CachedGlyph>& glyphCache() {
            static std::unordered_map<std::uint64_t, CachedGlyph> cache;
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

        static int scaleKey(float scale) {
            return std::max(1, static_cast<int>(std::lround(scale * 10000.0f)));
        }

        static std::string renderCacheKey(const std::string& imagePath,
                                          const std::string& topText,
                                          const std::string& bottomText,
                                          const std::string& centerText,
                                          int targetWidth,
                                          int targetHeight,
                                          const TextStyle& style) {
            std::ostringstream out;
            out << imagePath << '\n'
                << topText << '\n'
                << centerText << '\n'
                << bottomText << '\n'
                << targetWidth << 'x' << targetHeight << '\n'
                << static_cast<int>(style.textR) << ',' << static_cast<int>(style.textG) << ','
                << static_cast<int>(style.textB) << ',' << static_cast<int>(style.textA) << '\n'
                << static_cast<int>(style.outlineR) << ',' << static_cast<int>(style.outlineG) << ','
                << static_cast<int>(style.outlineB) << ',' << style.outlineWidth << '\n'
                << style.shadowOffsetX << ',' << style.shadowOffsetY << ','
                << static_cast<int>(style.shadowR) << ',' << static_cast<int>(style.shadowG) << ','
                << static_cast<int>(style.shadowB) << ',' << static_cast<int>(style.shadowA) << '\n'
                << style.fontSizeOverride << '\n'
                << static_cast<int>(style.bgR) << ',' << static_cast<int>(style.bgG) << ','
                << static_cast<int>(style.bgB) << ',' << static_cast<int>(style.bgA);
            return out.str();
        }

        static const CachedGlyph& glyphFor(stbtt_fontinfo& fontInfo, int codepoint, float scale) {
            std::uint64_t key = (static_cast<std::uint64_t>(scaleKey(scale)) << 32)
                | static_cast<std::uint32_t>(codepoint);
            auto& cache = glyphCache();
            auto it = cache.find(key);
            if (it != cache.end()) return it->second;

            CachedGlyph glyph;
            int advanceWidth = 0;
            int leftSideBearing = 0;
            stbtt_GetCodepointHMetrics(&fontInfo, codepoint, &advanceWidth, &leftSideBearing);
            glyph.advance = advanceWidth * scale;

            unsigned char* bitmap = stbtt_GetCodepointBitmap(&fontInfo, scale, scale, codepoint,
                                                             &glyph.w, &glyph.h, &glyph.xoff, &glyph.yoff);
            if (bitmap) {
                glyph.bitmap.assign(bitmap, bitmap + glyph.w * glyph.h);
                stbtt_FreeBitmap(bitmap, nullptr);
            }

            return cache.emplace(key, std::move(glyph)).first->second;
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
                const TextStyle& activeStyle = TextStyle{},
                const std::vector<PositionedText>& posTexts = {}) {

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
            TextLayoutCache textLayoutCache;

            // Draw top text (upper half) — anchored to top edge
            if (!topText.empty()) {
                int regionY = 0;
                int regionH = h / 2;
                drawMemeText(pixels, w, h, fontInfo, fontData, topText,
                             regionY, regionH, activeStyle, textLayoutCache, -1);
            }

            if (!centerText.empty()) {
                drawMemeText(pixels, w, h, fontInfo, fontData, centerText,
                             0, h, activeStyle, textLayoutCache, 0);
            }

            // Draw bottom text (lower half) — anchored to bottom edge
            if (!bottomText.empty()) {
                int regionY = h / 2;
                int regionH = h / 2;
                drawMemeText(pixels, w, h, fontInfo, fontData, bottomText,
                             regionY, regionH, activeStyle, textLayoutCache, 1);
            }

            // Draw positioned text entries at absolute x,y coordinates
            for (const auto& pt : posTexts) {
                if (pt.content.empty()) continue;
                // Render at the specified position — use a small region around the point
                int regionH = h / 4;  // use 25% of image height for font sizing
                TextStyle positionedStyle = activeStyle;
                if (pt.fontSizeOverride != 0) positionedStyle.fontSizeOverride = pt.fontSizeOverride;
                drawMemeText(pixels, w, h, fontInfo, fontData, pt.content,
                             pt.y - regionH / 2, regionH, positionedStyle, textLayoutCache, 0,
                             pt.x);
            }

            outWidth = w;
            outHeight = h;
            return pixels;
        }

        // Word-wrap text to fit within maxWidth
        static std::vector<std::string> wrapText(stbtt_fontinfo& fontInfo,
                                                  const std::string& text,
                                                  float scale, float maxWidth,
                                                  TextLayoutCache& cache) {
            std::string cacheKey = std::to_string(scaleKey(scale)) + "|" +
                std::to_string(static_cast<int>(std::lround(maxWidth))) + "|" + text;
            auto cached = cache.wraps.find(cacheKey);
            if (cached != cache.wraps.end()) return cached->second;

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
                if (measureText(fontInfo, candidate, scale, cache) <= maxWidth) {
                    line = candidate;
                } else {
                    lines.push_back(line);
                    line = words[i];
                }
            }
            lines.push_back(line);
            cache.wraps.emplace(std::move(cacheKey), lines);
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
                                 const TextStyle& activeStyle,
                                 TextLayoutCache& textLayoutCache,
                                 int align = 0,
                                 int xCenter = -1) {

            std::string upper = activeStyle.uppercase ? toUpper(text) : text;
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
                lines = wrapText(fontInfo, upper, scale, maxWidth, textLayoutCache);
                float lineHeight = fontSize * 1.1f;
                float blockHeight = lines.size() * lineHeight;
                bool fits = (blockHeight <= regionH * 0.85f);
                if (fits) {
                    for (auto& l : lines) {
                        if (measureText(fontInfo, l, scale, textLayoutCache) > maxWidth) {
                            fits = false;
                            break;
                        }
                    }
                }
                if (fits) break;
                fontSize -= 2.0f;
            }

            if (fontSize < 8.0f) {
                fontSize = 8.0f;
                scale = stbtt_ScaleForPixelHeight(&fontInfo, fontSize);
                lines = wrapText(fontInfo, upper, scale, maxWidth, textLayoutCache);
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
                float lineWidth = measureText(fontInfo, lines[li], scale, textLayoutCache);
                int startX = (xCenter >= 0)
                    ? static_cast<int>(xCenter - lineWidth / 2.0f)
                    : static_cast<int>((imgW - lineWidth) / 2.0f);
                int startY = static_cast<int>(blockStartY + li * lineHeight + ascentPx);

                // Draw shadow if enabled
                if (activeStyle.shadowOffsetX != 0 || activeStyle.shadowOffsetY != 0) {
                    drawTextLine(pixels, imgW, imgH, fontInfo, lines[li], scale,
                                 startX + activeStyle.shadowOffsetX,
                                 startY + activeStyle.shadowOffsetY,
                                 activeStyle.shadowR, activeStyle.shadowG,
                                 activeStyle.shadowB, activeStyle.shadowA);
                }

                // Draw outline (skip if not bold — gives clean caption look)
                int r = activeStyle.bold ? activeStyle.outlineWidth : 0;
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
        static float measureText(stbtt_fontinfo& fontInfo,
                                 const std::string& text,
                                 float scale,
                                 TextLayoutCache& cache) {
            std::string cacheKey = std::to_string(scaleKey(scale)) + "|" + text;
            auto cached = cache.widths.find(cacheKey);
            if (cached != cache.widths.end()) return cached->second;

            float width = 0;
            for (size_t i = 0; i < text.size(); ++i) {
                const auto& glyph = glyphFor(fontInfo, static_cast<unsigned char>(text[i]), scale);
                width += glyph.advance;
                if (i + 1 < text.size()) {
                    int kern = stbtt_GetCodepointKernAdvance(
                        &fontInfo, static_cast<unsigned char>(text[i]), static_cast<unsigned char>(text[i + 1]));
                    width += kern * scale;
                }
            }
            cache.widths.emplace(std::move(cacheKey), width);
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
                const auto& glyph = glyphFor(fontInfo, static_cast<unsigned char>(text[i]), scale);
                if (!glyph.bitmap.empty()) {
                    int cx = static_cast<int>(xPos) + glyph.xoff;
                    int cy = y0 + glyph.yoff;
                    for (int py = 0; py < glyph.h; ++py) {
                        for (int px = 0; px < glyph.w; ++px) {
                            int dx = cx + px;
                            int dy = cy + py;
                            if (dx < 0 || dx >= imgW || dy < 0 || dy >= imgH) continue;
                            unsigned char alpha = glyph.bitmap[py * glyph.w + px];
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
                }

                xPos += glyph.advance;
                if (i + 1 < text.size()) {
                    int kern = stbtt_GetCodepointKernAdvance(
                        &fontInfo, static_cast<unsigned char>(text[i]), static_cast<unsigned char>(text[i + 1]));
                    xPos += kern * scale;
                }
            }
        }

        // ---- utilities ----

        static std::vector<unsigned char> readFile(const std::string& path) {
            std::ifstream f(path, std::ios::binary | std::ios::ate);
            if (!f.is_open()) return {};
            auto size = f.tellg();
            if (size < 0 || static_cast<size_t>(size) > 50 * 1024 * 1024) return {};  // 50MB max
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
