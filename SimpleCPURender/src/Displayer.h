#pragma once

#include <vector>
#include <string>
#include <opencv2/opencv.hpp>
#include "Vertex.h"


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

    void LoadFromFrameBuffer(const std::vector<std::vector<FragmentPixel>>& framebuffer){
        int width = framebuffer.size();
        int height = framebuffer[0].size();
        front_buffer = cv::Mat(height, width, CV_8UC3);

        for(int x = 0; x < width; x++){ // here value of (x, y) satisfies right-top corner
            for(int y = 0; y < height; y++){
                front_buffer.at<cv::Vec3b>(height - 1 - y, x) = cv::Vec3b(
                    static_cast<unsigned char>(framebuffer[x][y].color.b * 255),
                    static_cast<unsigned char>(framebuffer[x][y].color.g * 255),
                    static_cast<unsigned char>(framebuffer[x][y].color.r * 255)
                );
                // printf("(%f, %f, %f) ", framebuffer[i][j].color.r, framebuffer[i][j].color.g, framebuffer[i][j].color.b);
            }
            // printf("\n");
        }
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

