#pragma once
#include "LLMStrategy.h"
#include "State_pattern.h"
#include "Observer_Pattern.h"
#include <string>
#include <memory>
#include <iostream>
using namespace std;
 
class Agent : public Observer, public StateContext {   
    string name;
    shared_ptr<LLMStrategy> llm;   
    shared_ptr<TaskState> state;   
    Subject bus;                   
public:
    Agent(string n, shared_ptr<LLMStrategy> s)
        : name(move(n)), llm(move(s)), state(make_shared<IdleState>()) {}
 
    string getName() const { return name; }
    string getStateName() const { return state->name(); }
    Subject& getBus() { return bus; }
 
    string contextName() const override { return name; }
    void changeState(shared_ptr<TaskState> next) override {
        cout << "[" << name << "] state: " << state->name() << " -> " << next->name() << "\n";
        state = move(next);
    }
 
    void onMessage(const string& from, const string& topic, const string& payload) override {
        cout << "[" << name << "] nhan tu " << from
             << " | topic=" << topic
             << " | payload=" << payload << "\n";
        if (topic == "TASK") runTask(payload);
    }
 
    string runTask(const string& prompt) {
        if (!state->start(*this))   
            return "[Agent " + name + "] dang o trang thai " + state->name() + ", khong the nhan task moi";
 
        string result;
        try {
            result = llm->MethodGenerate(prompt);
        } catch (...) {
            state->fail(*this);
            return "[ERROR] Agent " + name + " gap loi khi goi LLM";
        }
        if (result.empty() || result.rfind("[ERROR]", 0) == 0) {   
            state->fail(*this);
            return result.empty() ? "[ERROR] LLM khong tra ve ket qua" : result;
        }
 
        state->complete(*this);
        bus.notify(name, "RESULT", result);
        return result;
    }
};
 
