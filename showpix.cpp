#include <windows.h>
#include <iostream>

#define IDT_TIMER 1

// 全局变量
HANDLE hMapFile = NULL;
unsigned char* sharedMemPixels = NULL;
unsigned char* pixmapBase = NULL;
unsigned char pixmap[640*460*4];
unsigned char lastByte = 0;
HWND hwndGlobal; // 全局变量，用于在 WinMain 中访问窗口句柄

// 窗口过程函数
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            const int width = 640;
            const int height = 480;

            if (hMapFile && pixmapBase) {
                BITMAPINFO bmi = { 0 };
                bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                bmi.bmiHeader.biWidth = width;
                bmi.bmiHeader.biHeight = -height;  // 负数表示顶部为起点
                bmi.bmiHeader.biPlanes = 1;
                bmi.bmiHeader.biBitCount = 24;
                bmi.bmiHeader.biCompression = BI_RGB;

                // 在窗口上显示像素数据
                if (StretchDIBits(hdc, 0, 0, width, height, 0, 0, width, height, pixmap, &bmi, DIB_RGB_COLORS, SRCCOPY) == GDI_ERROR) {
                    std::cout <<"pix: " << (int)pixmap[0] << std::endl;
                    MessageBoxW(NULL, L"StretchDIBits调用失败", L"错误", MB_OK);
                }
            } else {
                // wchar_t message[100];
                // swprintf(message, L"hMapFile: %d  pixmapBase: %d", (int)hMapFile , (int)pixmapBase);
                // MessageBoxW(NULL, message, L"错误", MB_OK);
                // std::cout <<"hMapFile: " << (int)hMapFile << "pixmapBase" << (int)pixmapBase << std::endl;
            }

            EndPaint(hwnd, &ps);
            break;
        }
        case WM_TIMER:
            if (hMapFile && sharedMemPixels) {
                if (sharedMemPixels[640 * 480 * 3] != lastByte) {
                    lastByte = sharedMemPixels[640 * 480 * 3];
                    pixmapBase = sharedMemPixels;

                    memcpy(pixmap, pixmapBase, 640 * 480 * 3);
                    InvalidateRect(hwnd, NULL, FALSE); // 触发重绘
                }
            }
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// 程序入口点
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // 注册窗口类
    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"WindowClassName";
    RegisterClassExW(&wc);

    // 创建窗口
    hwndGlobal = CreateWindowW(L"WindowClassName", L"WindowName", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 640, 480, NULL, NULL, hInstance, NULL);

    if (hwndGlobal == NULL) {
        MessageBoxW(NULL, L"窗口创建失败", L"错误", MB_OK);
        return 0;
    }

    ShowWindow(hwndGlobal, nCmdShow);

    // 打开共享内存
    const wchar_t* sharedMemName = L"SharedMemoryName";
    hMapFile = OpenFileMappingW(FILE_MAP_READ, FALSE, sharedMemName);
    if (hMapFile == NULL) {
        MessageBoxW(NULL, L"无法打开共享内存", L"错误", MB_OK);
        return 0;
    }
    sharedMemPixels = (unsigned char*)MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, 640 * 480 * 3 + 1);

    // 设置定时器
    SetTimer(hwndGlobal, IDT_TIMER, 5, NULL);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // 清理资源
    if (sharedMemPixels) {
        UnmapViewOfFile(sharedMemPixels);
    }
    if (hMapFile) {
        CloseHandle(hMapFile);
    }

    return 0;
}

// #include <windows.h>
// #include <iostream>

// // 窗口过程函数
// LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
//     switch (msg) {
//         case WM_PAINT: {
//             PAINTSTRUCT ps;
//             HDC hdc = BeginPaint(hwnd, &ps);

//             const int width = 640;
//             const int height = 480;

//             // 打开共享内存
//             const wchar_t* sharedMemName = L"SharedMemoryName";
//             HANDLE hMapFile = OpenFileMappingW(FILE_MAP_READ, FALSE, sharedMemName);
//             if (hMapFile) {
//                 unsigned char* sharedMemPixels = (unsigned char*)MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, width * height * 3);
//                 if (sharedMemPixels) {
//                     BITMAPINFO bmi = { 0 };
//                     bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
//                     bmi.bmiHeader.biWidth = width;
//                     bmi.bmiHeader.biHeight = -height;  // 负数表示顶部为起点
//                     bmi.bmiHeader.biPlanes = 1;
//                     bmi.bmiHeader.biBitCount = 24;
//                     bmi.bmiHeader.biCompression = BI_RGB;

//                     // 在窗口上显示像素数据
//                     if (StretchDIBits(hdc, 0, 0, width, height, 0, 0, width, height, sharedMemPixels, &bmi, DIB_RGB_COLORS, SRCCOPY) == GDI_ERROR) {
//                         MessageBoxW(NULL, L"StretchDIBits调用失败", L"错误", MB_OK);
//                     }

//                     // 释放共享内存
//                     UnmapViewOfFile(sharedMemPixels);
//                 } else {
//                     MessageBoxW(NULL, L"无法映射共享内存", L"错误", MB_OK);
//                 }
//                 CloseHandle(hMapFile);
//             } else {
//                 MessageBoxW(NULL, L"无法打开共享内存", L"错误", MB_OK);
//             }

//             EndPaint(hwnd, &ps);
//             break;
//         }
//         case WM_DESTROY:
//             PostQuitMessage(0);
//             break;
//         default:
//             return DefWindowProc(hwnd, msg, wParam, lParam);
//     }
//     return 0;
// }

// // 主程序入口
// int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
//     const int width = 640;
//     const int height = 480;

//     // 注册窗口类
//     WNDCLASSW wc = { 0 };  // 使用 WNDCLASSW
//     wc.lpfnWndProc = WndProc;
//     wc.hInstance = hInstance;
//     wc.lpszClassName = L"SharedMemoryViewer";

//     RegisterClassW(&wc);  // 使用 RegisterClassW

//     // 创建窗口
//     HWND hwnd = CreateWindowExW(0, L"SharedMemoryViewer", L"Shared Memory Pixel Viewer", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, NULL, NULL, hInstance, NULL);
//     if (hwnd == NULL) {
//         MessageBoxW(NULL, L"无法创建窗口", L"错误", MB_OK);
//         return 1;
//     }

//     ShowWindow(hwnd, nCmdShow);
//     UpdateWindow(hwnd);

//     // 消息循环
//     MSG msg;
//     while (GetMessage(&msg, NULL, 0, 0)) {
//         TranslateMessage(&msg);
//         DispatchMessage(&msg);
//     }

//     return 0;
// }



