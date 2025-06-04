#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif

#include <string>
#include <stdexcept>
#include <stb_image.h>
#include "ImageTexture.h"

using namespace oit;


glm::vec4 ImageTexture::Wrapper::WrapRepeat(
    int x, int y, int width, int height,
    const glm::vec4 data[], const glm::vec4& border_color
) {
    x = ut::EuMod(x, width);
    y = ut::EuMod(y, height);
    return data[y * width + x];
}

glm::vec4 ImageTexture::Wrapper::WrapClampToEdge(
    int x, int y, int width, int height,
    const glm::vec4 data[], const glm::vec4& border_color
) {
    x = ut::Clamp(x, 0, width - 1);
    y = ut::Clamp(y, 0, height - 1);
    return data[y * width + x];
}

glm::vec4 ImageTexture::Wrapper::WrapMirroredRepeat(
    int x, int y, int width, int height,
    const glm::vec4 data[], const glm::vec4& border_color
) {
    x = ut::MrrMod(x, width);
    y = ut::MrrMod(y, height);
    return data[y * width + x];
}

glm::vec4 ImageTexture::Wrapper::WrapClampToBorder(
    int x, int y, int width, int height,
    const glm::vec4 data[], const glm::vec4& border_color
) {
    if (x < 0 || x >= width || y < 0 || y >= height) return border_color;
    return data[y * width + x];
}

glm::vec4 ImageTexture::Filter::FiltrateNearest(
    const Wrapper& wrapper, const glm::vec4* data,
    int width, int height, const glm::vec2& uv
) {
    int x = static_cast<int>(uv.x * width);
    int y = static_cast<int>(uv.y * height);
    return wrapper.Wrap(data, width, height, x, y);
}

glm::vec4 ImageTexture::Filter::FiltrateLinear(
    const Wrapper& wrapper, const glm::vec4* data,
    int width, int height, const glm::vec2& uv
) {
    int x0 = static_cast<int>(uv.x * width);
    int y0 = static_cast<int>(uv.y * height);
    float du = uv.x * width - x0;
    float dv = uv.y * height - y0;

    int x1, y1;
    float sx, sy;
    if (du < 0.5f){
        x1 = (x0 - 1 + width) % width;
        sx = 0.5f - du;
    }
    else{
        x1 = (x0 + 1) % width;
        sx = du - 0.5f;
    }
    if (dv < 0.5f){
        y1 = (y0 - 1 + height) % height;
        sy = 0.5f - dv;
    }
    else{
        y1 = (y0 + 1) % height;
        sy = dv - 0.5f;
    }

    glm::vec4 c00 = wrapper.Wrap(data, width, height, x0, y0);
    glm::vec4 c01 = wrapper.Wrap(data, width, height, x0, y1);
    glm::vec4 c10 = wrapper.Wrap(data, width, height, x1, y0);
    glm::vec4 c11 = wrapper.Wrap(data, width, height, x1, y1);

    return (1.0f - sx) * ( (1.0f - sy) * c00 + sy * c01 )
                  + sx * ( (1.0f - sy) * c10 + sy * c11 );
}

void ImageTexture::LoadImage(const char* filename) {
    int n;
    unsigned char* src_data = stbi_load(filename, &width, &height, &n, 4);
    if (src_data == nullptr){
        throw std::runtime_error("Failed to load image: " + std::string(filename));
    }

    auto* pdata = new glm::vec4[width * height];
    // flip the image vertically
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            int idx_src = j * width + i;
            int idx_dst = (height - 1 - j) * width + i; // flip vertically
            pdata[idx_dst] = glm::vec4(
                src_data[idx_src * 4] / 255.0f,
                src_data[idx_src * 4 + 1] / 255.0f,
                src_data[idx_src * 4 + 2] / 255.0f,
                src_data[idx_src * 4 + 3] / 255.0f
            );
        }
    }
    data = pdata;

    stbi_image_free(src_data);
    stb_loaded = true;
}

void ImageTexture::FreeImage(){
    delete[] data;
}

