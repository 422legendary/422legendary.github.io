//-lgdi32
#ifndef TURTLE_H
#define TURTLE_H

#include <windows.h>
#include <cmath>
#include <thread>
#include <chrono>
#include <mutex>
#include <atomic>
#include <vector> 

int RED[3]={255, 0, 0}, BLACK[3]={0, 0, 0}, WHITE[3]={255, 255, 255}, 
    BLUE[3]={0, 0, 255}, GREEN[3]={0, 255, 0}, PINK[3]={255, 192, 203}, 
    YELLOW[3]={255, 255, 0}, ORANGE[3]={255, 165, 0}, CYAN[3]={0, 255, 255}, 
    PURPLE[3]={128, 0, 128}, BROWN[3]={165, 42, 42}, GRAY[3]={128, 128, 128};

class Canvas {
private:
    HWND hwnd = NULL;
    HDC hdc = NULL, memDC = NULL;
    HBITMAP bmp = NULL;
    int ww = 800, wh = 600;
    int bgR = 255, bgG = 255, bgB = 255;
    int speedDelay = 0;
    std::atomic<bool> isRunning{false};
    std::thread guiThread;
    HANDLE initEvent;
    std::mutex gdiMutex;

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        Canvas* p = (Canvas*)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
        switch (msg) {
        case WM_CREATE: {
            CREATESTRUCTA* cs = (CREATESTRUCTA*)lParam;
            p = (Canvas*)cs->lpCreateParams;
            SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)p);
            break;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            if (p && p->memDC) {
                std::lock_guard<std::mutex> lock(p->gdiMutex);
                BitBlt(hdc, 0, 0, p->ww, p->wh, p->memDC, 0, 0, SRCCOPY);
            }
            EndPaint(hwnd, &ps);
            break;
        }
        case WM_DESTROY:
            if (p) p->isRunning = false;
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProcA(hwnd, msg, wParam, lParam);
        }
        return 0;
    }

    void runMessageLoop() {
        WNDCLASSA wc = {0};
        wc.lpfnWndProc = WndProc;
        wc.hInstance = GetModuleHandleA(NULL);
        wc.hCursor = LoadCursorA(NULL, (LPCSTR)IDC_ARROW);
        wc.lpszClassName = "TurtleWindow";
        if (!GetClassInfoA(GetModuleHandleA(NULL), wc.lpszClassName, &wc)) {
            RegisterClassA(&wc);
        }
        RECT rect = {0};
        rect.right = ww;
        rect.bottom = wh;
        DWORD style = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE;
        AdjustWindowRect(&rect, style, FALSE);
        hwnd = CreateWindowExA(0, wc.lpszClassName, "Turtle", style,
            CW_USEDEFAULT, CW_USEDEFAULT,
            rect.right-rect.left, rect.bottom-rect.top,
            NULL, NULL, GetModuleHandleA(NULL), this);
        
        hdc = GetDC(hwnd);
        memDC = CreateCompatibleDC(hdc);
        bmp = CreateCompatibleBitmap(hdc, ww, wh);
        SelectObject(memDC, bmp);
        isRunning = true;
        clear();
        
        SetEvent(initEvent);
        
        MSG msg;
        while (isRunning && GetMessageA(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }

public:
    static Canvas& getInstance() {
        static Canvas instance;
        return instance;
    }

    Canvas() {
        initEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        guiThread = std::thread(&Canvas::runMessageLoop, this);
        WaitForSingleObject(initEvent, INFINITE);
    }

    ~Canvas() {
        if (hwnd && isRunning) {
            PostMessage(hwnd, WM_CLOSE, 0, 0);
        }
        if (guiThread.joinable()) {
            guiThread.join();
        }
        if (memDC) DeleteDC(memDC);
        if (bmp) DeleteObject(bmp);
        if (hdc && hwnd) ReleaseDC(hwnd, hdc);
        CloseHandle(initEvent);
    }

    void drawLine(int x1, int y1, int x2, int y2, int r, int g, int b, int width) {
        if (!isRunning || !memDC) return;
        std::lock_guard<std::mutex> lock(gdiMutex);
        HPEN pen = CreatePen(PS_SOLID, width, RGB(r, g, b));
        HPEN oldPen = (HPEN)SelectObject(memDC, pen);
        MoveToEx(memDC, x1, y1, NULL);
        LineTo(memDC, x2, y2);
        SelectObject(memDC, oldPen);
        DeleteObject(pen);
    }


    void drawFilledPolygon(const std::vector<POINT>& points, int r, int g, int b) {
        if (!isRunning || !memDC || points.empty()) return;
        std::lock_guard<std::mutex> lock(gdiMutex);
        
        HBRUSH brush = CreateSolidBrush(RGB(r, g, b));
        HBRUSH oldBrush = (HBRUSH)SelectObject(memDC, brush);
        HPEN pen = (HPEN)GetStockObject(NULL_PEN); 
        HPEN oldPen = (HPEN)SelectObject(memDC, pen);
        
        Polygon(memDC, points.data(), (int)points.size());
        
        SelectObject(memDC, oldPen);
        SelectObject(memDC, oldBrush);
        DeleteObject(brush);
    }

    void drawCircle(int cx, int cy, int r, int pr, int pg, int pb, int width, bool fill, int fr, int fg, int fb) {
        if (!isRunning || !memDC) return;
        std::lock_guard<std::mutex> lock(gdiMutex);
        HPEN pen = CreatePen(PS_SOLID, width, RGB(pr, pg, pb));
        HPEN oldPen = (HPEN)SelectObject(memDC, pen);
        HBRUSH brush = fill ? CreateSolidBrush(RGB(fr, fg, fb)) : (HBRUSH)GetStockObject(NULL_BRUSH);
        HBRUSH oldBrush = (HBRUSH)SelectObject(memDC, brush);
        Ellipse(memDC, cx - r, cy - r, cx + r, cy + r);
        SelectObject(memDC, oldBrush);
        SelectObject(memDC, oldPen);
        if (fill) DeleteObject(brush);
        DeleteObject(pen);
    }

    void drawText(int x, int y, const char* text, int r, int g, int b, int fontSize) {
        if (!isRunning || !memDC || !text) return;
        std::lock_guard<std::mutex> lock(gdiMutex);
        
        LOGFONTA lf = {0};
        lf.lfHeight = -fontSize;
        lf.lfWeight = FW_NORMAL;
        lf.lfCharSet = DEFAULT_CHARSET;
        lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
        lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
        lf.lfQuality = CLEARTYPE_QUALITY;
        lf.lfPitchAndFamily = FF_DONTCARE;
        strcpy(lf.lfFaceName, "SimSun");

        HFONT hFont = CreateFontIndirectA(&lf);
        HFONT oldFont = (HFONT)SelectObject(memDC, hFont);
        COLORREF oldColor = SetTextColor(memDC, RGB(r, g, b));
        
        SetBkMode(memDC, TRANSPARENT);
        TextOutA(memDC, x, y, text, (int)strlen(text));
        
        SetBkMode(memDC, OPAQUE);
        SetTextColor(memDC, oldColor);
        SelectObject(memDC, oldFont);
        DeleteObject(hFont);
    }

    void refresh() {
        if (isRunning && hwnd) {
            std::lock_guard<std::mutex> lock(gdiMutex);
            BitBlt(hdc, 0, 0, ww, wh, memDC, 0, 0, SRCCOPY);
            if (speedDelay > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(speedDelay));
            }
        }
    }

    void show() {
        if (isRunning && hwnd) {
            UpdateWindow(hwnd);
        }
    }

    void clear() {
        if (!isRunning || !memDC) return;
        {
        std::lock_guard<std::mutex> lock(gdiMutex);
        RECT r = {0, 0, ww, wh};
        HBRUSH brush = CreateSolidBrush(RGB(bgR, bgG, bgB));
        FillRect(memDC, &r, brush);
        DeleteObject(brush);
        }
        refresh();
    }

    void setSpeed(int s) {
        if (s == 0 || s == 10) {
            speedDelay = 0;
        } else if (s == 1) {
            speedDelay = 5;
        } else {
            speedDelay = 20 - s * 2; 
        }
        
        if (speedDelay < 0) speedDelay = 0;
    }

    void setBgColor(int r, int g, int b) { bgR = r; bgG = g; bgB = b; clear(); }
    void done() {
        if (guiThread.joinable()) guiThread.join();
    }

    friend class turtle;
};

