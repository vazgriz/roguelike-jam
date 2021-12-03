#include "TiledReader.h"

#include <iostream>
#include <fstream>
#include <libbase64.h>
#include <zlib.h>

#include "../Decompress.h"

Tiled::LayerData::LayerData(uint32_t dataRaw) {
    id = dataRaw & 0x1FFFFFFF;
    flipX = (dataRaw & 0x80000000) != 0;
    flipY = (dataRaw & 0x40000000) != 0;
    flipDiagonal = (dataRaw & 0x20000000) != 0;
}

Tiled::Tiled(const std::string& path) {
    std::cout << "Reading Tiled map: " << path << "\n";
    std::ifstream file(path);
    nlohmann::json data;
    file >> data;

    loadMap(data);
}

void Tiled::loadMap(nlohmann::json& json) {
    m_map.width = json["width"].get<int32_t>();
    m_map.height = json["height"].get<int32_t>();

    loadTilesets(m_map.tilesets, json["tilesets"]);
    loadLayers(m_map.layers, json["layers"]);
}

void Tiled::loadTilesets(std::vector<Tileset>& tilesets, nlohmann::json& json) {
    for (auto& item : json) {
        Tileset tileset = {};
        loadTiles(tileset.tiles, item["tiles"]);

        tilesets.push_back(tileset);
    }
}

void Tiled::loadTiles(std::vector<Tile>& tiles, nlohmann::json& json) {
    for (auto& item : json) {
        Tile tile = {};
        tile.id = item["id"].get<int32_t>();

        tiles.push_back(tile);
    }
}

void Tiled::loadLayers(std::vector<Layer>& layers, nlohmann::json& json) {
    for (auto& item : json) {
        Layer layer = {};
        auto compressionJson = item["compression"];
        Compression compression = Compression::None;

        if (!compressionJson.is_null()) {
            std::string compressionString = compressionJson.get<std::string>();

            if (compressionString == "zlib") {
                compression = Compression::Zlib;
            } else {
                throw std::runtime_error("Unsupported compression type");
            }
        }

        auto encodingJson = item["encoding"];

        if (encodingJson.is_null()) {
            throw std::runtime_error("Encoding not defined (must be base64)");
        }

        if (encodingJson.get<std::string>() != "base64") {
            throw std::runtime_error("Unsupported encoding (must be base64)");
        }

        auto dataJson = item["data"];

        if (!dataJson.is_string()) {
            throw std::runtime_error("Unsupported encoding (must be base64)");
        }

        std::string dataString = dataJson.get<std::string>();
        std::vector<char> dataBytes;
        dataBytes.resize(dataString.size());
        size_t dataSize = dataBytes.size();

        base64_decode(dataString.data(), dataString.size(), dataBytes.data(), &dataSize, 0);
        dataBytes.resize(dataSize);

        std::vector<uint32_t> dataRaw;

        if (compression == Compression::Zlib) {
            dataRaw = std::move(decompress(dataBytes));
        } else {
            dataRaw.resize(dataSize / sizeof(uint32_t));
            memcpy(dataRaw.data(), dataBytes.data(), dataSize);
        }

        std::vector<LayerData> data;

        for (auto datum : dataRaw) {
            data.emplace_back(datum);
        }

        layer.data = std::move(data);

        layers.emplace_back(std::move(layer));
    }
}

std::vector<uint32_t> Tiled::decompress(std::vector<char>& bytes) {
    std::vector<char> decompressed = decompressZLIB(bytes);
    std::vector<uint32_t> result;
    result.resize(decompressed.size() / sizeof(uint32_t));
    memcpy(result.data(), decompressed.data(), decompressed.size());
    return result;
}