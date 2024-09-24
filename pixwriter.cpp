#include <windows.h>
#include <conio.h>
#include <iostream>

const int width = 640;
const int height = 480;

int main() {
    const wchar_t* sharedMemName = L"SharedMemoryName";

    // 创建共享内存
    HANDLE hMapFile = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, width * height * 3+1, sharedMemName);
    if (hMapFile == NULL) {
        MessageBoxW(NULL, L"无法创建共享内存", L"错误", MB_OK);
        return 1;
    }

    // 映射共享内存
    unsigned char* sharedMemPixels = (unsigned char*)MapViewOfFile(hMapFile, FILE_MAP_WRITE, 0, 0, width * height * 3+1);
    if (sharedMemPixels == NULL) {
        MessageBoxW(NULL, L"无法映射共享内存", L"错误", MB_OK);
        CloseHandle(hMapFile);
        return 1;
    }

    unsigned char r = 0;
    unsigned char frameindex = 0;
    unsigned char* pixelPage = sharedMemPixels;

    int dir = 0;
    while (1) {
        std::cout << "r=" << static_cast<int>(r) << "\r";

        // 填充一些像素数据
        for (int i = 0; i < width * height; ++i) {
            pixelPage[i * 3 + 0] = r;   // R
            pixelPage[i * 3 + 1] = r;   // G
            pixelPage[i * 3 + 2] = r;   // B
        }
        sharedMemPixels[width * height * 3] = frameindex;
        frameindex++;
        if (r==255) {
            dir=0;
        } else if (r==0) {
            dir=1;
        }

        if (dir==0) {
            r--;
        } else {
            r++;
        }

        // 等待一段时间
        Sleep(10);
        // 检查是否有键盘输入
        if (_kbhit()) {
            int key = _getch(); // 读取字符但不回显

            // 如果按下了'q'键（或'Q'键，取决于你是否想区分大小写）
            if (key == 'q' || key == 'Q') {
                break; // 退出循环
            }
        }
    }

    // 清理资源
    UnmapViewOfFile(sharedMemPixels);
    CloseHandle(hMapFile);

    return 0;
}
