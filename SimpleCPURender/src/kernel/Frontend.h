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
    BuildinSingleBufferDisplayer(): mat(1, 1, CV_8UC3) {}

    virtual ~BuildinSingleBufferDisplayer() {
        if (data != nullptr) FrameBuffer::DeleteColorBuffer(data);
    }

    virtual int GetBufferNumber() const override {
        return 1;
    }

    void LoadFromImageFile(const std::string& img_path) {
        mat = cv::imread(img_path);
    }

    void TestLoad(int width, int height) {
        mat = cv::Mat(height, width, CV_32FC3);
        for(int i = 0; i < width; i++){
            for(int j = 0; j < height; j++){
                mat.at<cv::Vec3f>(j, i) = cv::Vec3f(
                    1.0f * i / width,   // b
                    1.0f * j / height,  // g
                    0.0f                // r
                );
            }
        }
    }

    virtual void LoadFromFrameBuffer(const FrameBuffer* frame_buffer) override {
        LoadFromFrameBuffer_8UC3(frame_buffer);
        // LoadFromFrameBuffer_16UC3(frame_buffer);
        // LoadFromFrameBuffer_32FC4(frame_buffer);
        // LoadFromFrameBuffer_64FC4(frame_buffer);
        // LoadFromFrameBufferDirectly(frame_buffer);
    }

    void LoadFromFrameBufferDirectly(const FrameBuffer* frame_buffer) {
        int width = frame_buffer->width;
        int height = frame_buffer->height;
        void* data = frame_buffer->GetColorBuffer();
        mat = cv::Mat(height, width, CV_32FC4, data);
    }

    void LoadFromFrameBuffer_8UC3(const FrameBuffer* frame_buffer) {
        static Format format{Format::TOP_DOWN, Format::BGR, Format::UINT8};

        if (mat.cols != frame_buffer->width || mat.rows != frame_buffer->height) {
            frame_buffer->DeleteColorBuffer(data);
            data = FrameBuffer::NewColorBuffer(frame_buffer->width, frame_buffer->height, format);
            mat = cv::Mat(frame_buffer->width, frame_buffer->height, CV_8UC3, data);
        }

        frame_buffer->WriteColorBuffer(data, format);
    }

    void LoadFromFrameBuffer_16UC3(const FrameBuffer* frame_buffer) {
        static Format format{Format::TOP_DOWN, Format::BGR, Format::UINT16};

        if (mat.cols != frame_buffer->width || mat.rows != frame_buffer->height) {
            frame_buffer->DeleteColorBuffer(data);
            data = FrameBuffer::NewColorBuffer(frame_buffer->width, frame_buffer->height, format);
            mat = cv::Mat(frame_buffer->width, frame_buffer->height, CV_16UC3, data);
        }

        frame_buffer->WriteColorBuffer(data, format);
    }

    void LoadFromFrameBuffer_32FC4(const FrameBuffer* frame_buffer) {
        static Format format{Format::TOP_DOWN, Format::BGRA, Format::FLOAT32};

        if (mat.cols != frame_buffer->width || mat.rows != frame_buffer->height) {
            frame_buffer->DeleteColorBuffer(data);
            data = FrameBuffer::NewColorBuffer(frame_buffer->width, frame_buffer->height, format);
            mat = cv::Mat(frame_buffer->width, frame_buffer->height, CV_32FC4, data);
        }

        frame_buffer->WriteColorBuffer(data, format);
    }

    void LoadFromFrameBuffer_64FC4(const FrameBuffer* frame_buffer) {
        static Format format{Format::TOP_DOWN, Format::BGRA, Format::FLOAT64};

        if (mat.cols != frame_buffer->width || mat.rows != frame_buffer->height) {
            frame_buffer->DeleteColorBuffer(data);
            data = FrameBuffer::NewColorBuffer(frame_buffer->width, frame_buffer->height, format);
            mat = cv::Mat(frame_buffer->width, frame_buffer->height, CV_64FC4, data);
        }

        frame_buffer->WriteColorBuffer(data, format);
    }

    void __LoadFromFrameBuffer_8UC3(const FrameBuffer* frame_buffer) {
        int width = frame_buffer->width;
        int height = frame_buffer->height;
        mat = cv::Mat(height, width, CV_8UC3);
        
        for(int y = 0; y < height; y++){ // here value of (row, col) satisfies right-top corner
            for(int x = 0; x < width; x++){
                const glm::vec3& color = frame_buffer->GetColorAt(x, y);
                mat.at<cv::Vec3b>(height - 1 - y, x) = cv::Vec3b(
                    static_cast<unsigned char>(color.b * 255),
                    static_cast<unsigned char>(color.g * 255),
                    static_cast<unsigned char>(color.r * 255)
                );
            }
        }
    }

    virtual void Show(int delay = 1) override {
        cv::imshow("Rendered Image", mat);
        cv::waitKey(delay);
    }

    virtual void RotateBuffer() override {}

