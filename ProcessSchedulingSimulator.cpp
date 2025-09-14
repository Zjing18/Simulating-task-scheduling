#define UNICODE
#define _UNICODE
#include "stdafx.h"
#include "ProcessSchedulingSimulator.h"
#include "GanttChart.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <memory>
#include <vector>
#include <string>
#include <windows.h>
#include <io.h>
#include <fcntl.h>

using namespace std;

// 进程状态字符串映射
const wchar_t* StateStrings[] = { L"执行", L"就绪", L"完成", L"未到达", L"阻塞" };

// 构造函数
ProcessScheduler::ProcessScheduler() : running_process(nullptr) {}

// 析构函数
ProcessScheduler::~ProcessScheduler() {}

// 主入口
void ProcessScheduler::Run() {
    InputProcesses();  // 输入进程信息
    PrintAll(-1);      // 打印初始状态

    int policy = SelectPolicy();  // 选择调度算法
    switch (policy) {
    case 1: FCFS(); break;
    case 2: RoundRobin(); break;
    case 3: DynamicPriority(); break;
    case 4: SJF(); break;
    default: FCFS(); break;
    }

    ShowGanttChart();  // 显示甘特图
    PrintStatistics(); // 输出统计信息
}

// 显示甘特图
void ProcessScheduler::ShowGanttChart() {
    GanttChart chart;
    chart.Show(gantt_data, finish_queue);
}

// 比较到达时间
bool ProcessScheduler::CompareArriveTime(const ProcessPCB& a, const ProcessPCB& b) {
    return a.arrive_time < b.arrive_time;
}

// 比较优先级
bool ProcessScheduler::ComparePriority(const ProcessPCB& a, const ProcessPCB& b) {
    if (a.priority != b.priority) {
        return a.priority > b.priority;
    } else {
        return a.arrive_time < b.arrive_time;
    }
}

// 选择调度算法
int ProcessScheduler::SelectPolicy() {
    wcout << L"\n请选择调度算法：\n";
    wcout << L"1. 优先级调度(FCFS)\n";
    wcout << L"2. 时间片轮转(Round-Robin)\n";
    wcout << L"3. 动态优先级(DynamicPriority)\n";
    wcout << L"4. 最短作业优先(SJF)\n";
    wcout << L"5. 高响应比优先(HRRN)\n";
    wcout << L"6. 最短剩余时间优先(SRTF)\n";
    int n;
    wcout << L"请输入算法编号: ";
    while (wcin >> n) {
        if (n < 1 || n > 6) {
            wcout << L"请输入有效编号(1-6): ";
        } else {
            break;
        }
    }
    return n;
}

// 输入进程信息
void ProcessScheduler::InputProcesses() {
    int num;
    wcout << L"请输入进程数量: ";
    wcin >> num;

    for (int i = 1; i <= num; i++) {
        ProcessPCB pro;
        wcout << L"\n请输入第" << i << L"个进程的信息（进程名 到达时间 服务时间 优先级 IO开始时间 IO阻塞时间）:\n";
        wcin >> pro.name >> pro.arrive_time >> pro.service_time >> pro.priority >> pro.io_start >> pro.io_time;

        // 初始化
        pro.ID = i;
        pro.all_time = pro.service_time;
        pro.cpu_time = 0;
        pro.start_time = -1;
        pro.end_time = -1;
        pro.wait_time = 0;
        pro.response_time = -1;
        pro.turnaround_time = 0;
        pro.state = Unarrive;
        pro.io_count = (pro.io_time > 0) ? 1 : 0;
        arrive_queue.push_back(pro);
    }

    // 按到达时间排序
    sort(arrive_queue.begin(), arrive_queue.end(),
        [this](const ProcessPCB& a, const ProcessPCB& b) {
            return CompareArriveTime(a, b);
        });
}

