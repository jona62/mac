#ifndef MAC_TIMELINE_H
#define MAC_TIMELINE_H

#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

namespace meme {

    class MacTimeline {
    public:
        struct Keyframe {
            int timeMs;
            std::vector<unsigned char> pixels;
            int width;
            int height;
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

        void addKeyframe(std::vector<unsigned char> pixels, int width, int height) {
            keyframes.push_back({static_cast<int>(keyframes.size()), std::move(pixels), width, height});
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

        std::vector<OutputFrame> renderFrames() const {
            std::vector<OutputFrame> output;
            if (keyframes.empty()) return output;

            int targetW = keyframes[0].width;
            int targetH = keyframes[0].height;

            for (size_t i = 0; i < keyframes.size(); i++) {
                // Add hold frame for this keyframe
                int holdMs = 0;
                if (i < holds.size()) holdMs = holds[i];
                if (holdMs <= 0) holdMs = 2000; // default 2s hold for readable text

                // Add the keyframe as a static frame
                auto resized = resizeToTarget(keyframes[i].pixels, keyframes[i].width,
                                               keyframes[i].height, targetW, targetH);
                output.push_back({resized, targetW, targetH, holdMs / 10});

                // Add transition to next keyframe if available
                if (i + 1 < keyframes.size() && i < transitions.size()) {
                    auto& trans = transitions[i];
                    int transMs = trans.durationMs;
                    if (transMs <= 0) transMs = 150;
                    int fps = 15;
                    int frameCount = std::max(2, transMs * fps / 1000);
                    int frameDelayCs = transMs / (frameCount * 10);
                    if (frameDelayCs < 1) frameDelayCs = 1;

                    auto from = resizeToTarget(keyframes[i].pixels, keyframes[i].width,
                                                keyframes[i].height, targetW, targetH);
                    auto to = resizeToTarget(keyframes[i + 1].pixels, keyframes[i + 1].width,
                                              keyframes[i + 1].height, targetW, targetH);

                    for (int f = 1; f < frameCount; f++) {
                        float t = static_cast<float>(f) / frameCount;
                        t = applyEasing(t, trans.easing);
                        auto frame = renderTransitionFrame(from, to, targetW, targetH, t, trans.type);
                        output.push_back({frame, targetW, targetH, frameDelayCs});
                    }
                }
            }

            return output;
        }

        std::string toString() const {
            return "<timeline " + std::to_string(keyframes.size()) + " keyframes>";
        }

    private:
        static std::vector<unsigned char> resizeToTarget(
                const std::vector<unsigned char>& src, int srcW, int srcH,
                int dstW, int dstH) {
            if (srcW == dstW && srcH == dstH) return src;
            std::vector<unsigned char> dst(dstW * dstH * 4);
            for (int y = 0; y < dstH; y++) {
                int sy = y * srcH / dstH;
                for (int x = 0; x < dstW; x++) {
                    int sx = x * srcW / dstW;
                    int si = (sy * srcW + sx) * 4;
                    int di = (y * dstW + x) * 4;
                    dst[di + 0] = src[si + 0];
                    dst[di + 1] = src[si + 1];
                    dst[di + 2] = src[si + 2];
                    dst[di + 3] = src[si + 3];
                }
            }
            return dst;
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
