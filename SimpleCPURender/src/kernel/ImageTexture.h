#pragma once

#include <glm/glm.hpp>
#include "utility.h"
#include "Texture.h"


namespace oit {

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
        using WrapFunc = glm::vec4 (*)(int, int, int, int, const glm::vec4*, const glm::vec4&);
    public:
        Wrapper(WrapMode wrap_mode): wrap_func(GetWrapFunction(wrap_mode)) {}

        Wrapper(WrapMode wrap_mode, const glm::vec4& border_color)
            : wrap_func(GetWrapFunction(wrap_mode)), border_color(border_color) {}

        // wrap (x, y) in data[width x height]
        glm::vec4 Wrap(const glm::vec4 data[], int width, int height, int x, int y) const {
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

        static glm::vec4 WrapRepeat(
            int x, int y, int width, int height,
            const glm::vec4 data[], const glm::vec4& border_color
        );

        static glm::vec4 WrapClampToEdge(
            int x, int y, int width, int height,
            const glm::vec4 data[], const glm::vec4& border_color
        );

        static glm::vec4 WrapMirroredRepeat(
            int x, int y, int width, int height,
            const glm::vec4 data[], const glm::vec4& border_color
        );

        static glm::vec4 WrapClampToBorder(
            int x, int y, int width, int height,
            const glm::vec4 data[], const glm::vec4& border_color
        );

    private:
        const WrapFunc wrap_func;
        glm::vec4 border_color = glm::vec4(0.0f);
    };

    class Filter{
        using FilterFunc = glm::vec4 (*)(const Wrapper&, const glm::vec4*, int, int, const glm::vec2&);
    public:
        Filter(FilterMode filter_mode): filter_func(GetFiltrateFunction(filter_mode)) {}

        glm::vec4 Filtrate(
            const Wrapper& wrapper, const glm::vec4* data,
            int width, int height, const glm::vec2& uv
        ) const {
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

        static glm::vec4 FiltrateNearest(
            const Wrapper& wrapper, const glm::vec4* data,
            int width, int height, const glm::vec2& uv
        );

        static glm::vec4 FiltrateLinear(
            const Wrapper& wrapper, const glm::vec4* data,
            int width, int height, const glm::vec2& uv
        );

    private:
        const FilterFunc filter_func;
    };

public:
    // construct with existing image data, no border color given
    // @param[in] data  image data (32 bits floating number per channel, RGBA, pixels in bottom-up order).
    //                  User should take responsibility of its memory management.
    // @param[in] wrap_mode  wrap mode, in {`REPEAT`, `CLAMP_TO_EDGE`, `MIRRORED_REPEAT`, `CLAMP_TO_BORDER`}
    // @param[in] filter_mode  filter mode, in {`NEAREST`, `LINEAR`}
    ImageTexture(
        const void* data, int width, int height,
        WrapMode wrap_mode = REPEAT, FilterMode filter_mode = NEAREST
    ):
        data(reinterpret_cast<const glm::vec4*>(data)),
        width(width), height(height),
        wrapper(wrap_mode), filter(filter_mode)
    {}

    // construct with existing image data, with border color given
    // @param[in] data  image data (32 bits floating number per channel, RGBA, pixels in bottom-up order).
    //                  User should take responsibility of its memory management.
    // @param[in] border_color  border color, only used when `wrap_mode` is `CLAMP_TO_BORDER`
    // @param[in] wrap_mode  wrap mode, in {`REPEAT`, `CLAMP_TO_EDGE`, `MIRRORED_REPEAT`, `CLAMP_TO_BORDER`}
    // @param[in] filter_mode  filter mode, in {`NEAREST`, `LINEAR`}
    ImageTexture(
        const void* data, int width, int height, const glm::vec4& border_color,
        WrapMode wrap_mode = CLAMP_TO_BORDER, FilterMode filter_mode = NEAREST
    ):
        data(reinterpret_cast<const glm::vec4*>(data)),
        width(width), height(height),
        wrapper(wrap_mode, border_color), filter(filter_mode)
    {}

    // load image from file, no border color given
    // @param[in] filename  image file name
    // @param[in] wrap_mode  wrap mode, in {`REPEAT`, `CLAMP_TO_EDGE`, `MIRRORED_REPEAT`, `CLAMP_TO_BORDER`}
    // @param[in] filter_mode  filter mode, in {`NEAREST`, `LINEAR`}
    ImageTexture(
        const char* filename,
        WrapMode wrap_mode = REPEAT, FilterMode filter_mode = NEAREST
    ):
        wrapper(wrap_mode), filter(filter_mode)
    {
        LoadImage(filename);
    }

    // load image from file, with border color given
    // @param[in] filename  image file name
    // @param[in] border_color  border color, only used when `wrap_mode` is `CLAMP_TO_BORDER`
    // @param[in] wrap_mode  wrap mode, in {`REPEAT`, `CLAMP_TO_EDGE`, `MIRRORED_REPEAT`, `CLAMP_TO_BORDER`}
    // @param[in] filter_mode  filter mode, in {`NEAREST`, `LINEAR`}
    ImageTexture(
        const char* filename, const glm::vec4& border_color,
        WrapMode wrap_mode = REPEAT, FilterMode filter_mode = NEAREST
    ):
        wrapper(wrap_mode, border_color), filter(filter_mode)
    {
        LoadImage(filename);
    }

    virtual ~ImageTexture() {
        if (stb_loaded) FreeImage();
    }

    virtual glm::vec4 Sample(const glm::vec2& uv) const override {
        return filter.Filtrate(wrapper, data, width, height, uv);
    }

    void LoadImage(const char* filename);

    void FreeImage();

protected:
    int width;
    int height;
    const glm::vec4* data = nullptr;
    bool stb_loaded = false;

private:
    Wrapper wrapper;
    Filter filter;
};

} // namespace oit
