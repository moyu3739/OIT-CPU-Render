#pragma once

#include <vector>
#include <string>
#include <mutex>
#include <thread>
#include <opencv2/opencv.hpp>
#include <windows.h>
#include <gdiplus.h>
#include "Primitive.h"
#include "FrameBuffer.h"

#pragma comment(lib, "gdiplus.lib")


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


// Windows GDI+ single buffer displayer
// @note  remember to delete, or it will cause crash when exit
// class WindowsSingleBufferDisplayer: public Displayer {
// public:
//     WindowsSingleBufferDisplayer(const std::string& windowName = "GDI+ Render", int width = 800, int height = 600)
//         : m_windowName(windowName), m_width(width), m_height(height), m_data(nullptr), m_hWnd(NULL)
//     {
//         // 初始化GDI+
//         Gdiplus::GdiplusStartupInput gdiplusStartupInput;
//         Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);
        
//         // 创建窗口
//         HINSTANCE hInstance = GetModuleHandle(NULL);
        
//         // 注册窗口类
//         WNDCLASSEX wcex = {0};
//         wcex.cbSize = sizeof(WNDCLASSEX);
//         wcex.style = CS_HREDRAW | CS_VREDRAW;
//         wcex.lpfnWndProc = WindowProc;
//         wcex.hInstance = hInstance;
//         wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
//         wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
//         wcex.lpszClassName = L"WindowsSingleBufferDisplayer";
//         RegisterClassEx(&wcex);
        
//         // 创建窗口
//         m_hWnd = CreateWindow(L"WindowsSingleBufferDisplayer", 
//                               std::wstring(m_windowName.begin(), m_windowName.end()).c_str(),
//                               WS_OVERLAPPEDWINDOW,
//                               CW_USEDEFAULT, CW_USEDEFAULT, 
//                               m_width, m_height, 
//                               NULL, NULL, hInstance, NULL);
        
//         if (m_hWnd)
//         {
//             // 存储this指针以便在窗口过程中使用
//             SetWindowLongPtr(m_hWnd, GWLP_USERDATA, (LONG_PTR)this);
//             ShowWindow(m_hWnd, SW_SHOW);
//             UpdateWindow(m_hWnd);
//         }
        
//         // 创建位图
//         m_pBitmap = new Gdiplus::Bitmap(m_width, m_height, PixelFormat32bppARGB);
//     }

//     virtual ~WindowsSingleBufferDisplayer() {
//         if (m_data != nullptr) FrameBuffer::DeleteColorBuffer(m_data);
        
//         if (m_pBitmap) {
//             delete m_pBitmap;
//             m_pBitmap = nullptr;
//         }
        
//         if (m_hWnd) {
//             DestroyWindow(m_hWnd);
//             m_hWnd = NULL;
//         }
        
//         // 关闭GDI+
//         Gdiplus::GdiplusShutdown(m_gdiplusToken);
//     }

//     virtual int GetBufferNumber() const override {
//         return 1;
//     }

//     virtual void LoadFromFrameBuffer(const FrameBuffer* frame_buffer) override {
//         static Format format{Format::TOP_DOWN, Format::BGRA, Format::UINT8};

//         if (m_width != frame_buffer->width || m_height != frame_buffer->height) {
//             m_width = frame_buffer->width;
//             m_height = frame_buffer->height;
            
//             // 调整窗口大小
//             SetWindowPos(m_hWnd, NULL, 0, 0, m_width, m_height, SWP_NOMOVE | SWP_NOZORDER);
            
//             // 重新创建位图
//             if (m_pBitmap) {
//                 delete m_pBitmap;
//             }
//             m_pBitmap = new Gdiplus::Bitmap(m_width, m_height, PixelFormat32bppARGB);
            
//             // 重新分配缓冲区
//             if (m_data != nullptr) {
//                 frame_buffer->DeleteColorBuffer(m_data);
//             }
//             m_data = FrameBuffer::NewColorBuffer(m_width, m_height, format);
//         }

//         // 从FrameBuffer写入数据
//         frame_buffer->WriteColorBuffer(m_data, format);
        
//         // 更新位图
//         Gdiplus::BitmapData bitmapData;
//         Gdiplus::Rect rect(0, 0, m_width, m_height);
        
//         m_pBitmap->LockBits(&rect, Gdiplus::ImageLockModeWrite, PixelFormat32bppARGB, &bitmapData);
        
//         // 复制数据到位图
//         unsigned char* src = static_cast<unsigned char*>(m_data);
//         unsigned char* dst = static_cast<unsigned char*>(bitmapData.Scan0);
        
//         for (int y = 0; y < m_height; y++) {
//             memcpy(dst + y * bitmapData.Stride, src + y * m_width * 4, m_width * 4);
//         }
        
//         m_pBitmap->UnlockBits(&bitmapData);
        
//         // 触发重绘
//         InvalidateRect(m_hWnd, NULL, FALSE);
//     }

//     virtual void Show(int delay = 1) override {
//         // 处理消息循环
//         MSG msg;
//         while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
//             TranslateMessage(&msg);
//             DispatchMessage(&msg);
//         }
        
//         // 模拟延迟
//         if (delay > 0) {
//             Sleep(delay);
//         }
//     }

//     virtual void Output(unsigned long long info) override {
//         Show(info);
//     }

//     virtual void RotateBuffer() override {
//         // 单缓冲显示器无需轮换缓冲区
//     }

// private:
//     std::string m_windowName;
//     int m_width;
//     int m_height;
//     void* m_data;
//     HWND m_hWnd;
//     ULONG_PTR m_gdiplusToken;
//     Gdiplus::Bitmap* m_pBitmap;

//     // 窗口过程
//     static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
//         WindowsSingleBufferDisplayer* pThis = reinterpret_cast<WindowsSingleBufferDisplayer*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
        
//         switch (message) {
//         case WM_PAINT:
//             if (pThis && pThis->m_pBitmap) {
//                 PAINTSTRUCT ps;
//                 HDC hdc = BeginPaint(hWnd, &ps);
                
//                 // 创建Graphics对象用于绘制
//                 Gdiplus::Graphics graphics(hdc);
//                 graphics.DrawImage(pThis->m_pBitmap, 0, 0);
                
//                 EndPaint(hWnd, &ps);
//             }
//             return 0;
            
//         case WM_DESTROY:
//             PostQuitMessage(0);
//             return 0;
//         }
        
//         return DefWindowProc(hWnd, message, wParam, lParam);
//     }
// };

