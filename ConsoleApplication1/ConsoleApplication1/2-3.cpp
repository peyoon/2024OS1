//2-3 (최대한으로 작성한 것)
#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <condition_variable>
#include <chrono>
#include <queue>
#include <list>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <map>

// 프로세스 타입 정의: 포그라운드와 백그라운드
enum class ProcessType {
    Foreground,
    Background
};

// 프로세스 구조체 정의
struct Process {
    int pid;
    ProcessType type;
    std::string status;
};

// 동적 큐를 관리하는 클래스
class DynamicQueue {
public:
    std::list<std::list<Process>> stacks;
    std::mutex dqMutex;
    int next_pid = 0;
    std::map<int, std::string> waitQueue;  // PID and remaining time

    void enqueue(Process proc, bool isForeground);
    Process dequeue();
    void promote();
    void split_n_merge();
    void printState();
};

void DynamicQueue::enqueue(Process proc, bool isForeground) {
    std::lock_guard<std::mutex> lock(dqMutex);
    proc.pid = next_pid++;  // Assign and increment PID
    proc.status = isForeground ? "F" : "B";  // Assign status based on process type
    if (isForeground) {
        if (stacks.empty() || stacks.front().empty()) {
            stacks.push_front({ proc });
        }
        else {
            stacks.front().push_back(proc);
        }
    }
    else {
        if (stacks.empty() || stacks.back().empty()) {
            stacks.push_back({ proc });
        }
        else {
            stacks.back().push_back(proc);
        }
    }
}

Process DynamicQueue::dequeue() {
    std::lock_guard<std::mutex> lock(dqMutex);
    if (!stacks.empty() && !stacks.front().empty()) {
        Process proc = stacks.front().front();
        stacks.front().pop_front();
        if (stacks.front().empty()) {
            stacks.pop_front();
        }
        return proc;
    }
    return Process{ -1, ProcessType::Background, "Empty Queue" };
}

void DynamicQueue::promote() {
    std::lock_guard<std::mutex> lock(dqMutex);
    if (stacks.size() > 1) {
        auto lastIt = --stacks.end();
        if (!lastIt->empty()) {
            Process proc = lastIt->back();
            lastIt->pop_back();
            auto firstIt = stacks.begin();
            proc.status += "*";  // Mark as promoted
            firstIt->push_front(proc);
        }
    }
}

void DynamicQueue::split_n_merge() {
    std::lock_guard<std::mutex> lock(dqMutex);
    for (auto it = stacks.begin(); it != stacks.end(); ++it) {
        if (it->size() > 3) {
            std::list<Process> half;
            auto midPoint = it->begin();
            std::advance(midPoint, it->size() / 2);
            half.splice(half.begin(), *it, midPoint, it->end());

            if (std::next(it) != stacks.end()) {
                std::next(it)->splice(std::next(it)->begin(), half);
            }
            else {
                stacks.push_back(half);
            }
        }
    }
}

void DynamicQueue::printState() {
    std::lock_guard<std::mutex> lock(dqMutex);
    std::cout << "---------------------------" << std::endl;
    std::cout << "Running: " << "[]" << std::endl;
    std::cout << "---------------------------" << std::endl;
    std::cout << "DQ: P => ";
    for (auto& stack : stacks) {
        for (auto& proc : stack) {
            std::cout << "[" << proc.pid << proc.status << "] ";
        }
    }
    std::cout << "\n--------------------------" << std::endl;
    std::cout << "WQ: ";
    for (auto& wq : waitQueue) {
        std::cout << "[" << wq.first << ":" << wq.second << "] ";
    }
    std::cout << std::endl;
    std::cout << "---------------------------" << std::endl;
}

DynamicQueue dq;

// 명령어를 파싱하는 함수
char** parse(const char* command) {
    int spaceCount = 0;
    const char* tmp = command;
    while (*tmp) {
        if (*tmp == ' ') spaceCount++;
        tmp++;
    }

    std::cout << "prompt> " << command << std::endl;

    char** tokens = new char* [spaceCount + 2];
    int i = 0;
    char* next_token = nullptr;

    char* cmd_copy = new char[strlen(command) + 1];
    strcpy_s(cmd_copy, strlen(command) + 1, command);

    char* token = strtok_s(cmd_copy, " ", &next_token);

    while (token != nullptr) {
        tokens[i++] = _strdup(token);
        token = strtok_s(nullptr, " ", &next_token);
    }
    tokens[i] = nullptr;

    delete[] cmd_copy;

    return tokens;
}


std::mutex cout_mutex;

void echo(const std::string& str) {
    std::lock_guard<std::mutex> lock(cout_mutex);
    std::cout << str << std::endl;
}

void dummy() {
    // Do nothing
}

