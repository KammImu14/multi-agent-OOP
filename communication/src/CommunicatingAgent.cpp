#include "communication/CommunicatingAgent.h"

#include <algorithm>
#include <stdexcept>

#include "communication/TopicMatcher.h"

namespace agentfw::communication {

CommunicatingAgent::CommunicatingAgent(std::string name) : name_(std::move(name)) {
    if (name_.empty()) throw std::invalid_argument("Agent name must not be empty");
}

// ----------------------------------------------------------------------------
void CommunicatingAgent::connect(const std::shared_ptr<MessageBus>& bus) {
    if (!bus) throw std::invalid_argument("MessageBus must not be null");
    if (bus->isRegistered(name_) && bus_ != bus)
        throw std::runtime_error("Agent name already in use on this bus: " + name_);
    bus_ = bus;
    bus_->registerObserver(shared_from_this());
    // Re-attach topics that were subscribed before connect()
    for (auto& [topic, h] : handlers_) bus_->attach(shared_from_this(), topic);
}

void CommunicatingAgent::disconnect() {
    if (!bus_) return;
    bus_->detach(shared_from_this());
    bus_.reset();
}

void CommunicatingAgent::subscribe(const std::string& topic, Handler handler) {
    auto it = std::find_if(handlers_.begin(), handlers_.end(),
                           [&](const auto& p) { return p.first == topic; });
    if (it != handlers_.end()) it->second = std::move(handler);
    else handlers_.emplace_back(topic, std::move(handler));

    if (bus_) bus_->attach(shared_from_this(), topic);
}

void CommunicatingAgent::unsubscribe(const std::string& topic) {
    handlers_.erase(std::remove_if(handlers_.begin(), handlers_.end(),
                                   [&](const auto& p) { return p.first == topic; }),
                    handlers_.end());
    if (bus_) bus_->detach(shared_from_this(), topic);
}

// ----------------------------------------------------------------------------
//  Template Method: store in inbox -> find a matching topic handler ->
//  otherwise call onMessage() (overridden by subclasses).
// ----------------------------------------------------------------------------
void CommunicatingAgent::update(const Message& message) {
    {
        std::lock_guard<std::mutex> lock(inboxMutex_);
        inbox_.push_back(message);
    }

    if (!message.isDirect()) {
        for (const auto& [pattern, handler] : handlers_) {
            if (handler && TopicMatcher::matches(pattern, message.topic())) {
                Handler h = handler;   // copy: the handler may subscribe/unsubscribe while running
                h(message);
                return;
            }
        }
    }
    onMessage(message);
}

// ----------------------------------------------------------------------------
std::size_t CommunicatingAgent::post(const Message& message) {
    if (!bus_) throw std::logic_error("Agent '" + name_ + "' is not connected to a MessageBus");
    return bus_->publish(message);
}

std::size_t CommunicatingAgent::send(const std::string& receiver, const std::string& content,
                                     const std::string& topic, MessageType type) {
    return post(Message(name_, topic, content, type, receiver));
}

std::size_t CommunicatingAgent::publish(const std::string& topic, const std::string& content,
                                        MessageType type) {
    return post(Message(name_, topic, content, type));
}

std::size_t CommunicatingAgent::broadcast(const std::string& content) {
    return post(Message(name_, "broadcast", content, MessageType::Broadcast));
}

std::size_t CommunicatingAgent::reply(const Message& original, const std::string& content) {
    return post(original.createReply(name_, content));
}

// ----------------------------------------------------------------------------
std::vector<Message> CommunicatingAgent::inbox() const {
    std::lock_guard<std::mutex> lock(inboxMutex_);
    return inbox_;
}

std::size_t CommunicatingAgent::inboxSize() const {
    std::lock_guard<std::mutex> lock(inboxMutex_);
    return inbox_.size();
}

void CommunicatingAgent::clearInbox() {
    std::lock_guard<std::mutex> lock(inboxMutex_);
    inbox_.clear();
}

} // namespace agentfw::communication
