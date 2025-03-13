#pragma once

#include <vector>
#include <string>
#include <opencv2/opencv.hpp>
#include "Primitive.h"
#include "FrameBuffer.h"


class Displayer{
public:
    Displayer() {}

    void LoadFromImageFile(const std::string& img_path){
        front_buffer = cv::imread(img_path);
    }

    void TestLoad(int width, int height){
        front_buffer = cv::Mat(height, width, CV_32FC3);
        for(int i = 0; i < width; i++){
            for(int j = 0; j < height; j++){
                front_buffer.at<cv::Vec3f>(j, i) = cv::Vec3f(
                    1.0f * i / width,   // b
                    1.0f * j / height,  // g
                    0.0f                // r
                );
            }
        }
    }

    void LoadFromFrameBuffer(const FrameBuffer* frame_buffer) {
        LoadFromFrameBuffer8UC3(frame_buffer);
        // LoadFromFrameBuffer32FC3(frame_buffer);
    }

    void LoadFromFrameBuffer8UC3(const FrameBuffer* frame_buffer){
        int width = frame_buffer->GetWidth();
        int height = frame_buffer->GetHeight();
        front_buffer = cv::Mat(height, width, CV_8UC3);
        
        for(int row = 0; row < height; row++){ // here value of (row, col) satisfies right-top corner
            for(int col = 0; col < width; col++){
                const glm::vec3& color = frame_buffer->GetColorAtIndex(row, col);
                front_buffer.at<cv::Vec3b>(row, col) = cv::Vec3b(
                    static_cast<unsigned char>(color.b * 255),
                    static_cast<unsigned char>(color.g * 255),
                    static_cast<unsigned char>(color.r * 255)
                );
            }
        }
    }

    void LoadFromFrameBuffer32FC3(const FrameBuffer* frame_buffer){
        front_buffer = cv::Mat(frame_buffer->GetHeight(), frame_buffer->GetWidth(), CV_32FC3,
                                const_cast<glm::vec3*>(frame_buffer->GetColorBuffer()));
    }

    void Show(int delay = 1){
        cv::imshow("Rendered Image", front_buffer);
        cv::waitKey(delay);
    }

    void KeepShow(){
        cv::imshow("Rendered Image", front_buffer);
        cv::waitKey(0);
    }

private:
    cv::Mat front_buffer;
};

