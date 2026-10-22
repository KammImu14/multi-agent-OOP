#pragma once
#include <string>
#include <iostream>
using namespace std;

enum class State_Pattern {
    IDLE,
    IN_PROGRESS,
    COMPLETED,
    FAILED
};

inline string stateToString(State_Pattern s) {
    switch (s) {
        case State_Pattern::IDLE:        return "IDLE";
        case State_Pattern::IN_PROGRESS: return "IN_PROGRESS";
        case State_Pattern::COMPLETED:   return "COMPLETED";
        case State_Pattern::FAILED:      return "FAILED";
    }
    return "UNKNOWN";
}