#include "communication/Message.h"

#include <atomic>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace agentfw::communication {

namespace {
// Collapse line breaks so a log entry always fits on one line
std::string oneLine(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    for (char c : text) {
        if (c == '\n') {
            if (out.size() < 3 || out.compare(out.size() - 3, 3, " / ") != 0) out += " / ";
        }
        else if (c != '\r') out += c;
    }
    return out;
}
} // namespace

std::string toString(MessageType type) {
    switch (type) {
        case MessageType::Request:   return "REQUEST";
        case MessageType::Response:  return "RESPONSE";
        case MessageType::Event:     return "EVENT";
        case MessageType::Broadcast: return "BROADCAST";
        case MessageType::Error:     return "ERROR";
    }
    return "UNKNOWN";
}

Message::Message(std::string sender, std::string topic, std::string content,
                 MessageType type, std::optional<std::string> receiver,
                 std::optional<std::string> replyTo, Metadata metadata)
    : id_(generateId()),
      sender_(std::move(sender)),
      topic_(std::move(topic)),
      content_(std::move(content)),
      type_(type),
      receiver_(std::move(receiver)),
      replyTo_(std::move(replyTo)),
      metadata_(std::move(metadata)),
      timestamp_(Clock::now()) {}

std::string Message::generateId() {
    static std::atomic<unsigned long> counter{0};
    std::ostringstream oss;
    oss << "msg-" << std::setw(5) << std::setfill('0') << ++counter;
    return oss.str();
}

Message Message::createReply(const std::string& from, const std::string& content,
                             MessageType type) const {
    return Message(from, topic_, content, type, sender_, id_);
}

std::string Message::toString() const {
    std::time_t t = Clock::to_time_t(timestamp_);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << "[" << std::put_time(&tm, "%H:%M:%S") << "] "
        << id_ << " " << communication::toString(type_)
        << " " << sender_ << " -> " << (receiver_ ? *receiver_ : "(topic)")
        << " | topic=" << topic_ << " | " << oneLine(content_);
    if (replyTo_) oss << " (reply to " << *replyTo_ << ")";
    return oss.str();
}

} // namespace agentfw::communication
