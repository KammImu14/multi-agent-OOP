#include "communication/TopicMatcher.h"
#include "communication/Message.h"

namespace agentfw::communication {

bool TopicMatcher::matches(const std::string& pattern, const std::string& topic) {
    if (pattern == kWildcardTopic) return true;

    // "abc.*"  => topic must start with "abc."
    const std::string suffix = ".*";
    if (pattern.size() > suffix.size() &&
        pattern.compare(pattern.size() - suffix.size(), suffix.size(), suffix) == 0) {
        const std::string prefix = pattern.substr(0, pattern.size() - 1); // keep the '.'
        return topic.compare(0, prefix.size(), prefix) == 0 && topic.size() > prefix.size();
    }
    return pattern == topic;
}

} // namespace agentfw::communication
