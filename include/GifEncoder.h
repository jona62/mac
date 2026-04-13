#ifndef GIF_ENCODER_H
#define GIF_ENCODER_H

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace meme {

    class GifEncoder {
    public:
        GifEncoder(const std::string& outputPath, int width, int height)
            : width(width), height(height), firstFrame(true), fp(nullptr) {
            fp = std::fopen(outputPath.c_str(), "wb");
            if (!fp) return;

            // --- GIF89a header ---
            std::fwrite("GIF89a", 1, 6, fp);

            // --- Logical screen descriptor ---
            writeU16(width);
            writeU16(height);

            // Global color table flag = 1, color resolution = 7 (8 bits),
            // sort = 0, size of GCT = 7 (2^(7+1) = 256 entries)
            unsigned char packed = 0x80 | (7 << 4) | 7; // = 0xF7
            std::fputc(packed, fp);
            std::fputc(0, fp);   // background color index
            std::fputc(0, fp);   // pixel aspect ratio

            // --- Global color table (6-6-6 color cube = 216 + 40 grays) ---
            buildGlobalPalette();
            std::fwrite(palette, 1, 768, fp);

            // --- Netscape application extension for looping ---
            std::fputc(0x21, fp);  // extension introducer
            std::fputc(0xFF, fp);  // application extension
            std::fputc(11, fp);    // block size
            std::fwrite("NETSCAPE2.0", 1, 11, fp);
            std::fputc(3, fp);     // sub-block size
            std::fputc(1, fp);     // sub-block index
            writeU16(0);           // loop count (0 = infinite)
            std::fputc(0, fp);     // block terminator
        }

        ~GifEncoder() {
            if (fp) {
                finish();
            }
        }

        void addFrame(const unsigned char* rgba, int delayCs) {
            if (!fp) return;

            // Floyd-Steinberg dithering + quantization
            std::vector<float> buf(width * height * 3);
            for (int i = 0; i < width * height; ++i) {
                buf[i * 3 + 0] = rgba[i * 4 + 0];
                buf[i * 3 + 1] = rgba[i * 4 + 1];
                buf[i * 3 + 2] = rgba[i * 4 + 2];
            }

            std::vector<unsigned char> indexed(width * height);
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    int i = y * width + x;
                    int r = std::max(0, std::min(255, static_cast<int>(buf[i * 3 + 0] + 0.5f)));
                    int g = std::max(0, std::min(255, static_cast<int>(buf[i * 3 + 1] + 0.5f)));
                    int b = std::max(0, std::min(255, static_cast<int>(buf[i * 3 + 2] + 0.5f)));
                    unsigned char idx = findClosestColor(r, g, b);
                    indexed[i] = idx;

                    float errR = r - palette[idx * 3 + 0];
                    float errG = g - palette[idx * 3 + 1];
                    float errB = b - palette[idx * 3 + 2];

                    auto diffuse = [&](int nx, int ny, float w) {
                        if (nx < 0 || nx >= width || ny >= height) return;
                        int ni = (ny * width + nx) * 3;
                        buf[ni + 0] += errR * w;
                        buf[ni + 1] += errG * w;
                        buf[ni + 2] += errB * w;
                    };
                    diffuse(x + 1, y,     7.0f / 16.0f);
                    diffuse(x - 1, y + 1, 3.0f / 16.0f);
                    diffuse(x,     y + 1, 5.0f / 16.0f);
                    diffuse(x + 1, y + 1, 1.0f / 16.0f);
                }
            }

            // --- Graphic Control Extension ---
            std::fputc(0x21, fp);  // extension introducer
            std::fputc(0xF9, fp);  // graphic control label
            std::fputc(4, fp);     // block size
            std::fputc(0x04, fp);  // disposal method 1 (do not dispose), no transparency
            writeU16(delayCs);     // delay in centiseconds
            std::fputc(0, fp);     // transparent color index (unused)
            std::fputc(0, fp);     // block terminator

            // --- Image Descriptor ---
            std::fputc(0x2C, fp);  // image separator
            writeU16(0);           // left
            writeU16(0);           // top
            writeU16(width);
            writeU16(height);
            std::fputc(0, fp);     // no local color table, not interlaced

            // --- LZW image data ---
            int minCodeSize = 8;
            std::fputc(minCodeSize, fp);
            lzwCompress(indexed.data(), width * height, minCodeSize);

            firstFrame = false;
        }

        void finish() {
            if (!fp) return;
            std::fputc(0x3B, fp);  // GIF trailer
            std::fclose(fp);
            fp = nullptr;
        }

    private:
        int width, height;
        bool firstFrame;
        FILE* fp;
        unsigned char palette[768]; // 256 * 3

        void writeU16(int value) {
            std::fputc(value & 0xFF, fp);
            std::fputc((value >> 8) & 0xFF, fp);
        }

        void buildGlobalPalette() {
            // 6x6x6 color cube (216 colors) + 40 gray levels
            int idx = 0;
            for (int r = 0; r < 6; ++r) {
                for (int g = 0; g < 6; ++g) {
                    for (int b = 0; b < 6; ++b) {
                        palette[idx * 3 + 0] = static_cast<unsigned char>(r * 51);
                        palette[idx * 3 + 1] = static_cast<unsigned char>(g * 51);
                        palette[idx * 3 + 2] = static_cast<unsigned char>(b * 51);
                        idx++;
                    }
                }
            }
            // Fill remaining 40 entries with evenly-spaced grays
            for (int i = 0; i < 40; ++i) {
                unsigned char v = static_cast<unsigned char>(i * 255 / 39);
                palette[idx * 3 + 0] = v;
                palette[idx * 3 + 1] = v;
                palette[idx * 3 + 2] = v;
                idx++;
            }
        }

        unsigned char findClosestColor(int r, int g, int b) const {
            // Fast 6-6-6 cube index
            int ri = (r + 25) / 51; if (ri > 5) ri = 5;
            int gi = (g + 25) / 51; if (gi > 5) gi = 5;
            int bi = (b + 25) / 51; if (bi > 5) bi = 5;
            return static_cast<unsigned char>(ri * 36 + gi * 6 + bi);
        }

        // ---- LZW compression ----

        struct LzwDict {
            static const int MAX_CODE = 4096;
            int prefixes[MAX_CODE];
            unsigned char suffixes[MAX_CODE];
            int size;

            void reset(int clearCode) {
                size = clearCode + 2;
                for (int i = 0; i < clearCode; ++i) {
                    prefixes[i] = -1;
                    suffixes[i] = static_cast<unsigned char>(i);
                }
                // Mark clear code and EOI entries so find() never matches them
                prefixes[clearCode] = -2;
                suffixes[clearCode] = 0;
                prefixes[clearCode + 1] = -2;
                suffixes[clearCode + 1] = 0;
            }

            int find(int prefix, unsigned char suffix) const {
                for (int i = (prefix == -1 ? 0 : prefix + 1); i < size; ++i) {
                    if (prefixes[i] == prefix && suffixes[i] == suffix) return i;
                }
                return -1;
            }

            bool add(int prefix, unsigned char suffix) {
                if (size >= MAX_CODE) return false;
                prefixes[size] = prefix;
                suffixes[size] = suffix;
                size++;
                return true;
            }
        };

        struct BitWriter {
            FILE* out;
            unsigned char buf[255];
            int bufPos;
            unsigned int bits;
            int bitCount;

            BitWriter(FILE* f) : out(f), bufPos(0), bits(0), bitCount(0) {}

            void writeBits(int code, int nbits) {
                bits |= static_cast<unsigned int>(code) << bitCount;
                bitCount += nbits;
                while (bitCount >= 8) {
                    buf[bufPos++] = bits & 0xFF;
                    bits >>= 8;
                    bitCount -= 8;
                    if (bufPos == 255) flushBlock();
                }
            }

            void flushBlock() {
                if (bufPos > 0) {
                    std::fputc(bufPos, out);
                    std::fwrite(buf, 1, bufPos, out);
                    bufPos = 0;
                }
            }

            void finish() {
                if (bitCount > 0) {
                    buf[bufPos++] = bits & 0xFF;
                }
                flushBlock();
                std::fputc(0, out); // sub-block terminator
            }
        };

        void lzwCompress(const unsigned char* data, int dataSize, int minCodeSize) {
            int clearCode = 1 << minCodeSize;
            int eoiCode = clearCode + 1;
            int codeSize = minCodeSize + 1;

            LzwDict dict;
            dict.reset(clearCode);

            BitWriter bw(fp);
            bw.writeBits(clearCode, codeSize);

            if (dataSize == 0) {
                bw.writeBits(eoiCode, codeSize);
                bw.finish();
                return;
            }

            int prefix = data[0];
            for (int i = 1; i < dataSize; ++i) {
                unsigned char suffix = data[i];
                int entry = dict.find(prefix, suffix);
                if (entry != -1) {
                    prefix = entry;
                } else {
                    bw.writeBits(prefix, codeSize);
                    if (dict.size < LzwDict::MAX_CODE) {
                        dict.add(prefix, suffix);
                        if (dict.size > (1 << codeSize) && codeSize < 12) {
                            codeSize++;
                        }
                    } else {
                        // Table full -- emit clear code and reset
                        bw.writeBits(clearCode, codeSize);
                        dict.reset(clearCode);
                        codeSize = minCodeSize + 1;
                    }
                    prefix = suffix;
                }
            }

            bw.writeBits(prefix, codeSize);
            bw.writeBits(eoiCode, codeSize);
            bw.finish();
        }
    };

} // namespace meme

#endif // GIF_ENCODER_H
