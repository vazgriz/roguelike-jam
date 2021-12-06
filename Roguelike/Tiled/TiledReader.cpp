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

Tiled::Tiled(const std::string& root) {
    m_root = root + "/";
}

Tiled::Tileset& Tiled::loadTileset(const std::string& path) {
    std::ifstream file(m_root + path);
    nlohmann::json json;
    file >> json;

    return loadTileset(path, json);
}

Tiled::Tileset Tiled::loadTileset(nlohmann::json& json) {
    Tileset tileset = {};

    tileset.name = json["name"].get<std::string>();
    tileset.columns = json["columns"].get<int32_t>();
    tileset.image = json["image"].get<std::string>();
    tileset.imageWidth = json["imagewidth"].get<int32_t>();
    tileset.imageHeight = json["imageheight"].get<int32_t>();
    tileset.margin = json["margin"].get<int32_t>();
    tileset.spacing = json["spacing"].get<int32_t>();
    tileset.tileWidth = json["tilewidth"].get<int32_t>();
    tileset.tileHeight = json["tileheight"].get<int32_t>();
    tileset.tileCount = json["tilecount"].get<int32_t>();

    loadTiles(tileset.tiles, json["tiles"]);

    return tileset;
}

Tiled::Tileset& Tiled::loadTileset(const std::string& name, nlohmann::json& json) {
    auto it = m_tilesetMap.find(name);

    if (it != m_tilesetMap.end()) {
        return *it->second;
    }

    Tileset& tileset = *m_tilesets.emplace_back(std::make_unique<Tileset>());

    tileset = loadTileset(json);

    m_tilesetMap.insert({ name, &tileset });

    return tileset;
}

Tiled::Map& Tiled::loadMap(const std::string& path) {
    std::ifstream file(m_root + path);
    nlohmann::json json;
    file >> json;

    return loadMap(json);
}

Tiled::Map& Tiled::loadMap(nlohmann::json& json) {
    Map& map = *m_maps.emplace_back(std::make_unique<Map>());
    map.width = json["width"].get<int32_t>();
    map.height = json["height"].get<int32_t>();
    map.tileWidth = json["tilewidth"].get<int32_t>();
    map.tileHeight = json["tileheight"].get<int32_t>();

    loadMapTilesets(map.tilesets, json["tilesets"]);
    loadMapLayers(map.layers, json["layers"]);

    return map;
}

void Tiled::loadMapTilesets(std::vector<Tileset>& tilesets, nlohmann::json& json) {
    for (auto& item : json) {
        auto& sourcePathJSON = item["source"];

        if (sourcePathJSON.is_null()) {
            Tileset tileset = loadTileset(item);
            tilesets.push_back(tileset);
        } else {
            std::string path = sourcePathJSON.get<std::string>();
            std::ifstream file(m_root + path);
            nlohmann::json sourceJSON;
            file >> sourceJSON;

            Tileset tileset = loadTileset(path, sourceJSON);
            tileset.firstGID = item["firstgid"].get<int32_t>();

            tilesets.push_back(tileset);
        }
    }
}

void Tiled::loadTiles(std::vector<Tile>& tiles, nlohmann::json& json) {
    for (auto& item : json) {
        Tile tile = {};
        tile.id = item["id"].get<int32_t>();

        tiles.push_back(tile);
    }
}

void Tiled::loadMapLayers(std::vector<Layer>& layers, nlohmann::json& json) {
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
        layer.width = item["width"].get<int32_t>();
        layer.height = item["height"].get<int32_t>();
        layer.id = item["id"].get<int32_t>();

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