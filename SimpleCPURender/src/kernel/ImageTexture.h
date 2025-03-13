#pragma once

#include <glm/glm.hpp>
#include "utility.h"
#include "Texture.h"


enum WrapMode{
    REPEAT,
    CLAMP_TO_EDGE,
    MIRRORED_REPEAT,
    CLAMP_TO_BORDER
};

enum FilterMode{
    NEAREST,
    LINEAR
};

class ImageTexture: public Texture{
private:
    class Wrapper{
        using WrapFunc = glm::vec4 (*)(int, int, int, int, glm::vec4*, const glm::vec4&);
    public:
        Wrapper(WrapMode wrap_mode): wrap_func(GetWrapFunction(wrap_mode)) {}

        Wrapper(WrapMode wrap_mode, const glm::vec4& border_color)
            : wrap_func(GetWrapFunction(wrap_mode)), border_color(border_color) {}

        // wrap (x, y) in data[width x height]
        glm::vec4 Wrap(glm::vec4* data, int width, int height, int x, int y) const {
            return wrap_func(x, y, width, height, data, border_color);
        }

    private:
        // setup wrap function pointer
        static WrapFunc GetWrapFunction(WrapMode wrap_mode) {
            switch (wrap_mode) {
                case REPEAT:
                    return WrapRepeat;
                case CLAMP_TO_EDGE:
                    return WrapClampToEdge;
                case MIRRORED_REPEAT:
                    return WrapMirroredRepeat;
                case CLAMP_TO_BORDER:
                    return WrapClampToBorder;
            }
        }

        static glm::vec4 WrapRepeat(int x, int y, int width, int height, glm::vec4 data[], const glm::vec4& border_color);

        static glm::vec4 WrapClampToEdge(int x, int y, int width, int height, glm::vec4 data[], const glm::vec4& border_color);

        static glm::vec4 WrapMirroredRepeat(int x, int y, int width, int height, glm::vec4 data[], const glm::vec4& border_color);

        static glm::vec4 WrapClampToBorder(int x, int y, int width, int height, glm::vec4 data[], const glm::vec4& border_color);

    private:
        const WrapFunc wrap_func;
        glm::vec4 border_color;
    };

    class Filter{
        using FilterFunc = glm::vec4 (*)(const Wrapper*, glm::vec4*, int, int, const glm::vec2&);
    public:
        Filter(FilterMode filter_mode): filter_func(GetFiltrateFunction(filter_mode)) {}

        glm::vec4 Filtrate(const Wrapper* wrapper, glm::vec4* data, int width, int height, const glm::vec2& uv) const {
            return filter_func(wrapper, data, width, height, uv);
        }

    private:
        static FilterFunc GetFiltrateFunction(FilterMode filter_mode){
            switch (filter_mode) {
                case NEAREST:
                    return FiltrateNearest;
                case LINEAR:
                    return FiltrateLinear;
            }
        }

        static glm::vec4 FiltrateNearest(const Wrapper* wrapper, glm::vec4* data, int width, int height, const glm::vec2& uv);

        static glm::vec4 FiltrateLinear(const Wrapper* wrapper, glm::vec4* data, int width, int height, const glm::vec2& uv);

    private:
        const FilterFunc filter_func;
    };

public:
    // `unsigned char*` data, no border color given
    ImageTexture(unsigned char* data, int width, int height,
                 WrapMode wrap_mode = REPEAT, FilterMode filter_mode = NEAREST)
                : data(reinterpret_cast<glm::vec4*>(data)), width(width), height(height) {
        wrapper = new Wrapper(wrap_mode);
        filter = new Filter(filter_mode);
    }

    // `unsigned char*` data, with border color given
    ImageTexture(unsigned char* data, int width, int height, const glm::vec4& border_color,
                 WrapMode wrap_mode = CLAMP_TO_BORDER, FilterMode filter_mode = NEAREST)
                : data(reinterpret_cast<glm::vec4*>(data)), width(width), height(height) {
        wrapper = new Wrapper(wrap_mode, border_color);
        filter = new Filter(filter_mode);
    }

    // `glm::vec4*` data, no border color given
    ImageTexture(glm::vec4* data, int width, int height,
                 WrapMode wrap_mode = REPEAT, FilterMode filter_mode = NEAREST)
                : data(data), width(width), height(height) {
        wrapper = new Wrapper(wrap_mode);
        filter = new Filter(filter_mode);
    }

    // `glm::vec4*` data, with border color given
    ImageTexture(glm::vec4* data, int width, int height, const glm::vec4& border_color,
                 WrapMode wrap_mode = CLAMP_TO_BORDER, FilterMode filter_mode = NEAREST)
                : data(data), width(width), height(height) {
        wrapper = new Wrapper(wrap_mode, border_color);
        filter = new Filter(filter_mode);
    }

    // load image from file, no border color given
    ImageTexture(const char* filename,
                 WrapMode wrap_mode = REPEAT, FilterMode filter_mode = NEAREST) {
        LoadImage(filename);
        wrapper = new Wrapper(wrap_mode);
        filter = new Filter(filter_mode);
    }

    // load image from file, with border color given
    ImageTexture(const char* filename, const glm::vec4& border_color,
                 WrapMode wrap_mode = REPEAT, FilterMode filter_mode = NEAREST) {
        LoadImage(filename);
        wrapper = new Wrapper(wrap_mode, border_color);
        filter = new Filter(filter_mode);
    }

    virtual ~ImageTexture() {
        delete wrapper;
        delete filter;
        if (stb_loaded) FreeImage();
    }

    virtual glm::vec4 Sample(const glm::vec2& uv) const override {
        return filter->Filtrate(wrapper, data, width, height, uv);
    }

    bool LoadImage(const char* filename);

    void FreeImage();

protected:
    int width;
    int height;
    glm::vec4* data = nullptr;
    bool stb_loaded = false;

private:
    const Wrapper* wrapper = nullptr;
    const Filter* filter = nullptr;
};
