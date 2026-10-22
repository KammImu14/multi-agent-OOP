#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
using namespace std;

// ===== Observer Pattern =====
class Observer {
public:
    virtual ~Observer() = default;
    virtual void onMessage(const string& from,
                           const string& topic,
                           const string& payload) = 0;
};

class Subject {
protected:
    vector<Observer*> observers;
public:
    virtual ~Subject() = default;

    void attach(Observer* o) {
        if (o && find(observers.begin(), observers.end(), o) == observers.end())
            observers.push_back(o);
    }

    void detach(Observer* o) {
        observers.erase(remove(observers.begin(), observers.end(), o),
                        observers.end());
    }

    void notify(const string& from,
                const string& topic,
                const string& payload) {
        // copy để tránh invalidate nếu observer detach trong lúc notify
        auto snapshot = observers;
        for (auto* o : snapshot) o->onMessage(from, topic, payload);
    }
};