private:
    cv::Mat mat;
    void* data = nullptr;
};


// build-in double buffer displayer
// @note  remember to delete, or it will cause crash (not only memory leak) when exit
class BuildinDoubleBufferDisplayer: public BuildinDisplayer {
public:
    BuildinDoubleBufferDisplayer(): mat_front(1, 1, CV_8UC3), mat_back(1, 1, CV_8UC3) {}

    virtual ~BuildinDoubleBufferDisplayer() {
        if (data_front != nullptr) FrameBuffer::DeleteColorBuffer(data_front);
        if (data_back != nullptr) FrameBuffer::DeleteColorBuffer(data_back);
    }

    virtual int GetBufferNumber() const override {
        return 2;
    }

    void LoadFromImageFile(const std::string& img_path) {
        mat_back = cv::imread(img_path);
    }

    void TestLoad(int width, int height) {
        mat_back = cv::Mat(height, width, CV_32FC3);
        for(int i = 0; i < width; i++){
            for(int j = 0; j < height; j++){
                mat_back.at<cv::Vec3f>(j, i) = cv::Vec3f(
                    1.0f * i / width,   // b
                    1.0f * j / height,  // g
                    0.0f                // r
                );
            }
        }
    }

    virtual void LoadFromFrameBuffer(const FrameBuffer* frame_buffer) override {
        LoadFromFrameBuffer_8UC3(frame_buffer);
    }

    void LoadFromFrameBuffer_8UC3(const FrameBuffer* frame_buffer) {
        static Format format{Format::TOP_DOWN, Format::BGR, Format::UINT8};

        if (mat_back.cols != frame_buffer->width || mat_back.rows != frame_buffer->height) {
            frame_buffer->DeleteColorBuffer(data_back);
            data_back = FrameBuffer::NewColorBuffer(frame_buffer->width, frame_buffer->height, format);
            mat_back = cv::Mat(frame_buffer->width, frame_buffer->height, CV_8UC3, data_back);
        }

        frame_buffer->WriteColorBuffer(data_back, format);
    }

    void __LoadFromFrameBuffer_8UC3(const FrameBuffer* frame_buffer) {
        int width = frame_buffer->width;
        int height = frame_buffer->height;
        mat_back = cv::Mat(height, width, CV_8UC3);
        
        for(int y = 0; y < height; y++) { // here value of (row, col) satisfies right-top corner
            for(int x = 0; x < width; x++) {
                const glm::vec3& color = frame_buffer->GetColorAt(x, y);
                mat_back.at<cv::Vec3b>(height - 1 - y, x) = cv::Vec3b(
                    static_cast<unsigned char>(color.b * 255),
                    static_cast<unsigned char>(color.g * 255),
                    static_cast<unsigned char>(color.r * 255)
                );
            }
        }
    }

    virtual void Show(int delay = 1) override {
        cv::imshow("Rendered Image", mat_front);
        cv::waitKey(delay);
    }

    virtual void RotateBuffer() override {
        SwapBuffer();
    }

    void SwapBuffer() {
        std::swap(mat_front, mat_back);
        std::swap(data_front, data_back);
    }

private:
    cv::Mat mat_front;
    cv::Mat mat_back;
    void* data_front;
    void* data_back;
};

