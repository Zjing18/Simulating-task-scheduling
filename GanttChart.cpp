#include "stdafx.h"
#include "GanttChart.h"
#include <Windows.h>
#include <string>
#include <sstream>
#include <vector>
#include <cmath>

using namespace std;

// HSV转RGB辅助函数
static COLORREF HSVtoRGB(double h, double s, double v) {
    double r = 0, g = 0, b = 0;
    int i = int(h * 6);
    double f = h * 6 - i;
    double p = v * (1 - s);
    double q = v * (1 - f * s);
    double t = v * (1 - (1 - f) * s);
    switch (i % 6) {
    case 0: r = v, g = t, b = p; break;
    case 1: r = q, g = v, b = p; break;
    case 2: r = p, g = v, b = t; break;
    case 3: r = p, g = q, b = v; break;
    case 4: r = t, g = p, b = v; break;
    case 5: r = v, g = p, b = q; break;
    }
    return RGB(int(r * 255), int(g * 255), int(b * 255));
}

// 构造函数
GanttChart::GanttChart() {}

// 显示甘特图窗口
void GanttChart::Show(const vector<pair<int, int>>& gantt_data,
    const vector<ProcessPCB>& processes) {
    if (gantt_data.empty()) return;

    m_gantt_data = gantt_data;
    m_processes = processes;

    // 注册窗口类（只注册一次）
    static bool registered = false;
    if (!registered) {
        WNDCLASSW wc = { 0 };
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = GetModuleHandleW(NULL);
        wc.lpszClassName = L"GanttChartWindow";
        wc.hCursor = LoadCursorW(NULL, MAKEINTRESOURCEW(32512)); // 32512 = IDC_ARROW
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        RegisterClassW(&wc);
        registered = true;
    }

    // 创建窗口
    HWND hwnd = CreateWindowExW(
        0,
        L"GanttChartWindow",
        L"Gantt Chart",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 900, 500,
        NULL, NULL, GetModuleHandleW(NULL), this);

    if (!hwnd) {
        MessageBoxW(NULL, L"Create Window Failed", L"Error", MB_ICONERROR);
        return;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

// 窗口消息处理函数
LRESULT CALLBACK GanttChart::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    GanttChart* pThis = nullptr;

    if (uMsg == WM_NCCREATE) {
        CREATESTRUCTW* pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
        pThis = reinterpret_cast<GanttChart*>(pCreate->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    }
    else {
        pThis = reinterpret_cast<GanttChart*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (pThis) {
        switch (uMsg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            pThis->OnPaint(hdc);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_SIZE:
            InvalidateRect(hwnd, NULL, TRUE);
            return 0;
        }
    }

    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

// 绘制主函数
void GanttChart::OnPaint(HDC hdc) {
    if (m_gantt_data.empty()) return;

    RECT clientRect;
    GetClientRect(WindowFromDC(hdc), &clientRect);
    int width = clientRect.right - clientRect.left;
    int height = clientRect.bottom - clientRect.top;

    HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 255));
    FillRect(hdc, &clientRect, hBrush);
    DeleteObject(hBrush);

    int max_time = 1;
    if (!m_gantt_data.empty()) {
        max_time = m_gantt_data.back().second;
        if (max_time == 0) max_time = 1;
    }

    DrawTimeAxis(hdc, width, height, max_time);
    DrawProcesses(hdc, width, height, m_gantt_data, max_time);
    DrawLegend(hdc, width, height, m_processes);
}

// 获取进程颜色
COLORREF GanttChart::GetProcessColor(int process_id) {
    double h = (process_id * 0.618033988749895);
    h = h - floor(h);
    return HSVtoRGB(h, 0.5, 0.95);
}

// 绘制时间轴
void GanttChart::DrawTimeAxis(HDC hdc, int width, int height, int max_time) {
    int margin = 50;
    int chartWidth = width - 2 * margin;

    MoveToEx(hdc, margin, height - margin, NULL);
    LineTo(hdc, width - margin, height - margin);
    MoveToEx(hdc, margin, height - margin, NULL);
    LineTo(hdc, margin, margin);

    int step = (max_time / 10) > 0 ? (max_time / 10) : 1;
    for (int t = 0; t <= max_time; t += step) {
        int x = margin + (t * chartWidth) / max_time;
        MoveToEx(hdc, x, height - margin, NULL);
        LineTo(hdc, x, height - margin + 5);

        std::wstring label = std::to_wstring(t);
        TextOutW(hdc, x - 10, height - margin + 10, label.c_str(), (int)label.length());
    }

    std::wstring title = L"Gantt Chart";
    TextOutW(hdc, width / 2 - 50, margin / 2, title.c_str(), (int)title.length());
}

// 绘制进程条
void GanttChart::DrawProcesses(HDC hdc, int width, int height,
    const std::vector<std::pair<int, int>>& gantt_data,
    int max_time) {
    int margin = 50;
    int chartWidth = width - 2 * margin;
    int barHeight = 30;
    int y = height - margin - barHeight - 30;

    int prev_time = 0;
    int prev_process = -1;

    for (size_t i = 0; i < gantt_data.size(); ++i) {
        int process_id = gantt_data[i].first;
        int time = gantt_data[i].second;

        if (prev_process != -1 && prev_time < time) {
            int x1 = margin + (prev_time * chartWidth) / max_time;
            int x2 = margin + (time * chartWidth) / max_time;

            HBRUSH brush = CreateSolidBrush(GetProcessColor(prev_process));
            HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush);
            Rectangle(hdc, x1, y, x2, y + barHeight);
            SelectObject(hdc, oldBrush);
            DeleteObject(brush);

            std::wstring label = L"P" + std::to_wstring(prev_process);
            SIZE textSize;
            GetTextExtentPoint32W(hdc, label.c_str(), (int)label.length(), &textSize);
            int label_x = x1 + (x2 - x1 - textSize.cx) / 2;
            int label_y = y + (barHeight - textSize.cy) / 2;
            if (label_x < x1 + 2) label_x = x1 + 2;
            TextOutW(hdc, label_x, label_y, label.c_str(), (int)label.length());
        }

        prev_time = time;
        prev_process = process_id;
    }
}

// 绘制图例
void GanttChart::DrawLegend(HDC hdc, int width, int height,
    const std::vector<ProcessPCB>& processes) {
    int margin = 50;
    int x = width - 150;
    int y = margin + 20;
    int boxSize = 15;
    int spacing = 25;

    TextOutW(hdc, x, y - 20, L"Legend", 6);

    for (size_t i = 0; i < processes.size(); ++i) {
        HBRUSH brush = CreateSolidBrush(GetProcessColor(processes[i].ID));
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush);
        Rectangle(hdc, x, y, x + boxSize, y + boxSize);
        SelectObject(hdc, oldBrush);
        DeleteObject(brush);

        std::wstring label = L"P" + std::to_wstring(processes[i].ID);
        TextOutW(hdc, x + boxSize + 5, y, label.c_str(), (int)label.length());

        y += spacing;
    }
}