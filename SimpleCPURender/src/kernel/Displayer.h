#pragma once

#include <vector>
#include <string>
#include <mutex>
#include <thread>
#include <opencv2/opencv.hpp>
#include "Primitive.h"
#include "FrameBuffer.h"


class Displayer{
public:
    Displayer() {}

    ~Displayer() {
        cv::destroyAllWindows();
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

    void LoadFromFrameBuffer(const FrameBuffer* frame_buffer) {
        LoadFromFrameBuffer8UC3(frame_buffer);
    }

    void LoadFromFrameBuffer8UC3(const FrameBuffer* frame_buffer) {
        int width = frame_buffer->GetWidth();
        int height = frame_buffer->GetHeight();
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

    void Show(int delay = 1) {
        cv::imshow("Rendered Image", buffer);
        cv::waitKey(delay);
    }

private:
    cv::Mat buffer;
};



class DoubleBufferDisplayer{
public:
    DoubleBufferDisplayer() {
        front_buffer = new cv::Mat(1, 1, CV_8UC3); // dummy buffer for displayable initialization
        back_buffer  = new cv::Mat(1, 1, CV_8UC3); // dummy buffer for displayable initialization
    }

    ~DoubleBufferDisplayer() {
        delete front_buffer;
        delete back_buffer;
        cv::destroyAllWindows();
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

    void LoadFromFrameBuffer(const FrameBuffer* frame_buffer) {
        LoadFromFrameBuffer8UC3(frame_buffer);
    }

    void LoadFromFrameBuffer8UC3(const FrameBuffer* frame_buffer) {
        int width = frame_buffer->GetWidth();
        int height = frame_buffer->GetHeight();
        *back_buffer = cv::Mat(height, width, CV_8UC3);
        
        for(int y = 0; y < height; y++){ // here value of (row, col) satisfies right-top corner
            for(int x = 0; x < width; x++){
                const glm::vec3& color = frame_buffer->GetColorAt(x, y);
                back_buffer->at<cv::Vec3b>(height - 1 - y, x) = cv::Vec3b(
                    static_cast<unsigned char>(color.b * 255),
                    static_cast<unsigned char>(color.g * 255),
                    static_cast<unsigned char>(color.r * 255)
                );
            }
        }
    }

    void Show(int delay = 1) {
        cv::imshow("Rendered Image", *front_buffer);
        cv::waitKey(delay);
    }

    void SwapBuffer() {
        std::swap(front_buffer, back_buffer);
    }

private:
    cv::Mat* front_buffer;
    cv::Mat* back_buffer;
};

