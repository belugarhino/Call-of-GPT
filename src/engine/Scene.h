#pragma once

struct InputState;

class Scene {
  public:
    virtual ~Scene() = default;

    virtual void update(float dt, const InputState &input) = 0;
    virtual void fixedUpdate(float dt, const InputState &input) = 0;
    virtual void render() = 0;
};
