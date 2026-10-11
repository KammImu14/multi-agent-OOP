#pragma once
// ============================================================================
//  IObserver.h  -  "Observer" interface of the Observer Pattern
//  Any class that wants to RECEIVE messages must implement this interface.
// ============================================================================
#include <string>
#include "communication/Message.h"

namespace agentfw::communication {

class IObserver {
public:
    virtual ~IObserver() = default;

    // Unique identifier (used as the address for direct messages)
    virtual std::string getObserverId() const = 0;

    // Called by the Subject when a matching message arrives (polymorphism)
    virtual void update(const Message& message) = 0;
};

} // namespace agentfw::communication