class turtle {
private:
    Canvas* canvas;
    int pr = 0, pg = 0, pb = 0; 
    int ps = 1;                 
    bool penDown = true;        
    double x = 400, y = 300;        
    double ang = 0;            
    bool isFilling = false;
    int fillR = 0, fillG = 0, fillB = 0;
    std::vector<POINT> fillPath; 

public:
    turtle() {
        canvas = &Canvas::getInstance();
    }

    ~turtle() {}

    void Goto(double x, double y) {
        this->x = 400 + x;
        this->y = 300 - y;
    }

    void forward(double d) {
        double rad = ang * 3.141592653589793 / 180.0;
        
        int steps = (int)fabs(d) / 2;
        if (steps < 1) steps = 1;
        
        double step_length = d / steps;
        double dx = step_length * cos(rad);
        double dy = -step_length * sin(rad);

        for (int i = 0; i < steps; i++) {
            double nx = x + dx;
            double ny = y + dy;
            
            if (isFilling) {
                POINT pt = {(LONG)nx, (LONG)ny};
                fillPath.push_back(pt);
            }

            if (penDown) { 
                canvas->drawLine((int)x, (int)y, (int)nx, (int)ny, pr, pg, pb, ps);
                canvas->refresh();
            }
            x = nx; 
            y = ny;
        }
    }
    void fd(double d) { forward(d); } 

