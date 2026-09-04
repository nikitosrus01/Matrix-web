#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <opencv2/opencv.hpp>

const std::string RAMP_DETAILED = " .'`^\",:;Il!i><~+_-?][}{1)(|\\/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B@$";
const std::string RAMP_BLOCKS   = " ░▒▓█";
const std::string RAMP_CYBERPUNK = " 01";

enum RenderMode { DETAILED, BLOCKS, CYBERPUNK, INVERTED };

int kbhit() {
    struct termios oldt, newt;
    int ch;
    int oldf;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
    if (ch != EOF) return ch;
    return 0;
}

std::string frameToAscii(const cv::Mat& colorFrame, RenderMode mode, bool autoEqualize, bool enableColor) {
    cv::Mat grayFrame, processed;
    cv::cvtColor(colorFrame, grayFrame, cv::COLOR_BGR2GRAY);

    if (autoEqualize) {
        cv::equalizeHist(grayFrame, processed);
    } else {
        processed = grayFrame;
    }

    std::string ramp = RAMP_DETAILED;
    if (mode == BLOCKS) ramp = RAMP_BLOCKS;
    if (mode == CYBERPUNK) ramp = RAMP_CYBERPUNK;

    int width = processed.cols;
    int height = processed.rows;
    size_t charCount = ramp.size();

    std::string buffer;
    buffer.reserve((width * 20 + 1) * height);

    for (int y = 0; y < height; ++y) {
        const uint8_t* grayRow = processed.ptr<uint8_t>(y);
        const cv::Vec3b* colorRow = colorFrame.ptr<cv::Vec3b>(y);

        for (int x = 0; x < width; ++x) {
            uint8_t pixelVal = grayRow[x];
            if (mode == INVERTED) {
                pixelVal = 255 - pixelVal;
            }

            size_t index = (pixelVal * (charCount - 1)) / 255;
            char asciiChar = ramp[index];

            if (enableColor) {
                cv::Vec3b bgr = colorRow[x];
                buffer += "\033[38;2;" + std::to_string(bgr[2]) + ";" 
                                       + std::to_string(bgr[1]) + ";" 
                                       + std::to_string(bgr[0]) + "m";
                buffer += asciiChar;
            } else {
                buffer += asciiChar;
            }
        }
        if (enableColor) {
            buffer += "\033[0m"; 
        }
        buffer += '\n';
    }
    return buffer;
}

int main() {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);

    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "[Error] Не удалось открыть веб-камеру!" << std::endl;
        return -1;
    }

    int targetWidth = 120;
    int targetHeight = static_cast<int>(targetWidth * 0.45 * (9.0 / 16.0));

    cv::Mat frame, resizedColorFrame;
    RenderMode currentMode = DETAILED;
    bool autoEqualize = false; 
    bool enableColor = true;   

    std::cout << "\033[2J\033[?25l";

    auto lastTime = std::chrono::high_resolution_clock::now();
    int frameCount = 0;
    double fps = 0.0;

    while (true) {
        cap >> frame;
        if (frame.empty()) break;

        int key = kbhit();
        if (key == 'q' || key == 'Q' || key == 27) break;
        if (key == '1') currentMode = DETAILED;
        if (key == '2') currentMode = BLOCKS;
        if (key == '3') currentMode = CYBERPUNK;
        if (key == '4') currentMode = INVERTED;
        if (key == 'c' || key == 'C') enableColor = !enableColor; 
        if (key == 'e' || key == 'E') autoEqualize = !autoEqualize;
        if (key == '+' || key == '=') {
            targetWidth = std::min(220, targetWidth + 5);
            targetHeight = static_cast<int>(targetWidth * 0.45 * (9.0 / 16.0));
        }
        if (key == '-') {
            targetWidth = std::max(40, targetWidth - 5);
            targetHeight = static_cast<int>(targetWidth * 0.45 * (9.0 / 16.0));
        }

        cv::resize(frame, resizedColorFrame, cv::Size(targetWidth, targetHeight), 0, 0, cv::INTER_LINEAR);

        std::string asciiBuffer = frameToAscii(resizedColorFrame, currentMode, autoEqualize, enableColor);

        frameCount++;
        auto currentTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = currentTime - lastTime;
        if (elapsed.count() >= 1.0) {
            fps = frameCount / elapsed.count();
            frameCount = 0;
            lastTime = currentTime;
        }

        std::cout << "\033[H" << asciiBuffer;
        std::cout << "\033[0m"; 
        std::cout << "┌────────────────────────────────────────────────────────────┐\n";
        std::cout << "│ FPS: " << static_cast<int>(fps) << " | Res: " << targetWidth << "x" << targetHeight 
                  << " | Color (C): " << (enableColor ? "RGB TrueColor" : "Monochrome   ") << "  │\n";
        std::cout << "│ Mode [1-4]: " << (currentMode == DETAILED ? "Detailed" : currentMode == BLOCKS ? "Blocks  " : currentMode == CYBERPUNK ? "Matrix  " : "Inverted")
                  << " | Equalize (E): " << (autoEqualize ? "ON " : "OFF") << " │\n";
        std::cout << "│ Resize: [+/-] | Exit: [Q]                                  │\n";
        std::cout << "└────────────────────────────────────────────────────────────┘\n";
        std::cout.flush();
    }

    std::cout << "\033[0m\033[?25h\033[2J";
    return 0;
}