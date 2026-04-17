#ifndef MEME_EFFECTS_H
#define MEME_EFFECTS_H

#include <algorithm>            // clamp, min, max
#include <cmath>                // sqrt, sin, cos, pow, fabs
#include <cstdlib>              // rand, srand
#include <cstring>              // memcpy
#include <vector>               // vector (pixel buffers)

namespace effects {

    // Clamp helper
    static inline unsigned char clampByte(float v) {
        if (v < 0.0f) return 0;
        if (v > 255.0f) return 255;
        return static_cast<unsigned char>(v + 0.5f);
    }

    // --- Invert ---
    inline void invertColors(unsigned char* pixels, int w, int h) {
        int n = w * h * 4;
        for (int i = 0; i < n; i += 4) {
            pixels[i + 0] = 255 - pixels[i + 0];
            pixels[i + 1] = 255 - pixels[i + 1];
            pixels[i + 2] = 255 - pixels[i + 2];
            // alpha unchanged
        }
    }

    // --- Sepia ---
    inline void sepia(unsigned char* pixels, int w, int h) {
        int n = w * h * 4;
        for (int i = 0; i < n; i += 4) {
            float r = pixels[i + 0];
            float g = pixels[i + 1];
            float b = pixels[i + 2];
            float nr = r * 0.393f + g * 0.769f + b * 0.189f;
            float ng = r * 0.349f + g * 0.686f + b * 0.168f;
            float nb = r * 0.272f + g * 0.534f + b * 0.131f;
            pixels[i + 0] = clampByte(nr);
            pixels[i + 1] = clampByte(ng);
            pixels[i + 2] = clampByte(nb);
        }
    }

    // --- Brightness ---
    inline void brightness(unsigned char* pixels, int w, int h, float amount) {
        int n = w * h * 4;
        for (int i = 0; i < n; i += 4) {
            pixels[i + 0] = clampByte(pixels[i + 0] * amount);
            pixels[i + 1] = clampByte(pixels[i + 1] * amount);
            pixels[i + 2] = clampByte(pixels[i + 2] * amount);
        }
    }

    // --- Contrast ---
    inline void contrast(unsigned char* pixels, int w, int h, float amount) {
        int n = w * h * 4;
        for (int i = 0; i < n; i += 4) {
            for (int c = 0; c < 3; c++) {
                float v = pixels[i + c] / 255.0f;
                v = (v - 0.5f) * amount + 0.5f;
                pixels[i + c] = clampByte(v * 255.0f);
            }
        }
    }

    // --- Saturate ---
    // Convert RGB to HSL, multiply saturation, convert back
    inline void saturate(unsigned char* pixels, int w, int h, float amount) {
        int n = w * h * 4;
        for (int i = 0; i < n; i += 4) {
            float r = pixels[i + 0] / 255.0f;
            float g = pixels[i + 1] / 255.0f;
            float b = pixels[i + 2] / 255.0f;

            float maxC = std::max({r, g, b});
            float minC = std::min({r, g, b});
            float delta = maxC - minC;
            float l = (maxC + minC) / 2.0f;

            if (delta < 0.001f) {
                // Already gray, skip
                continue;
            }

            // Compute saturation
            float s = (l > 0.5f) ? delta / (2.0f - maxC - minC)
                                  : delta / (maxC + minC);

            // Compute hue
            float h_val;
            if (maxC == r) {
                h_val = (g - b) / delta + (g < b ? 6.0f : 0.0f);
            } else if (maxC == g) {
                h_val = (b - r) / delta + 2.0f;
            } else {
                h_val = (r - g) / delta + 4.0f;
            }
            h_val /= 6.0f;

            // Modify saturation
            s = std::min(1.0f, s * amount);

            // HSL to RGB
            auto hue2rgb = [](float p, float q, float t) -> float {
                if (t < 0.0f) t += 1.0f;
                if (t > 1.0f) t -= 1.0f;
                if (t < 1.0f / 6.0f) return p + (q - p) * 6.0f * t;
                if (t < 1.0f / 2.0f) return q;
                if (t < 2.0f / 3.0f) return p + (q - p) * (2.0f / 3.0f - t) * 6.0f;
                return p;
            };

            float q = (l < 0.5f) ? l * (1.0f + s) : l + s - l * s;
            float p = 2.0f * l - q;

            float nr = hue2rgb(p, q, h_val + 1.0f / 3.0f);
            float ng = hue2rgb(p, q, h_val);
            float nb = hue2rgb(p, q, h_val - 1.0f / 3.0f);

            pixels[i + 0] = clampByte(nr * 255.0f);
            pixels[i + 1] = clampByte(ng * 255.0f);
            pixels[i + 2] = clampByte(nb * 255.0f);
        }
    }

