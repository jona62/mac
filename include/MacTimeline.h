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
            std::string type;  // "crossfade", "slideLeft", "slideRight", "slideUp", "slideDown", "wipe"
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

        void setTransition(int durationMs, const std::string& type) {
            if (transitions.size() < keyframes.size()) {
                transitions.push_back({durationMs, type});
            } else if (!transitions.empty()) {
                transitions.back() = {durationMs, type};
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
                if (holdMs <= 0) holdMs = 500; // default 500ms hold

                // Add the keyframe as a static frame
                auto resized = resizeToTarget(keyframes[i].pixels, keyframes[i].width,
                                               keyframes[i].height, targetW, targetH);
                output.push_back({resized, targetW, targetH, holdMs / 10});

                // Add transition to next keyframe if available
                if (i + 1 < keyframes.size() && i < transitions.size()) {
                    auto& trans = transitions[i];
                    int transMs = trans.durationMs;
                    if (transMs <= 0) transMs = 500;
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
