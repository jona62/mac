#ifndef MAC_GIF_H
#define MAC_GIF_H

#include <memory>
#include <string>
#include <vector>
#include "MacMeme.h"
#include "GifEncoder.h"
#include "MemeRenderer.h"

namespace meme {

    class MacGif {
    public:
        struct Frame {
            std::shared_ptr<MacMeme> meme;
            int durationMs;
        };

        std::vector<Frame> frames;

        // Chainable - returns shared_ptr to self
        std::shared_ptr<MacGif> addFrame(std::shared_ptr<MacMeme> m, int durationMs) {
            frames.push_back({m, durationMs});
            return std::shared_ptr<MacGif>(this, [](MacGif*){});
            // Note: we can't use shared_from_this here without enable_shared_from_this,
            // so we return a non-owning shared_ptr. The caller should hold onto
            // the original shared_ptr.
        }

        bool save(const std::string& path) const {
            if (frames.empty()) return false;

            // Determine output size from first frame
            int w = 0, h = 0;
            auto& first = frames[0];
            if (first.meme->width > 0) w = first.meme->width;
            if (first.meme->height > 0) h = first.meme->height;

            // If no explicit size, render first frame to discover native size
            if (w == 0 || h == 0) {
                int rw, rh;
                MemeRenderer::render(first.meme->imagePath,
                                     first.meme->topText, first.meme->bottomText,
                                     0, 0, rw, rh);
                if (w == 0) w = rw;
                if (h == 0) h = rh;
            }

            GifEncoder enc(path, w, h);

            for (auto& frame : frames) {
                int rw, rh;
                auto pixels = MemeRenderer::render(
                    frame.meme->imagePath,
                    frame.meme->topText, frame.meme->bottomText,
                    w, h, rw, rh);
                int delayCs = frame.durationMs / 10; // ms -> centiseconds
                if (delayCs < 1) delayCs = 1;
                enc.addFrame(pixels.data(), delayCs);
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
