// GanttChart.h
#define UNICODE
#define _UNICODE
#pragma once

#include <vector>
#include <Windows.h>
#include <string>
#include "ProcessSchedulingSimulator.h"

// 不建议 using namespace std; 放在头文件，建议去掉

// 甘特图绘制类
class GanttChart {
public:
    // 显示甘特图窗口
    void Show(const std::vector<std::pair<int, int>>& gantt_data,
              const std::vector<ProcessPCB>& processes);

    // 构造函数
    GanttChart();

private:
    // Windows窗口消息处理回调函数
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg,
        WPARAM wParam, LPARAM lParam);

    // 绘制图形内容
    void OnPaint(HDC hdc);

    // 绘制时间轴
    void DrawTimeAxis(HDC hdc, int width, int height, int max_time);

    // 绘制进程调度条
    void DrawProcesses(HDC hdc, int width, int height,
        const std::vector<std::pair<int, int>>& gantt_data,
        int max_time);

    // 绘制图例说明
    void DrawLegend(HDC hdc, int width, int height,
        const std::vector<ProcessPCB>& processes);

    // 获取进程对应的显示颜色
    COLORREF GetProcessColor(int process_id);

    std::vector<std::pair<int, int>> m_gantt_data;  // 存储甘特图数据
    std::vector<ProcessPCB> m_processes;            // 存储进程信息
};