// 到达队列进程转入就绪队列
void ProcessScheduler::MoveArrivedToReady(int current_time) {
    while (!arrive_queue.empty()) {
        ProcessPCB& pro = arrive_queue.front();
        if (pro.arrive_time <= current_time) {
            pro.state = Ready;
            ready_queue.push_back(pro);
            arrive_queue.erase(arrive_queue.begin());
        } else {
            break;
        }
    }
}

// 更新阻塞队列
void ProcessScheduler::UpdateBlockedQueue() {
    for (auto it = blocked_queue.begin(); it != blocked_queue.end(); ) {
        it->io_time--;
        if (it->io_time <= 0) {
            it->state = Ready;
            ready_queue.push_back(*it);
            it = blocked_queue.erase(it);
        } else {
            ++it;
        }
    }
}

// 打印单个进程信息
void ProcessScheduler::PrintProcess(const ProcessPCB* pro) {
    if (!pro) return;

    wcout << setw(4) << pro->ID
        << setw(10) << pro->name
        << setw(10) << pro->arrive_time
        << setw(10) << pro->service_time
        << setw(8) << pro->priority
        << setw(10) << StateStrings[pro->state];

    if (pro->start_time == -1) {
        wcout << setw(10) << L"--" << setw(10) << L"--" << setw(10) << L"--";
    } else {
        if (pro->end_time == -1) {
            wcout << setw(10) << pro->start_time << setw(10) << L"--" << setw(10) << pro->all_time;
        } else {
            wcout << setw(10) << pro->start_time << setw(10) << pro->end_time << setw(10) << pro->all_time;
        }
    }

    if (pro->state == Finish) {
        float weighted_time = (float)(pro->end_time - pro->arrive_time) / (float)pro->service_time;
        wcout << setw(10) << (pro->end_time - pro->arrive_time)
            << setw(10) << fixed << setprecision(2) << weighted_time
            << setw(10) << pro->wait_time
            << setw(10) << pro->response_time << L"\n";
    } else {
        wcout << setw(10) << L"--" << setw(10) << L"--"
            << setw(10) << pro->wait_time
            << setw(10) << pro->response_time << L"\n";
    }
}

// 打印所有进程信息
void ProcessScheduler::PrintAll(int current) {
    if (current == -1) {
        wcout << L"\n进程初始状态：\n";
    } else {
        wcout << L"\n当前时间：" << current << L"\n";
    }

    wcout << L"进程ID|进程名|到达时间|服务时间|优先级|  状态  |开始时间|结束时间|剩余时间|周转时间|带权周转|等待时间|响应时间\n";

    if (running_process) {
        PrintProcess(running_process.get());
    }

    for (const auto& pro : ready_queue) {
        PrintProcess(&pro);
    }

    for (const auto& pro : finish_queue) {
        PrintProcess(&pro);
    }

    for (const auto& pro : arrive_queue) {
        PrintProcess(&pro);
    }

    for (const auto& pro : blocked_queue) {
        PrintProcess(&pro);
    }
}

// 输出调度日志
void ProcessScheduler::Log(const wstring& msg, int current_time) {
    wcout << L"[时间" << current_time << L"] " << msg << L"\n";
}

