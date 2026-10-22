#pragma once
#include "Agent.h"
#include <vector>
#include <memory>
#include <string>
using namespace std;

class Orchestrator {
    vector<shared_ptr<Agent>> agents;
public:
    void addAgent(shared_ptr<Agent> a) { agents.push_back(a); }

    // Gửi task đến 1 agent theo tên
    string dispatch(const string& agentName, const string& prompt) {
        for (auto& a : agents) {
            if (a->getName() == agentName) return a->runTask(prompt);
        }
        return "[Orchestrator] Khong tim thay agent: " + agentName;
    }

    // Cho các agent subscribe lẫn nhau (broadcast)
    void wireAll() {
        for (auto& sub : agents)
            for (auto& pub : agents)
                if (sub != pub) pub->getBus().attach(sub.get());
    }
};