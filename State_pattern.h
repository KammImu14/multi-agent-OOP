#pragma once
#include <string>
#include <memory>

using namespace std;

class StateContext; 
class TaskState {
public:
    virtual ~TaskState() = default;
    virtual string name() const = 0;
    virtual bool start(StateContext& c)    { return false; }
    virtual bool complete(StateContext& c) { return false; }
    virtual bool fail(StateContext& c)     { return false; }
    virtual bool reset(StateContext& c)    { return false; }
};

class StateContext {
public:
    virtual ~StateContext() = default;
    virtual string contextName() const = 0; 
    virtual void changeState(shared_ptr<TaskState> next) = 0; 
};

class IdleState : public TaskState {
public:
    string name() const override { return "IDLE"; }
    bool start(StateContext& c) override;
};

class InProgressState : public TaskState {
public:
    string name() const override { return "IN_PROGRESS"; }
    bool complete(StateContext& c) override;
    bool fail(StateContext& c) override;
};

class CompletedState : public TaskState {
public:
    string name() const override { return "COMPLETED"; }
    bool reset(StateContext& c) override;
};

class FailedState : public TaskState {
public:
    string name() const override { return "FAILED"; }
    bool start(StateContext& c) override; 
    bool reset(StateContext& c) override;
};

bool IdleState::start(StateContext& c) { 
    c.changeState(make_shared<InProgressState>()); return true; 
}
bool InProgressState::complete(StateContext& c) { 
    c.changeState(make_shared<CompletedState>()); return true; 
}
bool InProgressState::fail(StateContext& c) { 
    c.changeState(make_shared<FailedState>()); return true; 
}
bool CompletedState::reset(StateContext& c) { 
    c.changeState(make_shared<IdleState>()); return true; 
}
bool FailedState::start(StateContext& c) { 
    c.changeState(make_shared<InProgressState>()); return true; 
}
bool FailedState::reset(StateContext& c) { 
    c.changeState(make_shared<IdleState>()); return true; 
}
