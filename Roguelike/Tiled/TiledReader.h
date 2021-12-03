#pragma once
#include <string>
#include <optional>
#include <nlohmann/json.hpp>

class Tiled {
public:
    enum class Compression {
        None,
        Zlib
    };

    struct Tile {
        int32_t id;
        int32_t imageWidth;
        int32_t imageHeight;
    };

    struct Tileset {
        std::string name;
        int32_t columns;
        int32_t firstGID;
        std::string image;
        int32_t imageWidth;
        int32_t imageHeight;
        int32_t margin;
        int32_t spacing;
        int32_t tileCount;
        int32_t tileWidth;
        int32_t tileHeight;
        std::vector<Tile> tiles;

    };

    struct LayerData {
        bool flipX;
        bool flipY;
        bool flipDiagonal;
        uint32_t id;

        LayerData(uint32_t dataRaw);
    };

    struct Layer {
        std::string name;
        std::vector<LayerData> data;
        int32_t width;
        int32_t height;
        int32_t id;
    };

    struct Map {
        std::optional<std::string> backgroundColor;
        int32_t width;
        int32_t height;
        int32_t tileWidth;
        int32_t tileHeight;
        std::vector<Layer> layers;
        std::vector<Tileset> tilesets;
    };

    Tiled(const std::string& path);
    Tiled(const Tiled& other) = delete;
    Tiled& operator = (const Tiled& other) = delete;
    Tiled(Tiled&& other) = default;
    Tiled& operator = (Tiled&& other) = default;

    Map& map() { return m_map; }

private:
    Map m_map;

    void loadMap(nlohmann::json& json);
    void loadTilesets(std::vector<Tileset>& tilesets, nlohmann::json& json);
    void loadTiles(std::vector<Tile>& tiles, nlohmann::json& json);
    void loadLayers(std::vector<Layer>& layers, nlohmann::json& json);
    std::vector<uint32_t> decompress(std::vector<char>& bytes);
};