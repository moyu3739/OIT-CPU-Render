#pragma once

#include <glm/glm.hpp>


class Texture{
public:
    Texture() {}

    virtual ~Texture() {}

    virtual glm::vec4 Sample(const glm::vec2& uv) const = 0;
};

class ProceduralTexture: public Texture{
public:
    ProceduralTexture() {}

    virtual ~ProceduralTexture() {}

    virtual glm::vec4 Sample(const glm::vec2& uv) const = 0;
};


class CheckerTexture: public ProceduralTexture{
public:
    CheckerTexture(const glm::vec4& color1, const glm::vec4& color2, float scale)
        : color1(color1), color2(color2), scale(scale) {}

    virtual ~CheckerTexture() {}

    virtual glm::vec4 Sample(const glm::vec2& uv) const final override {
        int seg_x = static_cast<int>(uv.x * scale);
        int seg_y = static_cast<int>(uv.y * scale);
        return (seg_x + seg_y) % 2 == 0 ? color1 : color2;
    }

private:
    glm::vec4 color1;
    glm::vec4 color2;
    float scale;
};

