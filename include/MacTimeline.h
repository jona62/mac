#ifndef MAC_TIMELINE_H
#define MAC_TIMELINE_H

#include <algorithm>            // min, max
#include <cmath>                // sin, cos, pow (easing curves)
#include <cstring>              // memcpy (pixel blending)
#include <memory>               // shared_ptr
#include <stdexcept>            // runtime_error
#include <string>               // string (output path)
#include <vector>               // vector (keyframes, transitions)
#include "GifLimits.h"          // meme::MAX_GIF_FRAMES
#include "GifEncoder.h"         // meme::GifEncoder (LZW encoding, file writing)
#include "MemeLayout.h"         // layout::resizePixels (frame scaling)
#include "RenderSurface.h"      // meme::RenderSurface (pixel data)

namespace meme {

    class MacTimeline {
    public:
        struct Keyframe {
            int timeMs;
            std::shared_ptr<RenderSurface> surface;
        };

        struct Transition {
            int durationMs;
            std::string type;    // "crossfade", "slideLeft", "slideRight", "slideUp", "slideDown", "wipe", "fadeBlack", "zoom"
            std::string easing;  // "linear", "ease", "easeIn", "easeOut", "easeInOut"
        };

        struct HoldEvent {
            int durationMs;
        };

        // Events in order: keyframe, then optional transition/hold before next keyframe
        std::vector<Keyframe> keyframes;
        std::vector<Transition> transitions;  // transitions[i] is between keyframe[i] and keyframe[i+1]
        std::vector<int> holds;                // hold durations after each keyframe (ms)
        int loopCount = 0;  // 0 = infinite

        void addKeyframe(const std::shared_ptr<RenderSurface>& surface) {
            keyframes.push_back({static_cast<int>(keyframes.size()), surface});
        }

        void setTransition(int durationMs, const std::string& type,
                           const std::string& easing = "linear") {
            if (transitions.size() < keyframes.size()) {
                transitions.push_back({durationMs, type, easing});
            } else if (!transitions.empty()) {
                transitions.back() = {durationMs, type, easing};
            }
        }

        void addHold(int durationMs) {
            holds.push_back(durationMs);
        }

        void setLoop(int count) {
            loopCount = count;
        }

        // Render all frames for a GIF at ~30fps
        // Returns vector of RGBA frames + delay per frame (in centiseconds)
        struct OutputFrame {
            std::vector<unsigned char> pixels;
            int width;
            int height;
            int delayCs;
        };

        bool save(const std::string& path) const {
            if (keyframes.empty()) return false;

            int targetW = keyframes[0].surface->width;
            int targetH = keyframes[0].surface->height;
            auto resized = resizedKeyframes(targetW, targetH);

            GifEncoder enc(path, targetW, targetH);
            emitFrames(resized, targetW, targetH, [&](const std::vector<unsigned char>& pixels, int delayCs) {
                enc.addFrame(pixels.data(), delayCs);
            });
            enc.finish();
            return true;
        }

        std::vector<OutputFrame> renderFrames() const {
            std::vector<OutputFrame> output;
            if (keyframes.empty()) return output;

            int targetW = keyframes[0].surface->width;
            int targetH = keyframes[0].surface->height;
            auto resized = resizedKeyframes(targetW, targetH);
            emitFrames(resized, targetW, targetH, [&](const std::vector<unsigned char>& pixels, int delayCs) {
                output.push_back({pixels, targetW, targetH, delayCs});
            });

            return output;
        }

        std::string toString() const {
            return "<timeline " + std::to_string(keyframes.size()) + " keyframes>";
        }

