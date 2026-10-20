#pragma once
// ============================================================================
//  Message.h  -  The message object exchanged between agents
//  Module: Communication (Observer Pattern)
// ============================================================================
#include <chrono>
#include <map>
#include <optional>
#include <string>

namespace agentfw::communication {

// Kind of message
enum class MessageType {
    Request,    // ask another agent to do something
    Response,   // answer to a Request
    Event,      // notification (e.g. a task changed state)
    Broadcast,  // sent to everyone
    Error       // error report
};

std::string toString(MessageType type);

// Special topic: subscribing to "*" receives every topic message
// (direct messages are NOT included)
inline const std::string kWildcardTopic = "*";
// Default topic for direct agent -> agent messages
inline const std::string kDirectTopic = "direct";

// ----------------------------------------------------------------------------
//  Message is immutable after construction: getters only, no setters.
//  => Encapsulation: nobody can modify a message while it is in transit.
// ----------------------------------------------------------------------------
class Message {
public:
    using Clock    = std::chrono::system_clock;
    using Metadata = std::map<std::string, std::string>;

    Message(std::string sender,
            std::string topic,
            std::string content,
            MessageType type                    = MessageType::Event,
            std::optional<std::string> receiver = std::nullopt,
            std::optional<std::string> replyTo  = std::nullopt,
            Metadata metadata                   = {});

    // ---- Getters ----
    const std::string&                id()        const { return id_; }
    const std::string&                sender()    const { return sender_; }
    const std::string&                topic()     const { return topic_; }
    const std::string&                content()   const { return content_; }
    MessageType                       type()      const { return type_; }
    const std::optional<std::string>& receiver()  const { return receiver_; }
    const std::optional<std::string>& replyTo()   const { return replyTo_; }
    const Metadata&                   metadata()  const { return metadata_; }
    Clock::time_point                 timestamp() const { return timestamp_; }

    // Direct message (has a specific receiver) vs. topic message
    bool isDirect() const { return receiver_.has_value(); }

    // Build a reply: receiver = original sender, replyTo = original id
    Message createReply(const std::string& from,
                        const std::string& content,
                        MessageType type = MessageType::Response) const;

    std::string toString() const;

private:
    static std::string generateId();

    std::string                id_;
    std::string                sender_;
    std::string                topic_;
    std::string                content_;
    MessageType                type_;
    std::optional<std::string> receiver_;
    std::optional<std::string> replyTo_;
    Metadata                   metadata_;
    Clock::time_point          timestamp_;
};

} // namespace agentfw::communication
