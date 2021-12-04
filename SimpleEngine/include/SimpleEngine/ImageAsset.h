#pragma once
#include<vector>
#include <stdint.h>

namespace SEngine {
class ImageAsset {
public:
    ImageAsset(int32_t width, int32_t height, std::vector<char>&& data);
    ImageAsset(const ImageAsset& other) = delete;
    ImageAsset& operator = (const ImageAsset& other) = delete;
    ImageAsset(ImageAsset&& other) = default;
    ImageAsset& operator = (ImageAsset&& other) = default;

    int32_t width() const { return m_width; }
    int32_t height() const { return m_height; }
    const char* data() const { return m_data.data(); }

private:
    int32_t m_width;
    int32_t m_height;
    std::vector<char> m_data;
};
}