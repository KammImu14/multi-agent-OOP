// ============================================================================
//  Unit tests for the Communication module - no external dependencies.
//  (To switch to GoogleTest, rename TEST_CASE/CHECK to TEST/EXPECT_*.)
// ============================================================================
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "communication/CommunicatingAgent.h"
#include "communication/MessageBus.h"
#include "communication/TopicMatcher.h"
#include "agents/LLMAgent.h"
#include "llm/LLMStrategy.h"

using namespace agentfw::communication;

// ---- Mini test framework ----------------------------------------------------
namespace {
struct TestCase { std::string name; std::function<void()> fn; };
std::vector<TestCase>& registry() { static std::vector<TestCase> r; return r; }
struct Registrar { Registrar(std::string n, std::function<void()> f) { registry().push_back({std::move(n), std::move(f)}); } };
struct CheckFailed : std::runtime_error { using std::runtime_error::runtime_error; };
}
#define CONCAT_(a, b) a##b
#define CONCAT(a, b) CONCAT_(a, b)
#define TEST_CASE(name) \
    static void CONCAT(test_fn_, __LINE__)(); \
    static Registrar CONCAT(test_reg_, __LINE__)(name, CONCAT(test_fn_, __LINE__)); \
    static void CONCAT(test_fn_, __LINE__)()
