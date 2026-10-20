// ============================================================================
//  Demo: Observer Pattern (MessageBus) + Strategy Pattern (LLMStrategy)
//
//   Planner --"task.research"--> Researcher (OpenAILLM)
//                                   | "task.write"
//                                   v
//                                Writer (ClaudeLLM)
//                                   | "task.result"
//                                   v
//                                Planner
//
//   Logger      subscribes to "*"             -> sees every topic message
//   StatusBoard subscribes to "task.status.*" -> hook point for the State Pattern
//
//  Part 2 switches the Researcher's LLM at runtime (Strategy Pattern).
//  Part 3 sends a direct question and simulates an LLM failure.
// ============================================================================
#include <initializer_list>
#include <iostream>
#include <memory>
#include <string>

#include "agents/LLMAgent.h"
#include "communication/CommunicatingAgent.h"
#include "communication/MessageBus.h"
#include "llm/LLMStrategy.h"

using namespace agentfw::communication;
using agentfw::agents::LLMAgent;

namespace {

// Behaves like GeminiLLM from feature/llm-adapter when GEMINI_API_KEY is missing
class BrokenGeminiLLM : public LLMStrategy {
public:
    std::string generate(const std::string&) override {
        return "ERROR: Khong tim thay GEMINI_API_KEY";
    }
};

// ---------------------------------------------------------------------------
class PlannerAgent : public CommunicatingAgent {
public:
    using CommunicatingAgent::CommunicatingAgent;

    void init() {
        subscribe("task.result", [this](const Message& m) {
            finalReport_ = m.content();
            std::cout << ">>> " << name() << " received the final report from " << m.sender() << "\n";
            publish("task.status.changed", currentTask_ + ": RUNNING -> SUCCESS");
        });
    }

    void startGoal(const std::string& taskId, const std::string& goal) {
        currentTask_ = taskId;
        std::cout << "\n>>> " << name() << " starts " << taskId << ": " << goal << "\n";
        publish("task.status.changed", taskId + ": PENDING -> RUNNING");
        publish("task.research", goal, MessageType::Request);
    }

    const std::string& finalReport() const { return finalReport_; }

protected:
    void onMessage(const Message& m) override {          // direct replies
        std::cout << ">>> " << name() << " got " << toString(m.type())
                  << " from " << m.sender() << ": " << m.content() << "\n";
    }

private:
    std::string currentTask_;
    std::string finalReport_;
};

class LoggerAgent : public CommunicatingAgent {
public:
    using CommunicatingAgent::CommunicatingAgent;
    void init() { subscribe(kWildcardTopic); }
protected:
    void onMessage(const Message& m) override { std::cout << "    [LOG] " << m.toString() << "\n"; }
};

class StatusBoardAgent : public CommunicatingAgent {
public:
    using CommunicatingAgent::CommunicatingAgent;
    void init() {
        subscribe("task.status.*", [](const Message& m) {
            std::cout << "    [STATUS] " << m.content() << "\n";
        });
    }
};

} // namespace

int main() {
    auto bus = std::make_shared<MessageBus>();

    auto planner    = std::make_shared<PlannerAgent>("Planner");
    auto researcher = std::make_shared<LLMAgent>("Researcher", std::make_shared<OpenAILLM>(),
                                                 "You are a researcher. Collect key facts.",
                                                 "task.research", "task.write");
    auto writer     = std::make_shared<LLMAgent>("Writer", std::make_shared<ClaudeLLM>(),
                                                 "You are a writer. Turn notes into a report.",
                                                 "task.write", "task.result");
    auto logger     = std::make_shared<LoggerAgent>("Logger");
    auto status     = std::make_shared<StatusBoardAgent>("StatusBoard");

    for (auto a : std::initializer_list<std::shared_ptr<CommunicatingAgent>>{
             planner, researcher, writer, logger, status})
        a->connect(bus);

    planner->init();
    researcher->start();
    writer->start();
    logger->init();
    status->init();

    // ---- Part 1: pipeline through the bus ----------------------------------
    std::cout << "\n========== PART 1: multi-agent pipeline ==========";
    planner->startGoal("task-1", "Summarize AI agent trends in 2026");
    std::cout << "\nRESULT:\n" << planner->finalReport() << "\n";

    // ---- Part 2: swap the LLM at runtime (Strategy Pattern) ----------------
    std::cout << "\n========== PART 2: switch Researcher to ClaudeLLM ==========\n";
    researcher->setStrategy(std::make_shared<ClaudeLLM>(), "ClaudeLLM");
    planner->startGoal("task-2", "Compare OpenAI and Gemini APIs");
    std::cout << "\nRESULT:\n" << planner->finalReport() << "\n";

    // ---- Part 3: direct request + error handling ---------------------------
    std::cout << "\n========== PART 3: direct question & LLM failure ==========\n";
    planner->send("Writer", "Give me a one-line title");
    researcher->setStrategy(std::make_shared<BrokenGeminiLLM>(), "GeminiLLM (no API key)");
    planner->send("Researcher", "Find sources about multi-agent systems");

    // ---- Statistics --------------------------------------------------------
    std::cout << "\n========== BUS STATISTICS ==========\n"
              << "Total messages      : " << bus->getHistory().size() << "\n"
              << "'task.*' messages   : " << bus->getHistory(std::string("task.*")).size() << "\n"
              << "'agent.error' events: " << bus->getHistory(std::string("agent.error")).size() << "\n"
              << "Researcher handled  : " << researcher->handledCount() << " request(s)\n"
              << "Writer handled      : " << writer->handledCount() << " request(s)\n"
              << "Dead letters        : " << bus->getDeadLetters().size() << "\n"
              << "Delivery errors     : " << bus->getErrors().size() << "\n";
    return 0;
}