    private:
        template <typename Emit>
        void emitFrames(const std::vector<std::vector<unsigned char>>& resized,
                        int targetW,
                        int targetH,
                        Emit emit) const {
            std::size_t emittedFrames = 0;
            auto emitChecked = [&](const std::vector<unsigned char>& pixels, int delayCs) {
                if (emittedFrames >= MAX_GIF_FRAMES) {
                    throw std::runtime_error("GIF frame limit exceeded (max " + std::to_string(MAX_GIF_FRAMES) + " frames).");
                }
                emit(pixels, delayCs);
                emittedFrames++;
            };

            for (size_t i = 0; i < keyframes.size(); i++) {
                int holdMs = (i < holds.size()) ? holds[i] : 0;
                if (holdMs <= 0) holdMs = 2000;
                emitChecked(resized[i], std::max(1, holdMs / 10));

                if (i + 1 < keyframes.size() && i < transitions.size()) {
                    auto& trans = transitions[i];
                    int transMs = trans.durationMs > 0 ? trans.durationMs : 150;
                    int fps = 15;
                    int frameCount = std::max(2, transMs * fps / 1000);
                    int frameDelayCs = std::max(1, transMs / (frameCount * 10));

                    for (int f = 1; f < frameCount; f++) {
                        float t = applyEasing(static_cast<float>(f) / frameCount, trans.easing);
                        emitChecked(renderTransitionFrame(resized[i], resized[i + 1],
                                                          targetW, targetH, t, trans.type),
                                    frameDelayCs);
                    }
                }
            }

            if (loopCount != 1 && keyframes.size() > 1) {
                size_t lastIdx = keyframes.size() - 1;
                if (lastIdx < transitions.size() && transitions[lastIdx].durationMs > 0) {
                    auto& trans = transitions[lastIdx];
                    int transMs = trans.durationMs;
                    int fps = 15;
                    int frameCount = std::max(2, transMs * fps / 1000);
                    int frameDelayCs = std::max(1, transMs / (frameCount * 10));

                    for (int f = 1; f < frameCount; f++) {
                        float t = applyEasing(static_cast<float>(f) / frameCount, trans.easing);
                        emitChecked(renderTransitionFrame(resized[lastIdx], resized[0],
                                                          targetW, targetH, t, trans.type),
                                    frameDelayCs);
                    }
                }
            }
        }

        std::vector<std::vector<unsigned char>> resizedKeyframes(int targetW, int targetH) const {
            std::vector<std::vector<unsigned char>> resized(keyframes.size());
            for (size_t i = 0; i < keyframes.size(); i++) {
                resized[i] = resizeToTarget(keyframes[i].surface->pixels,
                                            keyframes[i].surface->width,
                                            keyframes[i].surface->height,
                                            targetW, targetH);
            }
            return resized;
        }

        static std::vector<unsigned char> resizeToTarget(
                const std::vector<unsigned char>& src, int srcW, int srcH,
                int dstW, int dstH) {
            if (srcW == dstW && srcH == dstH) return src;
            return layout::resizePixels(src.data(), srcW, srcH, dstW, dstH);
        }

        // Easing functions
        static float applyEasing(float t, const std::string& easing) {
            if (easing == "ease") {
                // Cubic bezier approximation (0.25, 0.1, 0.25, 1.0)
                return t * t * (3.0f - 2.0f * t); // smoothstep
            } else if (easing == "easeIn") {
                return t * t * t;
            } else if (easing == "easeOut") {
                float u = 1.0f - t;
                return 1.0f - u * u * u;
            } else if (easing == "easeInOut") {
                return t < 0.5f ? 4.0f * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;
            } else if (easing == "bounce") {
                // easeOutBounce (Robert Penner): the transition lands, overshoots
                // slightly, settles, overshoots again, settles. Parameters are
                // the canonical ones used by most CSS/JS animation libraries.
                const float n1 = 7.5625f;
                const float d1 = 2.75f;
                if (t < 1.0f / d1) {
                    return n1 * t * t;
                } else if (t < 2.0f / d1) {
                    float u = t - 1.5f / d1;
                    return n1 * u * u + 0.75f;
                } else if (t < 2.5f / d1) {
                    float u = t - 2.25f / d1;
                    return n1 * u * u + 0.9375f;
                } else {
                    float u = t - 2.625f / d1;
                    return n1 * u * u + 0.984375f;
                }
            }
            return t; // linear (default)
        }