#define CHECK(cond) \
    do { if (!(cond)) throw CheckFailed(std::string(__FILE__) + ":" + std::to_string(__LINE__) + "  CHECK(" #cond ")"); } while (0)
#define CHECK_THROWS(expr) \
    do { bool t_ = false; try { expr; } catch (...) { t_ = true; } CHECK(t_); } while (0)

// ---- Agents used by the tests ----------------------------------------------------
class TestAgent : public CommunicatingAgent {
public:
    using CommunicatingAgent::CommunicatingAgent;
    int onMessageCalls = 0;
protected:
    void onMessage(const Message&) override { ++onMessageCalls; }
};

class ThrowingAgent : public CommunicatingAgent {
public:
    using CommunicatingAgent::CommunicatingAgent;
protected:
    void onMessage(const Message&) override { throw std::runtime_error("boom"); }
};

std::shared_ptr<TestAgent> makeAgent(const std::string& name, const std::shared_ptr<MessageBus>& bus) {
    auto a = std::make_shared<TestAgent>(name);
    a->connect(bus);
    return a;
}

// ============================================================================
TEST_CASE("TopicMatcher: exact, wildcard, prefix") {
    CHECK(TopicMatcher::matches("task.done", "task.done"));
    CHECK(!TopicMatcher::matches("task.done", "task.fail"));
    CHECK(TopicMatcher::matches("*", "anything"));
    CHECK(TopicMatcher::matches("task.*", "task.created"));
    CHECK(TopicMatcher::matches("task.*", "task.status.changed"));
    CHECK(!TopicMatcher::matches("task.*", "task"));
    CHECK(!TopicMatcher::matches("task.*", "tasks.created"));
}

TEST_CASE("Message: immutable, reply is linked correctly") {
    Message m("A", "topic", "hello", MessageType::Request);
    CHECK(!m.isDirect());
    Message r = m.createReply("B", "hi");
    CHECK(r.isDirect());
    CHECK(*r.receiver() == "A");
    CHECK(*r.replyTo() == m.id());
    CHECK(r.type() == MessageType::Response);
    CHECK(m.id() != r.id());
}

TEST_CASE("Publish reaches only subscribers of that topic") {
    auto bus = std::make_shared<MessageBus>();
    auto pub = makeAgent("Pub", bus);
    auto a = makeAgent("A", bus);
    auto b = makeAgent("B", bus);
    a->subscribe("news");
    b->subscribe("sport");

    CHECK(pub->publish("news", "x") == 1);
    CHECK(a->inboxSize() == 1);
    CHECK(b->inboxSize() == 0);
}

TEST_CASE("Sender does not receive its own message") {
    auto bus = std::make_shared<MessageBus>();
    auto a = makeAgent("A", bus);
    a->subscribe("*");
    CHECK(a->publish("news", "x") == 0);
    CHECK(a->inboxSize() == 0);
}

TEST_CASE("No duplicate delivery with several matching patterns") {
    auto bus = std::make_shared<MessageBus>();
    auto pub = makeAgent("Pub", bus);
    auto a = makeAgent("A", bus);
    a->subscribe("*");
    a->subscribe("task.*");
    a->subscribe("task.done");
    pub->publish("task.done", "x");
    CHECK(a->inboxSize() == 1);
}

TEST_CASE("Direct message reaches only its receiver") {
    auto bus = std::make_shared<MessageBus>();
    auto a = makeAgent("A", bus);
    auto b = makeAgent("B", bus);
    auto c = makeAgent("C", bus);
    c->subscribe("*");                 // wildcard does NOT see direct messages
    CHECK(a->send("B", "secret") == 1);
    CHECK(b->inboxSize() == 1);
    CHECK(b->onMessageCalls == 1);
    CHECK(c->inboxSize() == 0);
}

TEST_CASE("Unknown receiver -> dead letter") {
    auto bus = std::make_shared<MessageBus>();
    auto a = makeAgent("A", bus);
    CHECK(a->send("Ghost", "hello?") == 0);
    CHECK(a->publish("no.one.listens", "x") == 0);
    CHECK(bus->getDeadLetters().size() == 2);
}

TEST_CASE("Unsubscribe and disconnect") {
    auto bus = std::make_shared<MessageBus>();
    auto pub = makeAgent("Pub", bus);
    auto a = makeAgent("A", bus);
    a->subscribe("news");
    a->unsubscribe("news");
    CHECK(pub->publish("news", "x") == 0);

    a->subscribe("news");
    a->disconnect();
    CHECK(!a->isConnected());
    CHECK(!bus->isRegistered("A"));
    CHECK(pub->publish("news", "x") == 0);
    CHECK(pub->send("A", "x") == 0);
    CHECK_THROWS(a->publish("news", "x"));   // not connected -> throws
}

TEST_CASE("A failing observer does not affect the others") {
    auto bus = std::make_shared<MessageBus>();
    auto pub = makeAgent("Pub", bus);
    auto bad = std::make_shared<ThrowingAgent>("Bad");
    bad->connect(bus);
    bad->subscribe("news");
    auto good = makeAgent("Good", bus);
    good->subscribe("news");

    CHECK(pub->publish("news", "x") == 1);
    CHECK(good->inboxSize() == 1);
    CHECK(bus->getErrors().size() == 1);
    CHECK(bus->getErrors()[0].observerId == "Bad");
}

TEST_CASE("Topic handler takes priority over onMessage") {
    auto bus = std::make_shared<MessageBus>();
    auto pub = makeAgent("Pub", bus);
    auto a = makeAgent("A", bus);
    int handled = 0;
    a->subscribe("task.*", [&](const Message&) { ++handled; });
    a->subscribe("other");             // no handler
    pub->publish("task.created", "x");
    pub->publish("other", "y");
    CHECK(handled == 1);
    CHECK(a->onMessageCalls == 1);
}

TEST_CASE("Nested publish (agent replies inside its handler)") {
    auto bus = std::make_shared<MessageBus>();
    auto asker = makeAgent("Asker", bus);
    auto echo = makeAgent("Echo", bus);
    echo->subscribe("ping", [&](const Message& m) { echo->reply(m, "pong"); });
    asker->publish("ping", "hello");
    auto inbox = asker->inbox();
    CHECK(inbox.size() == 1);
    CHECK(inbox[0].content() == "pong");
    CHECK(inbox[0].type() == MessageType::Response);
}

TEST_CASE("Destroyed agent is removed from the bus (weak_ptr)") {
    auto bus = std::make_shared<MessageBus>();
    auto pub = makeAgent("Pub", bus);
    {
        auto temp = makeAgent("Temp", bus);
        temp->subscribe("news");
        CHECK(bus->subscriberCount("news") == 1);
    }   // temp is destroyed here
    CHECK(bus->subscriberCount("news") == 0);
    CHECK(pub->publish("news", "x") == 0);
    CHECK(!bus->isRegistered("Temp"));
}

TEST_CASE("History filtering by topic/sender and size limit") {
    auto bus = std::make_shared<MessageBus>(3);
    auto a = makeAgent("A", bus);
    auto b = makeAgent("B", bus);
    a->publish("task.x", "1");
    b->publish("task.y", "2");
    a->publish("misc", "3");
    CHECK(bus->getHistory().size() == 3);
    CHECK(bus->getHistory(std::string("task.*")).size() == 2);
    CHECK(bus->getHistory(std::nullopt, std::string("A")).size() == 2);
    a->publish("misc", "4");                 // exceeds limit of 3 -> oldest is dropped
    CHECK(bus->getHistory().size() == 3);
    CHECK(bus->getHistory().front().content() == "2");
}

TEST_CASE("Subscribing before connect still works") {
    auto bus = std::make_shared<MessageBus>();
    auto pub = makeAgent("Pub", bus);
    auto a = std::make_shared<TestAgent>("A");
    a->subscribe("news");
    a->connect(bus);
    CHECK(pub->publish("news", "x") == 1);
}

TEST_CASE("Invalid / duplicate agent names") {
    CHECK_THROWS(TestAgent(""));
    auto bus = std::make_shared<MessageBus>();
    auto a = makeAgent("Dup", bus);
    CHECK_THROWS(makeAgent("Dup", bus));
}


// ============================================================================
//  LLMAgent: Observer Pattern + Strategy Pattern integration
// ============================================================================
using agentfw::agents::LLMAgent;

class EchoLLM : public LLMStrategy {
public:
    explicit EchoLLM(std::string tag) : tag_(std::move(tag)) {}
    int calls = 0;
    std::string lastPrompt;
    std::string generate(const std::string& prompt) override {
        ++calls; lastPrompt = prompt;
        return tag_ + ":" + prompt;
    }
private:
    std::string tag_;
};

class ErrorStringLLM : public LLMStrategy {   // like GeminiLLM without an API key
public:
    std::string generate(const std::string&) override { return "ERROR: Khong tim thay GEMINI_API_KEY"; }
};

class ThrowingLLM : public LLMStrategy {
public:
    std::string generate(const std::string&) override { throw std::runtime_error("network down"); }
};

TEST_CASE("LLMAgent: requires a strategy") {
    CHECK_THROWS(LLMAgent("A", nullptr));
}

TEST_CASE("LLMAgent: direct request -> generate -> reply to sender") {
    auto bus = std::make_shared<MessageBus>();
    auto user = makeAgent("User", bus);
    auto llm = std::make_shared<EchoLLM>("echo");
    auto agent = std::make_shared<LLMAgent>("Bot", llm);
    agent->connect(bus);

    user->send("Bot", "hello");
    CHECK(llm->calls == 1);
    auto in = user->inbox();
    CHECK(in.size() == 1);
    CHECK(in[0].type() == MessageType::Response);
    CHECK(in[0].content() == "echo:hello");
    CHECK(agent->handledCount() == 1);
}

TEST_CASE("LLMAgent: role is prepended to the prompt") {
    auto bus = std::make_shared<MessageBus>();
    auto user = makeAgent("User", bus);
    auto llm = std::make_shared<EchoLLM>("e");
    auto agent = std::make_shared<LLMAgent>("Bot", llm, "You are a tester.");
    agent->connect(bus);
    user->send("Bot", "hi");
    CHECK(llm->lastPrompt == "You are a tester.\n\nhi");
}

TEST_CASE("LLMAgent: pipeline input topic -> output topic") {
    auto bus = std::make_shared<MessageBus>();
    auto planner = makeAgent("Planner", bus);
    auto a = std::make_shared<LLMAgent>("A", std::make_shared<EchoLLM>("A"), "", "step.1", "step.2");
    auto b = std::make_shared<LLMAgent>("B", std::make_shared<EchoLLM>("B"), "", "step.2", "step.done");
    a->connect(bus); a->start();
    b->connect(bus); b->start();
    planner->subscribe("step.done");

    planner->publish("step.1", "x", MessageType::Request);
    auto in = planner->inbox();
    CHECK(in.size() == 1);
    CHECK(in[0].content() == "B:A:x");
    CHECK(in[0].sender() == "B");
}

TEST_CASE("LLMAgent: ignores events and other topics") {
    auto bus = std::make_shared<MessageBus>();
    auto user = makeAgent("User", bus);
    auto llm = std::make_shared<EchoLLM>("e");
    auto agent = std::make_shared<LLMAgent>("Bot", llm, "", "work", "");
    agent->connect(bus); agent->start();
    agent->subscribe("news");
    user->publish("work", "just an event", MessageType::Event);
    user->publish("news", "x", MessageType::Request);
    CHECK(llm->calls == 0);
}

TEST_CASE("LLMAgent: setStrategy swaps provider at runtime and notifies") {
    auto bus = std::make_shared<MessageBus>();
    auto user = makeAgent("User", bus);
    auto watcher = makeAgent("Watcher", bus);
    watcher->subscribe("agent.strategy.changed");
    auto agent = std::make_shared<LLMAgent>("Bot", std::make_shared<OpenAILLM>());
    agent->connect(bus);

    user->send("Bot", "q");
    CHECK(user->inbox().back().content().rfind("[OpenAI GPT]", 0) == 0);

    agent->setStrategy(std::make_shared<ClaudeLLM>(), "ClaudeLLM");
    user->send("Bot", "q");
    CHECK(user->inbox().back().content().rfind("[Claude AI]", 0) == 0);
    CHECK(watcher->inboxSize() == 1);
    CHECK_THROWS(agent->setStrategy(nullptr));
}

TEST_CASE("LLMAgent: error string from LLM -> agent.error + Error reply") {
    auto bus = std::make_shared<MessageBus>();
    auto user = makeAgent("User", bus);
    auto monitor = makeAgent("Monitor", bus);
    monitor->subscribe("agent.error");
    auto agent = std::make_shared<LLMAgent>("Bot", std::make_shared<ErrorStringLLM>());
    agent->connect(bus);

    user->send("Bot", "q");
    CHECK(monitor->inboxSize() == 1);
    CHECK(user->inboxSize() == 1);
    CHECK(user->inbox()[0].type() == MessageType::Error);
    CHECK(agent->handledCount() == 0);
}

TEST_CASE("LLMAgent: exception from LLM is caught and reported") {
    auto bus = std::make_shared<MessageBus>();
    auto user = makeAgent("User", bus);
    auto monitor = makeAgent("Monitor", bus);
    monitor->subscribe("agent.*");
    auto agent = std::make_shared<LLMAgent>("Bot", std::make_shared<ThrowingLLM>());
    agent->connect(bus);

    user->send("Bot", "q");
    CHECK(monitor->inboxSize() == 1);
    CHECK(monitor->inbox()[0].content() == "Bot: network down");
    CHECK(bus->getErrors().empty());          // handled inside the agent, bus stays clean
}

// ============================================================================
int main() {
    int passed = 0, failed = 0;
    for (auto& t : registry()) {
        try {
            t.fn();
            ++passed;
            std::cout << "[PASS] " << t.name << "\n";
        } catch (const std::exception& e) {
            ++failed;
            std::cout << "[FAIL] " << t.name << "\n       " << e.what() << "\n";
        }
    }
    std::cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
