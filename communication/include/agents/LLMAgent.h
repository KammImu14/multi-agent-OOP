#pragma once
// ============================================================================
//  LLMAgent.h  -  An agent that talks to other agents (Observer Pattern)
//                 and thinks with an interchangeable LLM (Strategy Pattern)
//
//  How it reacts to incoming REQUEST messages:
//    * Topic message on its input topic -> generate() -> publish the result
//      on its output topic (pipeline), or reply to the sender if no output
//      topic is configured.
//    * Direct message                   -> generate() -> reply to the sender.
//    * LLM failure (exception, or an "ERROR..." string as returned by
//      GeminiLLM)                        -> publish on "agent.error" and reply
//      to the sender with MessageType::Error.
//
//  The LLM provider can be swapped at runtime with setStrategy().
// ============================================================================
#include <memory>
#include <mutex>
#include <string>

#include "communication/CommunicatingAgent.h"
#include "llm/LLMStrategy.h"

namespace agentfw::agents {

using communication::Message;
using communication::MessageType;

inline const std::string kAgentErrorTopic    = "agent.error";
inline const std::string kStrategyChangedTopic = "agent.strategy.changed";

class LLMAgent : public communication::CommunicatingAgent {
public:
    // role        : system-style instruction prepended to every prompt
    // inputTopic  : topic this agent works on ("" = direct requests only)
    // outputTopic : where results are published ("" = reply to sender)
    LLMAgent(std::string name,
             std::shared_ptr<LLMStrategy> strategy,
             std::string role        = "",
             std::string inputTopic  = "",
             std::string outputTopic = "");

    // Must be called after connect(bus): subscribes to the input topic.
    void start();

    // Strategy Pattern: swap the LLM provider without touching agent code.
    void setStrategy(std::shared_ptr<LLMStrategy> strategy, const std::string& label = "");
    std::shared_ptr<LLMStrategy> strategy() const;

    // Call the LLM directly (also used internally). Throws on failure.
    std::string ask(const std::string& input);

    const std::string& role()        const { return role_; }
    const std::string& inputTopic()  const { return inputTopic_; }
    const std::string& outputTopic() const { return outputTopic_; }
    std::size_t        handledCount() const { return handled_; }

protected:
    void onMessage(const Message& message) override;

    // Hook: subclasses may customise how the prompt is built.
    virtual std::string buildPrompt(const Message& message) const;

private:
    void handleRequest(const Message& message);
    static bool looksLikeError(const std::string& text);

    std::shared_ptr<LLMStrategy> strategy_;
    mutable std::mutex           strategyMutex_;
    std::string                  role_;
    std::string                  inputTopic_;
    std::string                  outputTopic_;
    std::size_t                  handled_ = 0;
};

} // namespace agentfw::agents
