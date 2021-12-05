#include <iostream>

#include <SimpleEngine/SimpleEngine.h>
#include <SimpleEngine/RenderGraph/AcquireNode.h>
#include <SimpleEngine/RenderGraph/PresentNode.h>
#include <SimpleEngine/RenderGraph/TransferNode.h>
#include <SimpleEngine/FPSCounter.h>

#include "RenderNode.h"
#include "Tiled/TiledReader.h"
#include "Freecam.h"

int main() {
    SEngine::Engine engine;
    SEngine::Window window(800, 600, "Roguelike");
    SEngine::Graphics graphics(window, "Roguelike");
    SEngine::RenderGraph renderGraph(graphics.device(), 2);

    engine.setWindow(window);
    engine.setGraphics(graphics);
    engine.setRenderGraph(renderGraph);

    auto& acquireNode = renderGraph.addNode<SEngine::AcquireNode>(engine, renderGraph);
    auto& presentNode = renderGraph.addNode<SEngine::PresentNode>(engine, renderGraph, vk::PipelineStageFlagBits::eColorAttachmentOutput, acquireNode);
    auto& transferNode = renderGraph.addNode<SEngine::TransferNode>(engine, renderGraph);
    auto& renderNode = renderGraph.addNode<RenderNode>(engine, renderGraph, acquireNode, transferNode);

    renderGraph.addEdge(SEngine::RenderGraph::ImageEdge(acquireNode.imageUsage(), renderNode.imageUsage()));
    renderGraph.addEdge(SEngine::RenderGraph::ImageEdge(renderNode.imageUsage(), presentNode.imageUsage()));
    renderGraph.addEdge(SEngine::RenderGraph::BufferEdge(transferNode.bufferUsage(), renderNode.bufferUsage()));
    renderGraph.addEdge(SEngine::RenderGraph::ImageEdge(transferNode.imageUsage(), renderNode.textureUsage()));

    renderGraph.bake();

    SEngine::Camera camera(800, 600);
    camera.setZoom(32);

    SEngine::FPSCounter fpsCounter(window, "Roguelike");
    engine.addSystem(fpsCounter);

    Freecam freecam(window, camera);
    freecam.setSpeed(50);
    engine.addSystem(freecam);

    Tiled map("data/sample_map.json");
    renderNode.setCamera(camera);
    renderNode.loadMap(map);

    engine.run();
}