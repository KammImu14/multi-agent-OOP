#pragma once
// ============================================================================
//  MessageBus.h  -  ConcreteSubject: the central "post office" of the
//  multi-agent system
//
//  * Topic-based publish/subscribe (supports "*" and "prefix.*" wildcards)
//  * Direct agent -> agent messages (addressed by observer id)
//  * Message history and "dead letters" (messages nobody received)
//  * Fault isolation: one observer throwing does not affect the others
//  * Holds weak_ptr to observers => no reference cycles / memory leaks
//  * Thread-safe; observers may publish from inside update()
// ============================================================================
#include <cstddef>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "communication/ISubject.h"

namespace agentfw::communication {

// Information recorded when an observer fails to handle a message
struct DeliveryError {
    std::string messageId;
    std::string observerId;
    std::string what;
};

class MessageBus : public ISubject {
public:
    explicit MessageBus(std::size_t maxHistory = 1000);

    // ---- ISubject ----
    void attach(const std::shared_ptr<IObserver>& observer,
                const std::string& topic = kWildcardTopic) override;
    void detach(const std::shared_ptr<IObserver>& observer,
                const std::string& topic = "") override;
    std::size_t notify(const Message& message) override;

    // More readable alias for notify()
    std::size_t publish(const Message& message) { return notify(message); }

    // Register an observer so it can receive DIRECT messages (no topic needed)
    void registerObserver(const std::shared_ptr<IObserver>& observer);
    bool isRegistered(const std::string& observerId) const;

    // ---- Queries / monitoring ----
    std::vector<Message> getHistory(const std::optional<std::string>& topic  = std::nullopt,
                                    const std::optional<std::string>& sender = std::nullopt) const;
    std::vector<Message>       getDeadLetters() const;
    std::vector<DeliveryError> getErrors() const;
    std::size_t                subscriberCount(const std::string& topic) const;
    void                       clearHistory();

private:
    using WeakObserver = std::weak_ptr<IObserver>;

    std::vector<std::shared_ptr<IObserver>> resolveRecipients(const Message& message);
    void record(const Message& message);
    void cleanupExpired();

    mutable std::recursive_mutex                     mutex_;
    std::map<std::string, std::vector<WeakObserver>> subscribers_; // pattern -> observers
    std::map<std::string, WeakObserver>              registry_;    // id -> observer
    std::deque<Message>                              history_;
    std::vector<Message>                             deadLetters_;
    std::vector<DeliveryError>                       errors_;
    std::size_t                                      maxHistory_;
};

} // namespace agentfw::communication
