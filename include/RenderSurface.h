#ifndef RENDER_SURFACE_H
#define RENDER_SURFACE_H

#include <string>
#include <utility>
#include <vector>

namespace meme {

    class RenderSurface {
    public:
        std::vector<unsigned char> pixels;
        int width = 0;
        int height = 0;

        RenderSurface() = default;

        RenderSurface(std::vector<unsigned char> rgba, int w, int h)
            : pixels(std::move(rgba)), width(w), height(h) {}

        std::string toString() const {
            return "<surface " + std::to_string(width) + "x" + std::to_string(height) + ">";
        }
    };

} // namespace meme

#endif // RENDER_SURFACE_H
