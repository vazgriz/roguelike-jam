#include "SimpleEngine/ImageAsset.h"

using namespace SEngine;

ImageAsset::ImageAsset(int32_t width, int32_t height, std::vector<char>&& data) {
    m_width = width;
    m_height = height;
    m_data = std::move(data);
}