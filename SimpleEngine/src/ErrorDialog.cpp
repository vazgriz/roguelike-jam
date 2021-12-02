#include "SimpleEngine/ErrorDialog.h"

#include <iostream>
#include <SDL2/SDL.h>

using namespace SEngine;

void SEngine::ShowError(const std::string& message) {
    std::cout << message << "\n";
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", message.c_str(), nullptr);
}