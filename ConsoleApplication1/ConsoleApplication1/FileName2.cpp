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

mutex coutMutex; // cout ����ȭ�� ���� ���ؽ�
vector<string> bgOutputs; // ��׶��� ��ɾ��� ����� ����

void executeCommand(const string& command, bool isBackground, int id) {
    if (command.substr(0, 4) == "echo") {
        string output = command.substr(5);
        if (isBackground) {
            // ��׶��� �۾��� ������ �� ���
            this_thread::sleep_for(chrono::seconds(1));
            lock_guard<mutex> lock(coutMutex);
            bgOutputs[id] = output; // ��� ����
        }
        else {
            // ���׶��� ��ɾ�� ��� ���
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
            trimmed.erase(0, 1); // '&' ����
            bgOutputs.push_back(""); // ��� ���� ���� Ȯ��
            bgThreads.emplace_back(executeCommand, trimmed, true, bgCount++);
            cout << "Running: [" << bgCount - 1 << "B]" << endl;
        }
        else {
            executeCommand(trimmed, false, -1);
        }
    }

    // ��׶��� �۾��� �Ϸ�Ǳ� ��ٸ�
    for (auto& th : bgThreads) {
        if (th.joinable()) {
            th.join();
        }
    }

    // ��׶��� ��ɾ� ��� ���
    for (const auto& output : bgOutputs) {
        if (!output.empty()) {
            cout << output << endl;
        }
    }
    bgOutputs.clear(); // ���� �Է��� ���� Ŭ����
}

int main() {
    ifstream file("commands.txt");
    string line;

    cout << "prompt> ";
    while (getline(file, line)) {
        processCommands(line);
        cout << "prompt> ";
        this_thread::sleep_for(chrono::seconds(5)); // Y�� ���� ���
    }
    file.close();
    return 0;
}
