#pragma once
// ============================================================================
//  ISubject.h  -  "Subject" interface of the Observer Pattern
//  Keeps a list of observers and notifies them when a message is published.
// ============================================================================
#include <cstddef>
#include <memory>
#include <string>
#include "communication/IObserver.h"
#include "communication/Message.h"

namespace agentfw::communication {

class ISubject {
public:
    virtual ~ISubject() = default;

    // Subscribe an observer to a topic (default "*": every topic)
    virtual void attach(const std::shared_ptr<IObserver>& observer,
                        const std::string& topic = kWildcardTopic) = 0;

    // Unsubscribe. Empty topic => remove from ALL topics and unregister
    virtual void detach(const std::shared_ptr<IObserver>& observer,
                        const std::string& topic = "") = 0;

    // Deliver a message to matching observers. Returns how many received it.
    virtual std::size_t notify(const Message& message) = 0;
};

} // namespace agentfw::communication