    // --- Box Blur ---
    inline void blur(unsigned char* pixels, int w, int h, int radius) {
        if (radius <= 0) return;
        std::vector<unsigned char> temp(w * h * 4);

        // Horizontal pass
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                float r = 0, g = 0, b = 0, a = 0;
                int count = 0;
                for (int dx = -radius; dx <= radius; dx++) {
                    int nx = x + dx;
                    if (nx < 0 || nx >= w) continue;
                    int idx = (y * w + nx) * 4;
                    r += pixels[idx + 0];
                    g += pixels[idx + 1];
                    b += pixels[idx + 2];
                    a += pixels[idx + 3];
                    count++;
                }
                int idx = (y * w + x) * 4;
                temp[idx + 0] = clampByte(r / count);
                temp[idx + 1] = clampByte(g / count);
                temp[idx + 2] = clampByte(b / count);
                temp[idx + 3] = clampByte(a / count);
            }
        }

        // Vertical pass
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                float r = 0, g = 0, b = 0, a = 0;
                int count = 0;
                for (int dy = -radius; dy <= radius; dy++) {
                    int ny = y + dy;
                    if (ny < 0 || ny >= h) continue;
                    int idx = (ny * w + x) * 4;
                    r += temp[idx + 0];
                    g += temp[idx + 1];
                    b += temp[idx + 2];
                    a += temp[idx + 3];
                    count++;
                }
                int idx = (y * w + x) * 4;
                pixels[idx + 0] = clampByte(r / count);
                pixels[idx + 1] = clampByte(g / count);
                pixels[idx + 2] = clampByte(b / count);
                pixels[idx + 3] = clampByte(a / count);
            }
        }
    }

    // --- Sharpen (3x3 kernel) ---
    inline void sharpen(unsigned char* pixels, int w, int h) {
        // Sharpening kernel:
        //  0 -1  0
        // -1  5 -1
        //  0 -1  0
        std::vector<unsigned char> orig(pixels, pixels + w * h * 4);
        for (int y = 1; y < h - 1; y++) {
            for (int x = 1; x < w - 1; x++) {
                for (int c = 0; c < 3; c++) {
                    int idx = (y * w + x) * 4 + c;
                    float val = 5.0f * orig[idx]
                              - orig[((y - 1) * w + x) * 4 + c]
                              - orig[((y + 1) * w + x) * 4 + c]
                              - orig[(y * w + x - 1) * 4 + c]
                              - orig[(y * w + x + 1) * 4 + c];
                    pixels[idx] = clampByte(val);
                }
            }
        }
    }

    // --- Pixelate ---
    inline void pixelate(unsigned char* pixels, int w, int h, int blockSize) {
        if (blockSize <= 1) return;
        for (int by = 0; by < h; by += blockSize) {
            for (int bx = 0; bx < w; bx += blockSize) {
                float r = 0, g = 0, b = 0, a = 0;
                int count = 0;
                int endY = std::min(by + blockSize, h);
                int endX = std::min(bx + blockSize, w);
                for (int y = by; y < endY; y++) {
                    for (int x = bx; x < endX; x++) {
                        int idx = (y * w + x) * 4;
                        r += pixels[idx + 0];
                        g += pixels[idx + 1];
                        b += pixels[idx + 2];
                        a += pixels[idx + 3];
                        count++;
                    }
                }
                unsigned char ar = clampByte(r / count);
                unsigned char ag = clampByte(g / count);
                unsigned char ab = clampByte(b / count);
                unsigned char aa = clampByte(a / count);
                for (int y = by; y < endY; y++) {
                    for (int x = bx; x < endX; x++) {
                        int idx = (y * w + x) * 4;
                        pixels[idx + 0] = ar;
                        pixels[idx + 1] = ag;
                        pixels[idx + 2] = ab;
                        pixels[idx + 3] = aa;
                    }
                }
            }
        }
    }

    // --- Noise ---
    inline void noise(unsigned char* pixels, int w, int h, float amount) {
        int n = w * h * 4;
        for (int i = 0; i < n; i += 4) {
            for (int c = 0; c < 3; c++) {
                float noise_val = ((std::rand() % 1000) / 500.0f - 1.0f) * amount * 255.0f;
                pixels[i + c] = clampByte(pixels[i + c] + noise_val);
            }
        }
    }

    // --- Vignette ---
    inline void vignette(unsigned char* pixels, int w, int h) {
        float cx = w / 2.0f;
        float cy = h / 2.0f;
        float maxDist = std::sqrt(cx * cx + cy * cy);
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                float dx = x - cx;
                float dy = y - cy;
                float dist = std::sqrt(dx * dx + dy * dy) / maxDist;
                float factor = 1.0f - dist * dist;  // quadratic falloff
                if (factor < 0.0f) factor = 0.0f;
                int idx = (y * w + x) * 4;
                pixels[idx + 0] = clampByte(pixels[idx + 0] * factor);
                pixels[idx + 1] = clampByte(pixels[idx + 1] * factor);
                pixels[idx + 2] = clampByte(pixels[idx + 2] * factor);
            }
        }
    }

    // --- JPEG quality simulation ---
    // Simulates low JPEG quality by quantizing color values
    inline void jpegQuality(unsigned char* pixels, int w, int h, int quality) {
        // Lower quality = more quantization
        int step = std::max(1, (100 - quality) / 5);
        int n = w * h * 4;
        for (int i = 0; i < n; i += 4) {
            for (int c = 0; c < 3; c++) {
                int v = pixels[i + c];
                v = (v / step) * step;
                pixels[i + c] = static_cast<unsigned char>(v);
            }
        }
    }

    // --- Tint (color overlay) ---
    inline void tint(unsigned char* pixels, int w, int h,
                     unsigned char tr, unsigned char tg, unsigned char tb, float alpha) {
        int n = w * h * 4;
        for (int i = 0; i < n; i += 4) {
            pixels[i + 0] = clampByte(pixels[i + 0] * (1.0f - alpha) + tr * alpha);
            pixels[i + 1] = clampByte(pixels[i + 1] * (1.0f - alpha) + tg * alpha);
            pixels[i + 2] = clampByte(pixels[i + 2] * (1.0f - alpha) + tb * alpha);
        }
    }

    // --- Hue Shift ---
    inline void hueShift(unsigned char* pixels, int w, int h, float degrees) {
        float rad = degrees * 3.14159265f / 180.0f;
        float cosA = std::cos(rad), sinA = std::sin(rad);
        // Rotation matrix for hue in RGB space
        float m00 = 0.213f + cosA * 0.787f - sinA * 0.213f;
        float m01 = 0.715f - cosA * 0.715f - sinA * 0.715f;
        float m02 = 0.072f - cosA * 0.072f + sinA * 0.928f;
        float m10 = 0.213f - cosA * 0.213f + sinA * 0.143f;
        float m11 = 0.715f + cosA * 0.285f + sinA * 0.140f;
        float m12 = 0.072f - cosA * 0.072f - sinA * 0.283f;
        float m20 = 0.213f - cosA * 0.213f - sinA * 0.787f;
        float m21 = 0.715f - cosA * 0.715f + sinA * 0.715f;
        float m22 = 0.072f + cosA * 0.928f + sinA * 0.072f;
        int n = w * h * 4;
        for (int i = 0; i < n; i += 4) {
            float r = pixels[i], g = pixels[i + 1], b = pixels[i + 2];
            pixels[i + 0] = clampByte(r * m00 + g * m01 + b * m02);
            pixels[i + 1] = clampByte(r * m10 + g * m11 + b * m12);
            pixels[i + 2] = clampByte(r * m20 + g * m21 + b * m22);
        }
    }

    // --- Glow (bloom) ---
    inline void glow(unsigned char* pixels, int w, int h, int radius) {
        // Copy original, blur the copy, screen blend
        std::vector<unsigned char> blurred(pixels, pixels + w * h * 4);
        blur(blurred.data(), w, h, radius);
        int n = w * h * 4;
        for (int i = 0; i < n; i += 4) {
            for (int c = 0; c < 3; c++) {
                float base = pixels[i + c] / 255.0f;
                float bloom = blurred[i + c] / 255.0f;
                // Screen blend: 1 - (1-a)(1-b)
                float result = 1.0f - (1.0f - base) * (1.0f - bloom);
                pixels[i + c] = clampByte(result * 255.0f);
            }
        }
    }

    // --- Posterize ---
    inline void posterize(unsigned char* pixels, int w, int h, int levels) {
        if (levels < 2) levels = 2;
        float factor = 255.0f / (levels - 1);
        int n = w * h * 4;
        for (int i = 0; i < n; i += 4) {
            for (int c = 0; c < 3; c++) {
                int quantized = static_cast<int>(std::round(pixels[i + c] / factor));
                pixels[i + c] = clampByte(quantized * factor);
            }
        }
    }

    // --- Chromatic Aberration ---
    inline void chromatic(unsigned char* pixels, int w, int h, int offset) {
        std::vector<unsigned char> copy(pixels, pixels + w * h * 4);
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                int di = (y * w + x) * 4;
                // Shift red channel left, blue channel right
                int rxSrc = std::clamp(x - offset, 0, w - 1);
                int bxSrc = std::clamp(x + offset, 0, w - 1);
                pixels[di + 0] = copy[(y * w + rxSrc) * 4 + 0]; // red shifted
                pixels[di + 1] = copy[di + 1];                    // green stays
                pixels[di + 2] = copy[(y * w + bxSrc) * 4 + 2]; // blue shifted
            }
        }
    }

    // --- Threshold (binarize) ---
    inline void threshold(unsigned char* pixels, int w, int h, int level) {
        int n = w * h * 4;
        for (int i = 0; i < n; i += 4) {
            int luma = (pixels[i] * 299 + pixels[i + 1] * 587 + pixels[i + 2] * 114) / 1000;
            unsigned char val = luma >= level ? 255 : 0;
            pixels[i + 0] = pixels[i + 1] = pixels[i + 2] = val;
        }
    }

    // --- Grayscale ---
    inline void grayscale(unsigned char* pixels, int w, int h) {
        int n = w * h * 4;
        for (int i = 0; i < n; i += 4) {
            int luma = (pixels[i] * 299 + pixels[i + 1] * 587 + pixels[i + 2] * 114) / 1000;
            pixels[i + 0] = pixels[i + 1] = pixels[i + 2] = static_cast<unsigned char>(luma);
        }
    }

} // namespace effects

#endif // MEME_EFFECTS_H
