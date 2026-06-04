#include <windows.h>
#include <vector>
#include <fstream>
#include <commdlg.h>
#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib")

// ГЕОМЕТРИЯ СЕНЬОРА
#define COLOR_TITLEBAR  RGB(25, 35, 90)   
#define COLOR_SIDEBAR   RGB(15, 15, 30)   
#define COLOR_CANVAS    RGB(230, 230, 240) 
#define HEADER_HEIGHT   50  
#define SIDEBAR_WIDTH   150 
#define BTN_HEIGHT      45

struct Point { int x; int y; bool start; COLORREF color; int size; };
std::vector<Point> points;
bool isDrawing = false;
int currentTool = 0;
COLORREF brushColor = RGB(0, 120, 215);
static COLORREF acrCustClr[16]; // Массив на 16 цветов

void ChooseBrushColor(HWND hwnd) {
    CHOOSECOLOR cc; ZeroMemory(&cc, sizeof(cc));
    cc.lStructSize = sizeof(cc); cc.hwndOwner = hwnd;
    cc.lpCustColors = acrCustClr; cc.rgbResult = brushColor;
    cc.Flags = CC_FULLOPEN | CC_RGBINIT;
    if (ChooseColor(&cc)) { brushColor = cc.rgbResult; currentTool = 0; InvalidateRect(hwnd, NULL, TRUE); }
}