        static std::vector<unsigned char> renderTransitionFrame(
                const std::vector<unsigned char>& from,
                const std::vector<unsigned char>& to,
                int w, int h, float t,
                const std::string& type) {
            std::vector<unsigned char> result(w * h * 4);

            if (type == "crossfade") {
                int n = w * h * 4;
                for (int i = 0; i < n; i++) {
                    result[i] = static_cast<unsigned char>(
                        from[i] * (1.0f - t) + to[i] * t);
                }
            } else if (type == "slideLeft") {
                int offset = static_cast<int>(w * t);
                for (int y = 0; y < h; y++) {
                    for (int x = 0; x < w; x++) {
                        int di = (y * w + x) * 4;
                        int srcX = x + offset;
                        if (srcX < w) {
                            int si = (y * w + srcX) * 4;
                            std::memcpy(&result[di], &from[si], 4);
                        } else {
                            int si = (y * w + (srcX - w)) * 4;
                            std::memcpy(&result[di], &to[si], 4);
                        }
                    }
                }
            } else if (type == "slideRight") {
                int offset = static_cast<int>(w * t);
                for (int y = 0; y < h; y++) {
                    for (int x = 0; x < w; x++) {
                        int di = (y * w + x) * 4;
                        int srcX = x - offset;
                        if (srcX >= 0) {
                            int si = (y * w + srcX) * 4;
                            std::memcpy(&result[di], &from[si], 4);
                        } else {
                            int si = (y * w + (w + srcX)) * 4;
                            std::memcpy(&result[di], &to[si], 4);
                        }
                    }
                }
            } else if (type == "slideUp") {
                int offset = static_cast<int>(h * t);
                for (int y = 0; y < h; y++) {
                    for (int x = 0; x < w; x++) {
                        int di = (y * w + x) * 4;
                        int srcY = y + offset;
                        if (srcY < h) {
                            int si = (srcY * w + x) * 4;
                            std::memcpy(&result[di], &from[si], 4);
                        } else {
                            int si = ((srcY - h) * w + x) * 4;
                            std::memcpy(&result[di], &to[si], 4);
                        }
                    }
                }
            } else if (type == "slideDown") {
                int offset = static_cast<int>(h * t);
                for (int y = 0; y < h; y++) {
                    for (int x = 0; x < w; x++) {
                        int di = (y * w + x) * 4;
                        int srcY = y - offset;
                        if (srcY >= 0) {
                            int si = (srcY * w + x) * 4;
                            std::memcpy(&result[di], &from[si], 4);
                        } else {
                            int si = ((h + srcY) * w + x) * 4;
                            std::memcpy(&result[di], &to[si], 4);
                        }
                    }
                }
            } else if (type == "wipe") {
                int boundary = static_cast<int>(w * t);
                for (int y = 0; y < h; y++) {
                    for (int x = 0; x < w; x++) {
                        int di = (y * w + x) * 4;
                        if (x < boundary) {
                            std::memcpy(&result[di], &to[di], 4);
                        } else {
                            std::memcpy(&result[di], &from[di], 4);
                        }
                    }
                }
            } else if (type == "fadeBlack") {
                // Fade to black then fade in from black
                int n = w * h * 4;
                if (t < 0.5f) {
                    float fade = 1.0f - t * 2.0f; // 1→0 in first half
                    for (int i = 0; i < n; i += 4) {
                        result[i + 0] = static_cast<unsigned char>(from[i + 0] * fade);
                        result[i + 1] = static_cast<unsigned char>(from[i + 1] * fade);
                        result[i + 2] = static_cast<unsigned char>(from[i + 2] * fade);
                        result[i + 3] = 255;
                    }
                } else {
                    float fade = (t - 0.5f) * 2.0f; // 0→1 in second half
                    for (int i = 0; i < n; i += 4) {
                        result[i + 0] = static_cast<unsigned char>(to[i + 0] * fade);
                        result[i + 1] = static_cast<unsigned char>(to[i + 1] * fade);
                        result[i + 2] = static_cast<unsigned char>(to[i + 2] * fade);
                        result[i + 3] = 255;
                    }
                }
            } else if (type == "zoom") {
                // Zoom out from center of 'from', zoom in to 'to'
                float scale = t < 0.5f ? 1.0f + t : 2.0f - t; // 1→1.5→1
                auto& src = t < 0.5f ? from : to;
                int cx = w / 2, cy = h / 2;
                for (int y = 0; y < h; y++) {
                    for (int x = 0; x < w; x++) {
                        int sx = cx + static_cast<int>((x - cx) / scale);
                        int sy = cy + static_cast<int>((y - cy) / scale);
                        int di = (y * w + x) * 4;
                        if (sx >= 0 && sx < w && sy >= 0 && sy < h) {
                            int si = (sy * w + sx) * 4;
                            std::memcpy(&result[di], &src[si], 4);
                        } else {
                            result[di] = result[di+1] = result[di+2] = 0;
                            result[di+3] = 255;
                        }
                    }
                }
            } else {
                // Default: crossfade
                int n = w * h * 4;
                for (int i = 0; i < n; i++) {
                    result[i] = static_cast<unsigned char>(
                        from[i] * (1.0f - t) + to[i] * t);
                }
            }

            return result;
        }
    };

} // namespace meme

#endif // MAC_TIMELINE_H