// 先来先服务
void ProcessScheduler::FCFS() {
    int current_time = 0;
    bool need_schedule = true;
    gantt_data.clear();
    running_process.reset();

    while (true) {
        if (!running_process && arrive_queue.empty() && ready_queue.empty() && blocked_queue.empty()) break;

        MoveArrivedToReady(current_time);
        UpdateBlockedQueue();

        // 更新等待时间
        for (auto& pro : ready_queue) pro.wait_time++;

        if (need_schedule && !ready_queue.empty()) {
            running_process = std::unique_ptr<ProcessPCB>(new ProcessPCB(ready_queue[0]));
            ready_queue.erase(ready_queue.begin());
            if (running_process->start_time == -1)
                running_process->start_time = current_time;
            if (running_process->response_time == -1)
                running_process->response_time = current_time - running_process->arrive_time;
            running_process->state = Executing;
            need_schedule = false;
            Log(L"进程" + running_process->name + L"（P" + to_wstring(running_process->ID) + L"）开始执行，优先级" + to_wstring(running_process->priority) + L"，剩余时间" + to_wstring(running_process->all_time), current_time);
            PrintAll(current_time);
        }

        if (running_process) {
            gantt_data.push_back(make_pair(running_process->ID, current_time));

            // IO阻塞
            if (running_process->cpu_time == running_process->io_start && running_process->io_time > 0) {
                running_process->state = Blocked;
                blocked_queue.push_back(*running_process);
                Log(L"进程" + running_process->name + L"（P" + to_wstring(running_process->ID) + L"）进入IO，阻塞" + to_wstring(running_process->io_time) + L"个时间片", current_time);
                running_process.reset();
                need_schedule = true;
                continue;
            }

            running_process->cpu_time++;
            running_process->all_time--;

            if (running_process->all_time == 0) {
                running_process->end_time = current_time + 1;
                running_process->state = Finish;
                running_process->turnaround_time = running_process->end_time - running_process->arrive_time;
                finish_queue.push_back(*running_process);
                Log(L"进程" + running_process->name + L"（P" + to_wstring(running_process->ID) + L"）完成", current_time + 1);
                running_process.reset();
                need_schedule = true;
            }
        }
        current_time++;
    }
    PrintAll(current_time);
}

// 时间片轮转
void ProcessScheduler::RoundRobin() {
    int current_time = 0;
    bool need_schedule = true;
    const int time_quantum = 2;
    int time_slice = 0;
    gantt_data.clear();
    running_process.reset();

    while (true) {
        if (!running_process && arrive_queue.empty() && ready_queue.empty() && blocked_queue.empty()) break;

        MoveArrivedToReady(current_time);
        UpdateBlockedQueue();

        for (auto& pro : ready_queue) pro.wait_time++;

        if (need_schedule && !ready_queue.empty()) {
            running_process = std::unique_ptr<ProcessPCB>(new ProcessPCB(ready_queue[0]));
            ready_queue.erase(ready_queue.begin());
            if (running_process->start_time == -1)
                running_process->start_time = current_time;
            if (running_process->response_time == -1)
                running_process->response_time = current_time - running_process->arrive_time;
            running_process->state = Executing;
            time_slice = 0;
            need_schedule = false;
            Log(L"进程" + running_process->name + L"（P" + to_wstring(running_process->ID) + L"）开始执行，优先级" + to_wstring(running_process->priority) + L"，剩余时间" + to_wstring(running_process->all_time), current_time);
            PrintAll(current_time);
        }

        if (running_process) {
            gantt_data.push_back(make_pair(running_process->ID, current_time));

            if (running_process->cpu_time == running_process->io_start && running_process->io_time > 0) {
                running_process->state = Blocked;
                blocked_queue.push_back(*running_process);
                Log(L"进程" + running_process->name + L"（P" + to_wstring(running_process->ID) + L"）进入IO，阻塞" + to_wstring(running_process->io_time) + L"个时间片", current_time);
                running_process.reset();
                need_schedule = true;
                continue;
            }

            running_process->cpu_time++;
            running_process->all_time--;
            time_slice++;

            if (running_process->all_time == 0) {
                running_process->end_time = current_time + 1;
                running_process->state = Finish;
                running_process->turnaround_time = running_process->end_time - running_process->arrive_time;
                finish_queue.push_back(*running_process);
                Log(L"进程" + running_process->name + L"（P" + to_wstring(running_process->ID) + L"）完成", current_time + 1);
                running_process.reset();
                need_schedule = true;
            } else if (time_slice == time_quantum) {
                running_process->state = Ready;
                ready_queue.push_back(*running_process);
                Log(L"进程" + running_process->name + L"（P" + to_wstring(running_process->ID) + L"）时间片用尽，回到就绪队列", current_time + 1);
                running_process.reset();
                need_schedule = true;
            }
        }
        current_time++;
    }
    PrintAll(current_time);
}