    void backward(double d) { forward(-d); }
    void back(double d) { backward(d); }
    void bk(double d) { backward(d); }   

    void left(double a) { ang += a; }
    void lt(double a) { left(a); }

    void right(double a) { ang -= a; }
    void rt(double a) { right(a); }

    void setheading(double a) { ang = a; }
    void seth(double a) { setheading(a); } 

    void penup() { penDown = false; } 
    void pu() { penup(); }   
    void up() { penup(); }  

    void pendown() { penDown = true; }
    void pd() { pendown(); } 
    void down() { pendown(); } 

    void pensize(int s) { ps = s; }
    void width(int s) { pensize(s); }

    void pencolor(int r, int g, int b) { pr = r; pg = g; pb = b; }
    void pencolor(int rgb[3]) { pencolor(rgb[0], rgb[1], rgb[2]); }

    void fillcolor(int r, int g, int b) { fillR = r; fillG = g; fillB = b; }
    void fillcolor(int rgb[3]) { fillcolor(rgb[0], rgb[1], rgb[2]); } 

    void color(int r, int g, int b) {
        pencolor(r, g, b);
        fillcolor(r, g, b);
    }
    
    void color(int rgb[3]) {
        pencolor(rgb);
        fillcolor(rgb);
    }

    void bgcolor(int r, int g, int b) { canvas->setBgColor(r, g, b); }
    void bgcolor(int rgb[3]) { bgcolor(rgb[0], rgb[1], rgb[2]); } 


    void begin_fill() { 
        isFilling = true; 
        fillPath.clear();

        POINT startPt = {(LONG)x, (LONG)y};
        fillPath.push_back(startPt);
    }
    
    void end_fill() { 
        isFilling = false; 
        if (!fillPath.empty()) {
            canvas->drawFilledPolygon(fillPath, fillR, fillG, fillB);
            canvas->refresh();
        }
    }

    void circle(double r) {
        if (r < 0) r = -r;
        
        int steps = 120; 
        double step_length = (2 * 3.141592653589793 * r) / steps;
        double step_angle = 360.0 / steps;

        for (int i = 0; i < steps; i++) {
            forward(step_length);
            left(step_angle);
        }
    }

    void write(const char* text, bool move = false, const char* align = "left", int fontSize = 14) {
        int tx = (int)x;
        int ty = (int)y;
        
        if (move) {
            x += (double)strlen(text) * fontSize * 0.6;
        }
        
        canvas->drawText(tx, ty, text, pr, pg, pb, fontSize);
        canvas->refresh();
    }

    void clear() { 
        canvas->clear(); 
        isFilling = false;
        fillPath.clear();
    }

    void speed(int s) { canvas->setSpeed(s); }

    void show() { canvas->show(); }

    void done() { canvas->done(); }
};

#endif
