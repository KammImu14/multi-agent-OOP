#pragma once
// ============================================================================
//  TopicMatcher.h  -  Matches a topic against a subscription pattern
//    "*"          matches every topic
//    "task.*"     matches "task.created", "task.status.changed", ...
//    "task.done"  matches exactly "task.done"
// ============================================================================
#include <string>

namespace agentfw::communication {

class TopicMatcher {
public:
    static bool matches(const std::string& pattern, const std::string& topic);
};

} // namespace agentfw::communication