void SaveWithDialogBMP(HWND hwnd) {
    char fileName[MAX_PATH] = "moy_art_final.bmp";
    OPENFILENAME ofn; ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = "Точечный рисунок (*.bmp потом переделай в пнг)\0*.bmp\0"; ofn.lpstrFile = fileName; ofn.nMaxFile = MAX_PATH;
    if (GetSaveFileName(&ofn)) {
        RECT r; GetClientRect(hwnd, &r);
        int w = r.right - SIDEBAR_WIDTH; int h = r.bottom - HEADER_HEIGHT;
        HDC hdc = GetDC(hwnd); HDC mDC = CreateCompatibleDC(hdc);
        HBITMAP hbm = CreateCompatibleBitmap(hdc, w, h); SelectObject(mDC, hbm);
        BitBlt(mDC, 0, 0, w, h, hdc, SIDEBAR_WIDTH, HEADER_HEIGHT, SRCCOPY);
        BITMAPFILEHEADER bfh = { 0x4d42, (DWORD)(54 + w * h * 4), 0, 0, 54 };
        BITMAPINFOHEADER bih = { sizeof(BITMAPINFOHEADER), w, h, 1, 32, BI_RGB };
        std::ofstream f(ofn.lpstrFile, std::ios::out | std::ios::binary);
        if (f.is_open()) {
            f.write((char*)&bfh, sizeof(bfh)); f.write((char*)&bih, sizeof(bih));
            char* data = new char[w * h * 4];
            GetDIBits(mDC, hbm, 0, h, data, (BITMAPINFO*)&bih, DIB_RGB_COLORS);
            f.write(data, w * h * 4);
            f.close(); delete[] data; MessageBox(hwnd, "ГОТОВО!", "БАЗА", MB_OK);
        }
        DeleteObject(hbm); DeleteDC(mDC); ReleaseDC(hwnd, hdc);
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
        COLORREF tColor = COLOR_TITLEBAR;
        DwmSetWindowAttribute(hwnd, DWMWA_CAPTION_COLOR, &tColor, sizeof(tColor));
        return 0;
    }
    case WM_LBUTTONDOWN: {
        int x = LOWORD(lParam); int y = HIWORD(lParam);
        if (x < SIDEBAR_WIDTH && y > HEADER_HEIGHT) {
            int topStart = HEADER_HEIGHT + 15;
            for (int i = 0; i < 4; i++) {
                int top = topStart + i * (BTN_HEIGHT + 10);
                if (y > top && y < top + BTN_HEIGHT) {
                    if (i == 0) currentTool = 0;
                    else if (i == 1) currentTool = 1;
                    else if (i == 2) SaveWithDialogBMP(hwnd);
                    else if (i == 3) ChooseBrushColor(hwnd);
                    InvalidateRect(hwnd, NULL, TRUE); break;
                }
            }
        }
        else if (x > SIDEBAR_WIDTH && y > HEADER_HEIGHT) {
            isDrawing = true;
            points.push_back({ x, y, true, (currentTool == 0 ? brushColor : COLOR_CANVAS), (currentTool == 0 ? 8 : 45) });
        }
        return 0;
    }
    case WM_LBUTTONUP: isDrawing = false; return 0;
    case WM_MOUSEMOVE: {
        int x = LOWORD(lParam); int y = HIWORD(lParam);
        if (isDrawing && x > SIDEBAR_WIDTH && y > HEADER_HEIGHT) {
            points.push_back({ x, y, false, (currentTool == 0 ? brushColor : COLOR_CANVAS), (currentTool == 0 ? 8 : 45) });
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC hdc = BeginPaint(hwnd, &ps);
        RECT r; GetClientRect(hwnd, &r); HDC mDC = CreateCompatibleDC(hdc);
        HBITMAP hbm = CreateCompatibleBitmap(hdc, r.right, r.bottom); SelectObject(mDC, hbm);
        HBRUSH hCB = CreateSolidBrush(COLOR_CANVAS); FillRect(mDC, &r, hCB); DeleteObject(hCB);

        for (size_t i = 1; i < points.size(); i++) {
            if (!points[i].start) {
                HPEN p = CreatePen(PS_SOLID, points[i].size, points[i].color);
                SelectObject(mDC, p); MoveToEx(mDC, points[i - 1].x, points[i - 1].y, NULL);
                LineTo(mDC, points[i].x, points[i].y); DeleteObject(p);
            }
        }

        HBRUSH hT = CreateSolidBrush(COLOR_TITLEBAR); RECT tR = { 0, 0, r.right, HEADER_HEIGHT }; FillRect(mDC, &tR, hT); DeleteObject(hT);
        HBRUSH hP = CreateSolidBrush(COLOR_SIDEBAR); RECT pR = { 0, HEADER_HEIGHT, SIDEBAR_WIDTH, r.bottom }; FillRect(mDC, &pR, hP); DeleteObject(hP);

        HPEN hW = CreatePen(PS_SOLID, 2, RGB(255, 255, 255)); SelectObject(mDC, hW);
        int topStart = HEADER_HEIGHT + 15;
        for (int i = 0; i < 4; i++) {
            int top = topStart + i * (BTN_HEIGHT + 10);
            if (i == currentTool && i < 2) {
                HBRUSH hB = CreateSolidBrush(RGB(50, 70, 130)); RECT bR = { 10, top, SIDEBAR_WIDTH - 10, top + BTN_HEIGHT };
                FillRect(mDC, &bR, hB); DeleteObject(hB);
            }
            int midX = SIDEBAR_WIDTH / 2; int midY = top + BTN_HEIGHT / 2;
            if (i == 0) { // ИГОЛКА КИСТИ (БЕЗ КВАДРАТОВ!)
                MoveToEx(mDC, midX - 15, midY + 10, NULL); LineTo(mDC, midX + 15, midY - 10);
                Ellipse(mDC, midX + 10, midY - 15, midX + 20, midY - 5);
            }
            else if (i == 1) { // ЛАСТИК (БЕЗ КВАДРАТОВ!)
                MoveToEx(mDC, midX - 15, midY - 10, NULL); LineTo(mDC, midX + 15, midY - 10);
                LineTo(mDC, midX + 15, midY + 10); LineTo(mDC, midX - 15, midY + 10);
                LineTo(mDC, midX - 15, midY - 10); MoveToEx(mDC, midX - 5, midY - 10, NULL); LineTo(mDC, midX - 5, midY + 10);
            }
            else if (i == 2) { // ДИСКЕТА (БЕЗ КВАДРАТОВ!)
                MoveToEx(mDC, midX - 15, midY - 15, NULL); LineTo(mDC, midX + 15, midY - 15);
                LineTo(mDC, midX + 15, midY + 15); LineTo(mDC, midX - 15, midY + 15);
                LineTo(mDC, midX - 15, midY - 15); Rectangle(mDC, midX - 8, midY - 15, midX + 8, midY - 5);
            }
            else if (i == 3) { // КРУГ ЦВЕТА
                HBRUSH hC = CreateSolidBrush(brushColor); SelectObject(mDC, hC);
                Ellipse(mDC, midX - 18, midY - 18, midX + 18, midY + 18); DeleteObject(hC);
            }
        }
        DeleteObject(hW); BitBlt(hdc, 0, 0, r.right, r.bottom, mDC, 0, 0, SRCCOPY);
        DeleteObject(hbm); DeleteDC(mDC); EndPaint(hwnd, &ps); return 0;
    }
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE h, HINSTANCE hp, LPSTR l, int n) {
    WNDCLASS wc = { 0 }; wc.lpfnWndProc = WindowProc; wc.hInstance = h; wc.lpszClassName = "Abobe";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW); RegisterClass(&wc);
    HWND hw = CreateWindowEx(0, "Abobe", "ABOBE ICONS FINAL REVENGE", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1200, 900, NULL, NULL, h, NULL);
    ShowWindow(hw, n); MSG m; while (GetMessage(&m, NULL, 0, 0)) { TranslateMessage(&m); DispatchMessage(&m); }
    return 0;
}
