#include <windows.h>
#include <conio.h>
#include <iostream>
#include <ctime>
#include <cstdlib>

#include <fstream>
#include <vector>
#include <cstdint>

// BMP文件头部结构
#pragma pack(push, 1)  // 设置字节对齐
struct BMPHeader {
    uint16_t bfType;        // 文件类型，必须是BM
    uint32_t bfSize;        // 文件大小
    uint16_t bfReserved1;   // 保留字段，必须为0
    uint16_t bfReserved2;   // 保留字段，必须为0
    uint32_t bfOffBits;     // 图像数据的偏移量
};

struct BMPInfoHeader {
    uint32_t biSize;        // 信息头的大小
    int32_t  biWidth;       // 图像宽度
    int32_t  biHeight;      // 图像高度
    uint16_t biPlanes;      // 必须为1
    uint16_t biBitCount;    // 每像素位数，24位BMP为24
    uint32_t biCompression; // 压缩类型
    uint32_t biSizeImage;   // 图像数据大小
    int32_t  biXPelsPerMeter; // 水平分辨率
    int32_t  biYPelsPerMeter; // 垂直分辨率
    uint32_t biClrUsed;     // 使用的颜色数
    uint32_t biClrImportant; // 重要颜色数
};
#pragma pack(pop)  // 取消字节对齐设置

bool readBMP(const char* filename, uint8_t* dataArray, int expectedWidth, int expectedHeight) {
    std::ifstream file(filename, std::ios::binary);

    if (!file) {
        std::cerr << "无法打开文件: " << filename << std::endl;
        return false;
    }

    BMPHeader bmpHeader;
    BMPInfoHeader bmpInfoHeader;

    // 读取BMP文件头和信息头
    file.read(reinterpret_cast<char*>(&bmpHeader), sizeof(bmpHeader));
    file.read(reinterpret_cast<char*>(&bmpInfoHeader), sizeof(bmpInfoHeader));

    // 检查是否是24位BMP且宽度和高度是否匹配
    if (bmpInfoHeader.biBitCount != 24 || bmpInfoHeader.biWidth != expectedWidth || bmpInfoHeader.biHeight != expectedHeight) {
        std::cerr << "只支持尺寸为 " << expectedWidth << "x" << expectedHeight << " 且24位的BMP文件" << std::endl;
        return false;
    }

    // BMP文件中的行数据是4字节对齐的，因此每行末尾可能有填充字节
    int padding = (4 - (bmpInfoHeader.biWidth * 3) % 4) % 4;

    // 从文件中读取图像数据（注意BMP图像数据是从左下角开始的）
    for (int i = 0; i < bmpInfoHeader.biHeight; ++i) {
        for (int j = 0; j < bmpInfoHeader.biWidth; ++j) {
            // RGB顺序存储在uint8_t数组中，3个字节表示一个像素
            int index = ((bmpInfoHeader.biHeight - 1 - i) * bmpInfoHeader.biWidth + j) * 3;
            file.read(reinterpret_cast<char*>(&dataArray[index]), 3);
        }
        // 跳过行末的填充字节
        file.ignore(padding);
    }

    file.close();
    return true;
}


const int width = 640;
const int height = 480;


class PIXEL_MATRIX {
public:
    static const int width = 32;
    static const int height = 32;
    static const int pixelSize = 3;
    static uint8_t buffer[width * height * pixelSize];


};

uint8_t PIXEL_MATRIX::buffer[PIXEL_MATRIX::width * PIXEL_MATRIX::height * PIXEL_MATRIX::pixelSize];
PIXEL_MATRIX pixelMatrix;


void rand_fill_ws2812(PIXEL_MATRIX *pm)
{
    // 只初始化一次随机种子
    static bool seedInitialized = false;
    if (!seedInitialized) {
        srand(static_cast<unsigned int>(time(0)));
        seedInitialized = true;
    }

    for (int i = 0; i < pm->width * pm->height * pm->pixelSize; i++) {
        pm->buffer[i] = rand() % 256;
    }
}

void print_ws2812(uint8_t * screenBuffer, uint8_t * pixelBuffer)
{
    int screenBufferWidth = 640;
    int screenBufferHeight = 480;
    int pixelBufferWidth = 32;
    int pixelBufferHeight = 32;
    // 把 pixelBuffer 矩放大填充到screenBuffer 每个pixel 填充成3*3的方块到屏幕上

    int blockSize = 12; // 每个像素在屏幕上占据 3x3 的方块
    int screenPixelSize = screenBufferWidth / pixelBufferWidth; // 计算每个 pixel 在 screen 上的像素大小

    for (int y = 0; y < pixelBufferHeight; y++) {
        for (int x = 0; x < pixelBufferWidth; x++) {
            // 取得当前 pixelBuffer 的像素
            uint8_t pixelr = pixelBuffer[y * pixelBufferWidth*3 + x*3];
            uint8_t pixelg = pixelBuffer[y * pixelBufferWidth*3 + x*3+1];
            uint8_t pixelb = pixelBuffer[y * pixelBufferWidth*3 + x*3+2];

            // 将该像素放大填充到 screenBuffer 中
            for (int i = 0; i < blockSize; i++) {
                for (int j = 0; j < blockSize; j++) {
                    int screenX = x * blockSize + i;
                    int screenY = y * blockSize + j;

                    // 确保不越界
                    if (screenX < screenBufferWidth && screenY < screenBufferHeight) {
                        screenBuffer[screenY * screenBufferWidth * 3 + screenX *3] = pixelr;
                        screenBuffer[screenY * screenBufferWidth * 3 + screenX *3+1] = pixelg;
                        screenBuffer[screenY * screenBufferWidth * 3 + screenX *3+2] = pixelb;
                    }
                }
            }
        }
    }

}
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


    unsigned char frameindex = 0;
    unsigned char* pixelPage = sharedMemPixels;


    while (1) {
        rand_fill_ws2812(&pixelMatrix);

        readBMP("./bear.bmp", pixelMatrix.buffer, 32, 32);

        print_ws2812(sharedMemPixels, pixelMatrix.buffer);

        sharedMemPixels[width * height * 3] = frameindex;
        frameindex++;


        // 等待一段时间
        Sleep(50);
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
