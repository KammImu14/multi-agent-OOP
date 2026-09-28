#include <iostream>
#include <string>
#include <vector>
#include <memory>

enum class TaskState { IDLE, IN_PROGRESS, COMPLETED };

class Strategy {
public:
    virtual ~Strategy() = default;
    virtual std::string generate(const std::string& prompt) = 0;
};


class Observer {
public:
    virtual ~Observer() = default;
    virtual void onEvent(const std::string& sender, const std::string& msg) = 0;
};


class Agent : public Observer {
    std::string name;
    std::unique_ptr<Strategy> strategy;
    TaskState state = TaskState::IDLE;
    std::vector<Observer*> observers;

public:
    Agent(std::string n, std::unique_ptr<Strategy> s)
        : name(std::move(n)), strategy(std::move(s)) {}

    void setStrategy(std::unique_ptr<Strategy> s) { strategy = std::move(s); }

    void addObserver(Observer* o) { observers.push_back(o); }

    void notify(const std::string& msg) {
        for (auto* o : observers) o->onEvent(name, msg);
    }

    void onEvent(const std::string& sender, const std::string& msg) override {
        std::cout << "  -> [" << name << "] RECEIVE FROM " << sender << ": " << msg << "\n";
    }

    void execute(const std::string& prompt) {
        state = TaskState::IN_PROGRESS;
        notify("START PROCESSING...");

        if (strategy)
            std::cout << "[" << name << "] " << strategy->generate(prompt) << "\n";

        state = TaskState::COMPLETED;
        notify("COMPLETED.");
    }

    std::string getName() const { return name; }
};

