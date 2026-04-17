#ifndef MAC_GIF_H
#define MAC_GIF_H

#include <memory>               // shared_ptr
#include <string>               // string (output path)
#include <vector>               // vector (frames)
#include "GifEncoder.h"         // meme::GifEncoder (LZW encoding, file writing)
#include "MemeLayout.h"         // layout::resizePixels (frame scaling)
#include "RenderSurface.h"      // meme::RenderSurface (pixel data)

namespace meme {

    class MacGif {
    public:
        struct Frame {
            std::shared_ptr<RenderSurface> surface;
            int durationMs;
        };

        std::vector<Frame> frames;

        // Chainable - returns shared_ptr to self
        std::shared_ptr<MacGif> addFrame(const std::shared_ptr<RenderSurface>& surface, int durationMs) {
            frames.push_back({surface, durationMs});
            return std::shared_ptr<MacGif>(this, [](MacGif*){});
        }

        std::shared_ptr<MacGif> addFrame(std::vector<unsigned char> pixels, int width, int height, int durationMs) {
            return addFrame(std::make_shared<RenderSurface>(std::move(pixels), width, height), durationMs);
        }

        bool save(const std::string& path) const {
            if (frames.empty()) return false;

            // Determine output size from first frame
            int w = frames[0].surface->width;
            int h = frames[0].surface->height;

            GifEncoder enc(path, w, h);

            for (auto& frame : frames) {
                const unsigned char* pixels = frame.surface->pixels.data();
                std::vector<unsigned char> resized;
                if (frame.surface->width != w || frame.surface->height != h) {
                    resized = layout::resizePixels(frame.surface->pixels.data(), frame.surface->width,
                                                   frame.surface->height, w, h);
                    pixels = resized.data();
                }
                int delayCs = frame.durationMs / 10; // ms -> centiseconds
                if (delayCs < 1) delayCs = 1;
                enc.addFrame(pixels, delayCs);
            }

            enc.finish();
            return true;
        }

        int frameCount() const { return static_cast<int>(frames.size()); }

        std::string toString() const {
            return "<gif " + std::to_string(frames.size()) + " frames>";
        }
    };

} // namespace meme

#endif // MAC_GIF_H