// 动态优先级
void ProcessScheduler::DynamicPriority() {
    int current_time = 0;
    bool need_schedule = true;
    gantt_data.clear();
    running_process.reset();

    while (true) {
        if (!running_process && arrive_queue.empty() && ready_queue.empty() && blocked_queue.empty()) break;

        MoveArrivedToReady(current_time);
        UpdateBlockedQueue();

        if (!ready_queue.empty()) {
            sort(ready_queue.begin(), ready_queue.end(),
                [this](const ProcessPCB& a, const ProcessPCB& b) {
                    return this->ComparePriority(a, b);
                });
        }

        for (auto& pro : ready_queue) pro.wait_time++;

        if (need_schedule && !ready_queue.empty()) {
            running_process = std::unique_ptr<ProcessPCB>(new ProcessPCB(ready_queue[0]));
            ready_queue.erase(ready_queue.begin());
            if (running_process->start_time == -1)
                running_process->start_time = current_time;
            if (running_process->response_time == -1)
                running_process->response_time = current_time - running_process->arrive_time;
            running_process->state = Executing;
            need_schedule = false;
            Log(L"进程" + running_process->name + L"（P" + to_wstring(running_process->ID) + L"）开始执行，优先级" + to_wstring(running_process->priority) + L"，剩余时间" + to_wstring(running_process->all_time), current_time);
            PrintAll(current_time);
        }

        if (running_process) {
            gantt_data.push_back(make_pair(running_process->ID, current_time));

            if (running_process->cpu_time == running_process->io_start && running_process->io_time > 0) {
                running_process->state = Blocked;
                blocked_queue.push_back(*running_process);
                Log(L"进程" + running_process->name + L"（P" + to_wstring(running_process->ID) + L"）进入IO，阻塞" + to_wstring(running_process->io_time) + L"个时间片", current_time);
                running_process.reset();
                need_schedule = true;
                continue;
            }

            running_process->cpu_time++;
            running_process->all_time--;

            if (running_process->all_time == 0) {
                running_process->end_time = current_time + 1;
                running_process->state = Finish;
                running_process->turnaround_time = running_process->end_time - running_process->arrive_time;
                finish_queue.push_back(*running_process);
                Log(L"进程" + running_process->name + L"（P" + to_wstring(running_process->ID) + L"）完成", current_time + 1);
                running_process.reset();
                need_schedule = true;
            } else {
                if (running_process->priority > 1) running_process->priority--;
                if (!ready_queue.empty() && ready_queue[0].priority >= running_process->priority) {
                    running_process->state = Ready;
                    ready_queue.push_back(*running_process);
                    Log(L"进程" + running_process->name + L"（P" + to_wstring(running_process->ID) + L"）被抢占，回到就绪队列", current_time + 1);
                    running_process.reset();
                    need_schedule = true;
                } else {
                    need_schedule = false;
                }
            }
        }
        current_time++;
    }
    PrintAll(current_time);
}