int gcd(int a, int b) {
    while (b != 0) {
        int t = b;
        b = a % b;
        a = t;
    }
    return a;
}

int prime_count(int n) {
    std::vector<bool> is_prime(n + 1, true);
    is_prime[0] = is_prime[1] = false;
    for (int i = 2; i * i <= n; ++i) {
        if (is_prime[i]) {
            for (int j = i * i; j <= n; j += i)
                is_prime[j] = false;
        }
    }
    int count = 0;
    for (int i = 2; i <= n; ++i) {
        if (is_prime[i]) count++;
    }
    return count;
}

long long sum_up_to(int n, int parts) {
    long long total = 0;
    std::vector<std::thread> threads;
    std::vector<long long> partial_sums(parts, 0);

    for (int i = 0; i < parts; ++i) {
        threads.emplace_back([i, n, parts, &partial_sums]() {
            int start = i * (n / parts) + 1;
            int end = (i == parts - 1) ? n : (i + 1) * (n / parts);
            for (int j = start; j <= end; ++j) {
                partial_sums[i] += j;
            }
            });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    for (auto part_sum : partial_sums) {
        total += part_sum;
    }
    return total % 1000000;
}

void exec_command(const std::string& cmd, const std::vector<std::string>& args, int n, int period, int duration, int multithread) {
    auto run = [&]() {
        auto start_time = std::chrono::high_resolution_clock::now();
        do {
            if (cmd == "echo") {
                echo(args.size() > 0 ? args[0] : "");
            }
            else if (cmd == "dummy") {
                dummy();
            }
            else if (cmd == "gcd") {
                if (args.size() >= 2) {
                    int a = std::stoi(args[0]);
                    int b = std::stoi(args[1]);
                    int result = gcd(a, b);
                    std::lock_guard<std::mutex> lock(cout_mutex);
                    std::cout << result << std::endl;
                }
            }
            else if (cmd == "prime") {
                if (args.size() > 0) {
                    int n = std::stoi(args[0]);
                    int result = prime_count(n);
                    std::lock_guard<std::mutex> lock(cout_mutex);
                    std::cout << result << std::endl;
                }
            }
            else if (cmd == "sum") {
                if (args.size() > 0) {
                    int n = std::stoi(args[0]);
                    long long result = sum_up_to(n, multithread);
                    std::lock_guard<std::mutex> lock(cout_mutex);
                    std::cout << result << std::endl;
                }
            }
            if (period > 0) {
                std::this_thread::sleep_for(std::chrono::seconds(period));
            }
            if (duration > 0) {
                auto current_time = std::chrono::high_resolution_clock::now();
                int elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
                if (elapsed >= duration) break;
            }
        } while (period > 0);
        };

    std::vector<std::thread> threads;
    for (int i = 0; i < n; ++i) {
        threads.emplace_back(run);
    }

    for (auto& th : threads) {
        th.join();
    }
}

// 명령어를 실행하는 함수
void exec(char** args) {
    std::string cmd;
    std::vector<std::string> commandArgs;
    int n = 1, period = 0, duration = 0, multithread = 1;

    for (int i = 0; args[i] != nullptr; ++i) {
        std::string arg = args[i];
        if (arg == "-n") {
            n = std::stoi(args[++i]);
        }
        else if (arg == "-p") {
            period = std::stoi(args[++i]);
        }
        else if (arg == "-d") {
            duration = std::stoi(args[++i]);
        }
        else if (arg == "-m") {
            multithread = std::stoi(args[++i]);
        }
        else if (cmd.empty()) {
            cmd = arg;
        }
        else {
            commandArgs.push_back(arg);
        }
    }

    exec_command(cmd, commandArgs, n, period, duration, multithread);

    delete[] args[0];
    delete[] args;
}

void monitorTask(int interval) {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(interval));
        dq.printState();
    }
}

void shellTask(int sleepTime) {
    std::ifstream file("commands.txt");
    std::string line;
    if (!file.is_open()) {
        std::cerr << "Failed to open commands.txt" << std::endl;
        return;
    }

    while (getline(file, line)) {
        std::istringstream iss(line);
        std::string command;
        while (getline(iss, command, ';')) {
            bool isBackground = command.find('&') == 0;
            if (isBackground) {
                command.erase(0, 1);
                std::thread bgThread([command] {
                    char** parsedCommand = parse(command.c_str());
                    exec(parsedCommand);
                    });
                bgThread.detach();
            }
            else {
                char** parsedCommand = parse(command.c_str());
                exec(parsedCommand);
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(sleepTime));
    }
    file.close();
}

int main()
{
    std::locale::global(std::locale("en_US.UTF-8"));

    std::thread monitorThread(monitorTask, 5);
    std::thread shellThread(shellTask, 3);

    monitorThread.join();
    shellThread.join();

    return 0;
}