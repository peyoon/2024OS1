#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <sstream>
#include <mutex>
#include <condition_variable>

using namespace std;

mutex coutMutex; // cout 동기화를 위한 뮤텍스
vector<string> bgOutputs; // 백그라운드 명령어의 출력을 저장

void executeCommand(const string& command, bool isBackground, int id) {
    if (command.substr(0, 4) == "echo") {
        string output = command.substr(5);
        if (isBackground) {
            // 백그라운드 작업은 딜레이 후 출력
            this_thread::sleep_for(chrono::seconds(1));
            lock_guard<mutex> lock(coutMutex);
            bgOutputs[id] = output; // 결과 저장
        }
        else {
            // 포그라운드 명령어는 즉시 출력
            lock_guard<mutex> lock(coutMutex);
            cout << output << endl;
        }
    }
}

void processCommands(const string& input) {
    stringstream ss(input);
    string segment;
    vector<thread> bgThreads;

    int bgCount = 0;
    while (getline(ss, segment, ';')) {
        string trimmed = segment.erase(0, segment.find_first_not_of(" \n\r\t"));
        if (trimmed[0] == '&') {
            trimmed.erase(0, 1); // '&' 제거
            bgOutputs.push_back(""); // 출력 저장 공간 확보
            bgThreads.emplace_back(executeCommand, trimmed, true, bgCount++);
            cout << "Running: [" << bgCount - 1 << "B]" << endl;
        }
        else {
            executeCommand(trimmed, false, -1);
        }
    }

    // 백그라운드 작업이 완료되길 기다림
    for (auto& th : bgThreads) {
        if (th.joinable()) {
            th.join();
        }
    }

    // 백그라운드 명령어 결과 출력
    for (const auto& output : bgOutputs) {
        if (!output.empty()) {
            cout << output << endl;
        }
    }
    bgOutputs.clear(); // 다음 입력을 위해 클리어
}

int main() {
    ifstream file("commands.txt");
    string line;

    cout << "prompt> ";
    while (getline(file, line)) {
        processCommands(line);
        cout << "prompt> ";
        this_thread::sleep_for(chrono::seconds(5)); // Y초 동안 대기
    }
    file.close();
    return 0;
}
