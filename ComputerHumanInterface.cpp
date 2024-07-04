#include <iostream>
#include <Windows.h>
#include <opencv2/opencv.hpp>

struct Point
{
    int x;
    int y;
};

struct Image
{
    cv::Mat image;
};

struct Match
{
    
};

cv::Mat hwnd2mat(HWND hwnd)
{
    HDC hwindowDC, hwindowCompatibleDC;

    int height, width, srcheight, srcwidth;
    HBITMAP hbwindow;
    cv::Mat src;
    BITMAPINFOHEADER  bi;

    hwindowDC = GetDC(hwnd);
    hwindowCompatibleDC = CreateCompatibleDC(hwindowDC);
    SetStretchBltMode(hwindowCompatibleDC, COLORONCOLOR);

    RECT windowsize;    // get the height and width of the screen
    GetClientRect(hwnd, &windowsize);

    srcheight = windowsize.bottom;
    srcwidth = windowsize.right;
    height = windowsize.bottom;  //change this to whatever size you want to resize to
    width = windowsize.right;

    src.create(height, width, CV_8UC4);

    // create a bitmap
    hbwindow = CreateCompatibleBitmap(hwindowDC, width, height);
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = width;
    bi.biHeight = -height;
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    // use the previously created device context with the bitmap
    SelectObject(hwindowCompatibleDC, hbwindow);
    // copy from the window device context to the bitmap device context
    StretchBlt(hwindowCompatibleDC, 0, 0, width, height, hwindowDC, 0, 0, srcwidth, srcheight, SRCCOPY);
    GetDIBits(hwindowCompatibleDC, hbwindow, 0, height, src.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

    // avoid memory leak
    DeleteObject(hbwindow);
    DeleteDC(hwindowCompatibleDC);
    ReleaseDC(hwnd, hwindowDC);

    return src;
}

Image image_read(std::string const& path)
{
    return Image
    {
        cv::imread(path, cv::IMREAD_COLOR)
    };
}

void image_write(std::string const& path, Image const& image)
{
    cv::imwrite(path, image.image);
}

Point find(Image& screen, Image& templateImage)
{
    Image image = {};
    cv::matchTemplate(screen.image, templateImage.image, image.image, cv::TM_CCOEFF_NORMED);

    double minVal, maxVal;
    cv::Point minLoc, maxLoc;
    cv::minMaxLoc(image.image, &minVal, &maxVal, &minLoc, &maxLoc);

    return Point{ maxLoc.x, maxLoc.y };
}

enum class MouseButton
{
    Left,
    Right,
    Middle
};

void wait(DWORD const ms)
{
    Sleep(ms);
}

void mouse_set(int const x, int const y)
{
    SetCursorPos(x, y);
}

void mouse_down(MouseButton const button)
{
    INPUT inputs[2] = {};

    inputs[0].type = INPUT_MOUSE;

    switch (button)
    {
    case MouseButton::Left:
        inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        break;
    case MouseButton::Right:
        inputs[0].mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
        break;
    case MouseButton::Middle:
        inputs[0].mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
        break;
    }

    SendInput(1, inputs, sizeof(INPUT));
}

void mouse_up(MouseButton const button)
{
    INPUT inputs[2] = {};

    inputs[0].type = INPUT_MOUSE;

    switch (button)
    {
    case MouseButton::Left:
        inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTUP;
        break;
    case MouseButton::Right:
        inputs[0].mi.dwFlags = MOUSEEVENTF_RIGHTUP;
        break;
    case MouseButton::Middle:
        inputs[0].mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
        break;
    }

    SendInput(1, inputs, sizeof(INPUT));
}

void mouse_click(MouseButton const button)
{
    INPUT inputs[2] = {};

    inputs[0].type = INPUT_MOUSE;
    inputs[1].type = INPUT_MOUSE;

    switch (button)
    {
    case MouseButton::Left:
        inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
        break;
    case MouseButton::Right:
        inputs[0].mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
        inputs[1].mi.dwFlags = MOUSEEVENTF_RIGHTUP;
        break;
    case MouseButton::Middle:
        inputs[0].mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
        inputs[1].mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
        break;
    }

    SendInput(2, inputs, sizeof(INPUT));
}

void key_down(WORD const key)
{
    INPUT inputs[1] = {};

    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = key;

    SendInput(1, inputs, sizeof(INPUT));
}

void key_up(WORD const key)
{
    INPUT inputs[1] = {};

    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = key;
    inputs[0].ki.dwFlags = KEYEVENTF_KEYUP;

    SendInput(1, inputs, sizeof(INPUT));
}

void key_type(WORD const key)
{
    INPUT inputs[2] = {};

    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = key;
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = key;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

    SendInput(1, inputs, sizeof(INPUT));
}

void key_type_string(std::string const& text, DWORD const delay)
{
    for (char c : text)
    {
        key_type(c);
        wait(delay);
    }
}

int main(int argc, char* argv[])
{
    HWND hwnd = GetDesktopWindow();
    cv::Mat screen = hwnd2mat(hwnd);

    // test
    cv::imwrite("screenshot.png", screen);

    return 0;
}