// 最短作业优先（SJF）
void ProcessScheduler::SJF() {
    int current_time = 0;
    bool need_schedule = true;
    gantt_data.clear();
    running_process.reset();

    while (true) {
        if (!running_process && arrive_queue.empty() && ready_queue.empty() && blocked_queue.empty()) break;

        MoveArrivedToReady(current_time);
        UpdateBlockedQueue();

        // 按剩余服务时间排序
        if (!ready_queue.empty()) {
            sort(ready_queue.begin(), ready_queue.end(),
                [](const ProcessPCB& a, const ProcessPCB& b) {
                    return a.all_time < b.all_time;
                });
        }

        for (auto& pro : ready_queue) pro.wait_time++;

        if (need_schedule && !ready_queue.empty()) {
            running_process = std::unique_ptr<ProcessPCB>(new ProcessPCB(ready_queue[0]));
            ready_queue.erase(ready_queue.begin());
            if (running_process->start_time == -1)
                running_process->start_time = current_time;
            if (running_process->response_time == -1)
                running_process->response_time = current_time - running_process->arrive_time;
            running_process->state = Executing;
            need_schedule = false;
            Log(L"进程" + running_process->name + L"（P" + to_wstring(running_process->ID) + L"）开始执行，服务时间" + to_wstring(running_process->service_time) + L"，剩余时间" + to_wstring(running_process->all_time), current_time);
            PrintAll(current_time);
        }

        if (running_process) {
            gantt_data.push_back(make_pair(running_process->ID, current_time));

            if (running_process->cpu_time == running_process->io_start && running_process->io_time > 0) {
                running_process->state = Blocked;
                blocked_queue.push_back(*running_process);
                Log(L"进程" + running_process->name + L"（P" + to_wstring(running_process->ID) + L"）进入IO，阻塞" + to_wstring(running_process->io_time) + L"个时间片", current_time);
                running_process.reset();
                need_schedule = true;
                continue;
            }

            running_process->cpu_time++;
            running_process->all_time--;

            if (running_process->all_time == 0) {
                running_process->end_time = current_time + 1;
                running_process->state = Finish;
                running_process->turnaround_time = running_process->end_time - running_process->arrive_time;
                finish_queue.push_back(*running_process);
                Log(L"进程" + running_process->name + L"（P" + to_wstring(running_process->ID) + L"）完成", current_time + 1);
                running_process.reset();
                need_schedule = true;
            }
        }
        current_time++;
    }
    PrintAll(current_time);
}
// 高xj响应比优先（HRRN）
void ProcessScheduler::HRRN() {
    int current_time = 0;
    bool need_schedule = true;
    gantt_data.clear();
    running_process.reset();

    while (true) {
        if (!running_process && arrive_queue.empty() && ready_queue.empty() && blocked_queue.empty()) break;

        MoveArrivedToReady(current_time);
        UpdateBlockedQueue();

        // 计算响应比并排序
        if (!ready_queue.empty()) {
            for (auto& pro : ready_queue) {
                double response_ratio = (double)(pro.wait_time + pro.service_time) / pro.service_time;
                pro.priority = static_cast<int>(response_ratio * 10000); // 用priority临时存储响应比，便于排序
            }
            std::sort(ready_queue.begin(), ready_queue.end(),
                [](const ProcessPCB& a, const ProcessPCB& b) {
                    return a.priority > b.priority;
                });
        }

        for (auto& pro : ready_queue) pro.wait_time++;

        if (need_schedule && !ready_queue.empty()) {
            running_process = std::unique_ptr<ProcessPCB>(new ProcessPCB(ready_queue[0]));
            ready_queue.erase(ready_queue.begin());
            if (running_process->start_time == -1)
                running_process->start_time = current_time;
            if (running_process->response_time == -1)
                running_process->response_time = current_time - running_process->arrive_time;
            running_process->state = Executing;
            need_schedule = false;
            Log(L"进程" + running_process->name + L"（P" + std::to_wstring(running_process->ID) + L"）开始执行", current_time);
            PrintAll(current_time);
        }

        if (running_process) {
            gantt_data.push_back(make_pair(running_process->ID, current_time));

            if (running_process->cpu_time == running_process->io_start && running_process->io_time > 0) {
                running_process->state = Blocked;
                blocked_queue.push_back(*running_process);
                Log(L"进程" + running_process->name + L"（P" + std::to_wstring(running_process->ID) + L"）进入IO", current_time);
                running_process.reset();
                need_schedule = true;
                continue;
            }

            running_process->cpu_time++;
            running_process->all_time--;

            if (running_process->all_time == 0) {
                running_process->end_time = current_time + 1;
                running_process->state = Finish;
                running_process->turnaround_time = running_process->end_time - running_process->arrive_time;
                finish_queue.push_back(*running_process);
                Log(L"进程" + running_process->name + L"（P" + std::to_wstring(running_process->ID) + L"）完成", current_time + 1);
                running_process.reset();
                need_schedule = true;
            }
        }
        current_time++;
    }
    PrintAll(current_time);
}

