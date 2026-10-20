#pragma once
// ============================================================================
//  CommunicatingAgent.h  -  ConcreteObserver: an agent that can communicate
//
//  Base class for every agent that sends/receives messages. The framework's
//  BaseAgent only needs to inherit from this class to get full messaging.
//
//  Template Method: update() (fixed) -> topic handler OR onMessage()
//  (subclasses override onMessage() to handle messages themselves).
//
//  NOTE: always create agents with std::make_shared<...>() and then call
//  connect(bus), because connect() needs shared_from_this() (which cannot
//  be used inside the constructor).
// ============================================================================
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "communication/IObserver.h"
#include "communication/MessageBus.h"

namespace agentfw::communication {

class CommunicatingAgent : public IObserver,
                           public std::enable_shared_from_this<CommunicatingAgent> {
public:
    using Handler = std::function<void(const Message&)>;

    explicit CommunicatingAgent(std::string name);
    ~CommunicatingAgent() override = default;

    // Non-copyable (each agent is a unique entity)
    CommunicatingAgent(const CommunicatingAgent&)            = delete;
    CommunicatingAgent& operator=(const CommunicatingAgent&) = delete;

    // ---- IObserver ----
    std::string getObserverId() const override { return name_; }
    void        update(const Message& message) final;

    // ---- Connection ----
    void connect(const std::shared_ptr<MessageBus>& bus);
    void disconnect();
    bool isConnected() const { return bus_ != nullptr; }

    // ---- Topic subscriptions ----
    void subscribe(const std::string& topic, Handler handler = nullptr);
    void unsubscribe(const std::string& topic);

    // ---- Sending ----
    std::size_t send(const std::string& receiver, const std::string& content,
                     const std::string& topic = kDirectTopic,
                     MessageType type = MessageType::Request);
    std::size_t publish(const std::string& topic, const std::string& content,
                        MessageType type = MessageType::Event);
    std::size_t broadcast(const std::string& content);
    std::size_t reply(const Message& original, const std::string& content);

    // ---- Inbox ----
    const std::string&   name() const { return name_; }
    std::vector<Message> inbox() const;
    std::size_t          inboxSize() const;
    void                 clearInbox();

protected:
    // Hook for subclasses: called when no topic handler matches
    virtual void onMessage(const Message& /*message*/) {}

    std::shared_ptr<MessageBus> bus() const { return bus_; }

private:
    std::size_t post(const Message& message);

    std::string                                  name_;
    std::shared_ptr<MessageBus>                  bus_;
    std::vector<std::pair<std::string, Handler>> handlers_;
    std::vector<Message>                         inbox_;
    mutable std::mutex                           inboxMutex_;
};

} // namespace agentfw::communication
