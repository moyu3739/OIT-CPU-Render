#pragma once

#include <vector>
#include <string>
#include <mutex>
#include <thread>
#include <opencv2/opencv.hpp>
#include "Primitive.h"
#include "FrameBuffer.h"


class Frontend {
public:
    Frontend() {}

    virtual ~Frontend() {}

    virtual int GetBufferNumber() const = 0;

    virtual void LoadFromFrameBuffer(const FrameBuffer* frame_buffer) = 0;

    virtual void Output(unsigned long long info) = 0;

    virtual void RotateBuffer() = 0;
};

class Displayer: public Frontend {
public:
    Displayer() {}

    virtual ~Displayer() {}

    virtual void Show(int delay = 1) = 0;

    virtual void Output(unsigned long long info) override {
        Show(info);
    }
};

// base class for built-in displayer
// @note  remember to delete, or it will cause crash (not only memory leak) when exit 
class BuildinDisplayer: public Displayer {
public:
    BuildinDisplayer() {}

    virtual ~BuildinDisplayer() {
        cv::destroyAllWindows();
    };
};


// build-in single buffer displayer
// @note  remember to delete, or it will cause crash (not only memory leak) when exit
class BuildinSingleBufferDisplayer: public BuildinDisplayer {
public:
    BuildinSingleBufferDisplayer() {}

    virtual ~BuildinSingleBufferDisplayer() {
        if (data != nullptr) FrameBuffer::DeleteColorBuffer(data);
    }

    virtual int GetBufferNumber() const override {
        return 1;
    }

    void LoadFromImageFile(const std::string& img_path) {
        buffer = cv::imread(img_path);
    }

    void TestLoad(int width, int height) {
        buffer = cv::Mat(height, width, CV_32FC3);
        for(int i = 0; i < width; i++){
            for(int j = 0; j < height; j++){
                buffer.at<cv::Vec3f>(j, i) = cv::Vec3f(
                    1.0f * i / width,   // b
                    1.0f * j / height,  // g
                    0.0f                // r
                );
            }
        }
    }

    virtual void LoadFromFrameBuffer(const FrameBuffer* frame_buffer) override {
        LoadFromFrameBuffer8UC3(frame_buffer);
        // LoadFromFrameBufferDirectly(frame_buffer);
    }

    void LoadFromFrameBufferDirectly(const FrameBuffer* frame_buffer) {
        int width = frame_buffer->width;
        int height = frame_buffer->height;
        void* data = frame_buffer->GetColorBuffer();
        buffer = cv::Mat(height, width, CV_32FC4, data);
    }

    void LoadFromFrameBuffer8UC3(const FrameBuffer* frame_buffer) {
        if (buffer.cols != frame_buffer->width || buffer.rows != frame_buffer->height) {
            frame_buffer->DeleteColorBuffer(data);
            data = FrameBuffer::NewColorBuffer(frame_buffer->width, frame_buffer->height, Format::BGR, Format::UINT8);
        }

        frame_buffer->WriteColorBuffer(data, Format::TOP_DOWN, Format::BGR, Format::UINT8);
        buffer = cv::Mat(frame_buffer->width, frame_buffer->height, CV_8UC3, data);
    }

    void __LoadFromFrameBuffer8UC3(const FrameBuffer* frame_buffer) {
        int width = frame_buffer->width;
        int height = frame_buffer->height;
        buffer = cv::Mat(height, width, CV_8UC3);
        
        for(int y = 0; y < height; y++){ // here value of (row, col) satisfies right-top corner
            for(int x = 0; x < width; x++){
                const glm::vec3& color = frame_buffer->GetColorAt(x, y);
                buffer.at<cv::Vec3b>(height - 1 - y, x) = cv::Vec3b(
                    static_cast<unsigned char>(color.b * 255),
                    static_cast<unsigned char>(color.g * 255),
                    static_cast<unsigned char>(color.r * 255)
                );
            }
        }
    }

    virtual void Show(int delay = 1) override {
        cv::imshow("Rendered Image", buffer);
        cv::waitKey(delay);
    }

    virtual void RotateBuffer() override {}

private:
    cv::Mat buffer;
    void* data = nullptr;
};


// build-in double buffer displayer
// @note  remember to delete, or it will cause crash (not only memory leak) when exit
class BuildinDoubleBufferDisplayer: public BuildinDisplayer {
public:
    BuildinDoubleBufferDisplayer() {
        front_buffer = new cv::Mat(1, 1, CV_8UC3); // dummy buffer for displayable initialization
        back_buffer  = new cv::Mat(1, 1, CV_8UC3); // dummy buffer for displayable initialization
    }

    virtual ~BuildinDoubleBufferDisplayer() {
        delete front_buffer;
        delete back_buffer;
        cv::destroyAllWindows();
    }

    virtual int GetBufferNumber() const override {
        return 2;
    }

    void LoadFromImageFile(const std::string& img_path) {
        *back_buffer = cv::imread(img_path);
    }

    void TestLoad(int width, int height) {
        *back_buffer = cv::Mat(height, width, CV_32FC3);
        for(int i = 0; i < width; i++){
            for(int j = 0; j < height; j++){
                back_buffer->at<cv::Vec3f>(j, i) = cv::Vec3f(
                    1.0f * i / width,   // b
                    1.0f * j / height,  // g
                    0.0f                // r
                );
            }
        }
    }

    virtual void LoadFromFrameBuffer(const FrameBuffer* frame_buffer) override {
        LoadFromFrameBuffer8UC3(frame_buffer);
    }

    void LoadFromFrameBuffer8UC3(const FrameBuffer* frame_buffer) {
        int width = frame_buffer->width;
        int height = frame_buffer->height;
        *back_buffer = cv::Mat(height, width, CV_8UC3);
        
        for(int y = 0; y < height; y++) { // here value of (row, col) satisfies right-top corner
            for(int x = 0; x < width; x++) {
                const glm::vec3& color = frame_buffer->GetColorAt(x, y);
                back_buffer->at<cv::Vec3b>(height - 1 - y, x) = cv::Vec3b(
                    static_cast<unsigned char>(color.b * 255),
                    static_cast<unsigned char>(color.g * 255),
                    static_cast<unsigned char>(color.r * 255)
                );
            }
        }
    }

    virtual void Show(int delay = 1) override {
        cv::imshow("Rendered Image", *front_buffer);
        cv::waitKey(delay);
    }

    virtual void RotateBuffer() override {
        SwapBuffer();
    }

    void SwapBuffer() {
        std::swap(front_buffer, back_buffer);
    }

private:
    cv::Mat* front_buffer;
    cv::Mat* back_buffer;
};