// 最短剩余时间优先（SRTF）
void ProcessScheduler::SRTF() {
    int current_time = 0;
    gantt_data.clear();
    running_process.reset();

    while (true) {
        if (!running_process && arrive_queue.empty() && ready_queue.empty() && blocked_queue.empty()) break;

        MoveArrivedToReady(current_time);
        UpdateBlockedQueue();

        // 按剩余时间排序
        if (!ready_queue.empty()) {
            std::sort(ready_queue.begin(), ready_queue.end(),
                [](const ProcessPCB& a, const ProcessPCB& b) {
                    return a.all_time < b.all_time;
                });
        }

        for (auto& pro : ready_queue) pro.wait_time++;

        // 抢占判断
        if (!ready_queue.empty()) {
            if (!running_process || ready_queue[0].all_time < running_process->all_time) {
                if (running_process) {
                    running_process->state = Ready;
                    ready_queue.push_back(*running_process);
                }
                running_process = std::unique_ptr<ProcessPCB>(new ProcessPCB(ready_queue[0]));
                ready_queue.erase(ready_queue.begin());
                if (running_process->start_time == -1)
                    running_process->start_time = current_time;
                if (running_process->response_time == -1)
                    running_process->response_time = current_time - running_process->arrive_time;
                running_process->state = Executing;
                Log(L"进程" + running_process->name + L"（P" + std::to_wstring(running_process->ID) + L"）开始/被抢占执行", current_time);
                PrintAll(current_time);
            }
        }

        if (running_process) {
            gantt_data.push_back(make_pair(running_process->ID, current_time));

            if (running_process->cpu_time == running_process->io_start && running_process->io_time > 0) {
                running_process->state = Blocked;
                blocked_queue.push_back(*running_process);
                Log(L"进程" + running_process->name + L"（P" + std::to_wstring(running_process->ID) + L"）进入IO", current_time);
                running_process.reset();
                continue;
            }

            running_process->cpu_time++;
            running_process->all_time--;

            if (running_process->all_time == 0) {
                running_process->end_time = current_time + 1;
                running_process->state = Finish;
                running_process->turnaround_time = running_process->end_time - running_process->arrive_time;
                finish_queue.push_back(*running_process);
                Log(L"进程" + running_process->name + L"（P" + std::to_wstring(running_process->ID) + L"）完成", current_time + 1);
                running_process.reset();
            }
        }
        current_time++;
    }
    PrintAll(current_time);
}

// 输出统计信息
void ProcessScheduler::PrintStatistics() {
    if (finish_queue.empty()) return;
    double avg_wait = 0, avg_turn = 0, avg_weighted = 0, avg_response = 0;
    for (const auto& pro : finish_queue) {
        avg_wait += pro.wait_time;
        avg_turn += pro.turnaround_time;
        avg_weighted += (double)pro.turnaround_time / pro.service_time;
        avg_response += pro.response_time;
    }
    avg_wait /= finish_queue.size();
    avg_turn /= finish_queue.size();
    avg_weighted /= finish_queue.size();
    avg_response /= finish_queue.size();

    wcout << L"\n统计信息：\n";
    wcout << L"平均等待时间：" << avg_wait << L"\n";
    wcout << L"平均周转时间：" << avg_turn << L"\n";
    wcout << L"平均带权周转时间：" << avg_weighted << L"\n";
    wcout << L"平均响应时间：" << avg_response << L"\n";
}

int main() {
    // 设置控制台为UTF-8模式
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    _setmode(_fileno(stdout), _O_U16TEXT);

    wcout << L"===================================================\n";
    wcout << L"          操作系统进程调度模拟实验        \n";
    wcout << L"===================================================\n\n";

    ProcessScheduler scheduler;
    scheduler.Run();

    wcout << L"\n模拟结束，甘特图将在窗口中显示。\n";
    return EXIT_SUCCESS;
}
