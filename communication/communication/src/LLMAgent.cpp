#include "agents/LLMAgent.h"

#include <stdexcept>

namespace agentfw::agents {

LLMAgent::LLMAgent(std::string name, std::shared_ptr<LLMStrategy> strategy,
                   std::string role, std::string inputTopic, std::string outputTopic)
    : CommunicatingAgent(std::move(name)),
      strategy_(std::move(strategy)),
      role_(std::move(role)),
      inputTopic_(std::move(inputTopic)),
      outputTopic_(std::move(outputTopic)) {
    if (!strategy_) throw std::invalid_argument("LLMAgent '" + this->name() + "' needs an LLMStrategy");
}

void LLMAgent::start() {
    if (!inputTopic_.empty()) subscribe(inputTopic_);   // no handler -> onMessage()
}

// ---------------------------------------------------------------------------
//  Strategy Pattern
// ---------------------------------------------------------------------------
void LLMAgent::setStrategy(std::shared_ptr<LLMStrategy> strategy, const std::string& label) {
    if (!strategy) throw std::invalid_argument("Strategy must not be null");
    {
        std::lock_guard<std::mutex> lock(strategyMutex_);
        strategy_ = std::move(strategy);
    }
    // Let the rest of the system know (Observer Pattern)
    if (isConnected())
        publish(kStrategyChangedTopic, name() + " now uses " + (label.empty() ? "a new LLM" : label));
}

std::shared_ptr<LLMStrategy> LLMAgent::strategy() const {
    std::lock_guard<std::mutex> lock(strategyMutex_);
    return strategy_;
}

std::string LLMAgent::ask(const std::string& input) {
    auto llm = strategy();                       // copy: safe even if swapped meanwhile
    std::string prompt = role_.empty() ? input : role_ + "\n\n" + input;
    std::string answer = llm->generate(prompt);
    // GeminiLLM (feature/llm-adapter) reports failures as strings, not exceptions
    if (looksLikeError(answer)) throw std::runtime_error(answer);
    return answer;
}

bool LLMAgent::looksLikeError(const std::string& text) {
    return text.rfind("ERROR", 0) == 0 || text.rfind("GEMINI ERROR", 0) == 0;
}

// ---------------------------------------------------------------------------
//  Observer Pattern: react to incoming messages
// ---------------------------------------------------------------------------
std::string LLMAgent::buildPrompt(const Message& message) const {
    return message.content();
}

void LLMAgent::onMessage(const Message& message) {
    if (message.type() != MessageType::Request) return;   // ignore events/responses
    if (!message.isDirect() && message.topic() != inputTopic_) return;
    handleRequest(message);
}

void LLMAgent::handleRequest(const Message& message) {
    std::string result;
    try {
        result = ask(buildPrompt(message));
    } catch (const std::exception& e) {
        publish(kAgentErrorTopic, name() + ": " + e.what(), MessageType::Error);
        send(message.sender(), e.what(), message.topic(), MessageType::Error);
        return;
    }
    ++handled_;

    if (!message.isDirect() && !outputTopic_.empty())
        publish(outputTopic_, result, MessageType::Request);   // pass along the pipeline
    else
        reply(message, result);
}

} // namespace agentfw::agents
