#include "engine/Config.h"
#include "engine/Game.h"
#include "game/GameScene.h"

#include <memory>

int main() {
    GameConfig config;
    config.screenWidth = 1280;
    config.screenHeight = 720;
    config.targetFPS = 120;
    config.windowTitle = "Call of GPT - FPS Prototype";

    Game game(config);
    game.setScene(std::make_unique<GameScene>(config));
    game.run();
    return 0;
}
