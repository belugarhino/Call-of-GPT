#pragma once

#include "Config.h"
#include "Input.h"
#include "Scene.h"

#include <memory>

class Game {
  public:
    explicit Game(GameConfig config);
    void setScene(std::unique_ptr<Scene> newScene);
    void run();

  private:
    void processFrame();

    GameConfig config;
    std::unique_ptr<Scene> scene;
    bool paused = false;
};
