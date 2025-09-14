#define UNICODE
#define _UNICODE
#pragma once
#ifndef PROCESS_SCHEDULING_SIMULATOR_H
#define PROCESS_SCHEDULING_SIMULATOR_H
#include <vector>
#include <string>
#include <memory>

#if __cplusplus < 201402L
namespace std {
    template<typename T, typename... Args>
    std::unique_ptr<T> make_unique(Args&&... args) {
        return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
    }
}
#endif
enum ProcessState {
    Executing, Ready, Finish, Unarrive, Blocked
};

// 进程控制块(PCB)结构体
struct ProcessPCB {
    int ID;
    std::wstring name;
    int arrive_time, service_time, priority;
    int io_start, io_time, all_time, cpu_time;
    int start_time, end_time, wait_time, response_time, turnaround_time, io_count;
    ProcessState state;
};

class ProcessScheduler {
public:
    ProcessScheduler();
    ~ProcessScheduler();

    void Run();
    void ShowGanttChart();
    void InputProcesses();
    int SelectPolicy();
    void PrintAll(int current);
    void PrintProcess(const ProcessPCB* pro);
    void Log(const std::wstring& msg, int current_time);
    void MoveArrivedToReady(int current_time);
    void UpdateBlockedQueue();
    bool CompareArriveTime(const ProcessPCB& a, const ProcessPCB& b);
    bool ComparePriority(const ProcessPCB& a, const ProcessPCB& b);

    void FCFS();
    void RoundRobin();
    void DynamicPriority();
    void SJF();
    void PrintStatistics();
    void HRRN();
    void SRTF();

    // 队列
    std::vector<ProcessPCB> arrive_queue;
    std::vector<ProcessPCB> ready_queue;
    std::vector<ProcessPCB> blocked_queue;
    std::vector<ProcessPCB> finish_queue;
    std::vector<std::pair<int, int>> gantt_data;
    std::unique_ptr<ProcessPCB> running_process;
};

#endif
