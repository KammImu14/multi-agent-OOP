#pragma once
#include <string>
#include <memory>

using namespace std;

class TaskContext; 

class TaskState {
public:
    virtual ~TaskState() = default;
    virtual string name() const = 0;
    virtual bool start(TaskContext& c)    { return false; }
    virtual bool complete(TaskContext& c) { return false; }
    virtual bool fail(TaskContext& c)     { return false; }
    virtual bool reset(TaskContext& c)    { return false; }
};

class TaskContext {
public:
    virtual ~TaskContext() = default;
    virtual void changestate(shared_ptr<TaskState> next) = 0;
};

class IdleState : public TaskState {
public:
    string name() const override { return "IDLE"; }
    bool start(TaskContext& c) override;
};
class InProgressState : public TaskState {
public:
    string name() const override { return "IN_PROGRESS"; }
    bool complete(TaskContext& c) override;
    bool fail(TaskContext& c) override;
};
class CompletedState : public TaskState {
public:
    string name() const override { return "COMPLETED"; }
    bool reset(TaskContext& c) override;
};
class FailedState : public TaskState {
public:
    string name() const override { return "FAILED"; }
    bool start(TaskContext& c) override; // retry
    bool reset(TaskContext& c) override;
};

bool IdleState::start(TaskContext& c) { 
    c.changestate(make_shared<InProgressState>()); return true; 
}
bool InProgressState::complete(TaskContext& c) { 
    c.changestate(make_shared<CompletedState>()); return true; 
}
bool InProgressState::fail(TaskContext& c) { 
    c.changestate(make_shared<FailedState>()); return true; 
}
bool CompletedState::reset(TaskContext& c) { 
    c.changestate(make_shared<IdleState>()); return true; 
}
bool FailedState::start(TaskContext& c) { 
    c.changestate(make_shared<InProgressState>()); return true; 
}
bool FailedState::reset(TaskContext& c) { 
    c.changestate(make_shared<IdleState>()); return true; 
}
