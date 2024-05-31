#include <iostream>
#include <thread>
#include <mutex>
#include <deque>
#include <vector>
#include <chrono>
#include <string>
#include <atomic>

using namespace std;

struct Process {
    int pid;
    string type;
    bool promoted;

    Process(int id, string t, bool p = false) : pid(id), type(t), promoted(p) {}
};

// 프로세스 큐
deque<Process> processQueue; 
vector<pair<int, int>> waitQueue; // <Process ID, Time Remaining>

mutex printMutex;
atomic<int> current_pid(1);  // PID는 1부터 시작

// 큐에 프로세스 추가 및 상태 출력
void addProcess(int pid, string type, bool isMonitor) {
    lock_guard<mutex> lock(printMutex);
    bool promoted = (isMonitor && pid % 3 == 0); // 모니터 프로세스가 3의 배수로 프로모션
    processQueue.push_back({pid, type, promoted});

    cout << "Running: [" << pid << type << "]\n";
    cout << "---------------------------\n";
    cout << "DQ: P => ";
    for (auto& proc : processQueue) {
        cout << "[" << proc.pid << proc.type << (proc.promoted ? "*" : "") << "] ";
    }
    cout << "(bottom/top)\n";
    cout << "WQ: ";
    for (auto& w : waitQueue) {
        cout << "[" << w.first << ":" << w.second << "] ";
    }
    cout << "\n---------------------------\n";
}

// 프로세스 스레드
void processThread(int totalDuration, int shellInterval, int monitorInterval) {
    int time = 0;
    while (time < totalDuration) {
        this_thread::sleep_for(chrono::seconds(shellInterval));
        int pid = current_pid++;
        string type = (time % monitorInterval == 0) ? "B" : "F";
        addProcess(pid, type, time % monitorInterval == 0);
        time += shellInterval;
    }
}

int main() {
    int totalDuration = 30;  // 전체 시뮬레이션 시간
    int shellInterval = 2;   // shell 프로세스 간격
    int monitorInterval = 5; // monitor 프로세스 간격

    thread processThread1(processThread, totalDuration, shellInterval, monitorInterval);

    processThread1.join();

    return 0;
}
