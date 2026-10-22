#pragma once
#include "LLMStrategy.h"
#include "State_pattern.h"
#include "Observer_pattern.h"
#include <string>
#include <memory>
#include <iostream>
using namespace std;

class Agent : public Observer {
    string name;
    shared_ptr<LLMStrategy> llm;   // Strategy
    State_Pattern state;           // State
    Subject bus;                   // Observer (agent cũng là subject để phát tin)
public:
    Agent(string n, shared_ptr<LLMStrategy> s)
        : name(move(n)), llm(move(s)), state(State_Pattern::IDLE) {}

    string getName() const { return name; }
    State_Pattern getState() const { return state; }

    // Cho phép agent khác subscribe vào bus của mình
    Subject& getBus() { return bus; }

    // Nhận tin từ agent khác (Observer callback)
    void onMessage(const string& from, const string& topic, const string& payload) override {
        cout << "[" << name << "] nhan tu " << from
             << " | topic=" << topic
             << " | payload=" << payload << "\n";
        if (topic == "TASK") {
            runTask(payload);
        }
    }

    // Thực thi task: chuyển state và gọi LLM
    string runTask(const string& prompt) {
        state = State_Pattern::IN_PROGRESS;
        cout << "[" << name << "] state -> IN_PROGRESS\n";

        string result;
        try {
            result = llm->MethodGenerate(prompt);
        } catch (...) {
            state = State_Pattern::FAILED;
            cout << "[" << name << "] state -> FAILED\n";
            return "[ERROR] Agent " + name + " gap loi khi goi LLM";
        }

        state = State_Pattern::COMPLETED;
        cout << "[" << name << "] state -> COMPLETED\n";

        // Phát kết quả cho các observer
        bus.notify(name, "RESULT", result);
        return result;
    }
};