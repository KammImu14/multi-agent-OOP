#include "communication/MessageBus.h"

#include <algorithm>
#include <exception>
#include <set>

#include "communication/TopicMatcher.h"

namespace agentfw::communication {

MessageBus::MessageBus(std::size_t maxHistory) : maxHistory_(maxHistory) {}

// ----------------------------------------------------------------------------
void MessageBus::registerObserver(const std::shared_ptr<IObserver>& observer) {
    if (!observer) return;
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    registry_[observer->getObserverId()] = observer;
}

bool MessageBus::isRegistered(const std::string& observerId) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto it = registry_.find(observerId);
    return it != registry_.end() && !it->second.expired();
}

void MessageBus::attach(const std::shared_ptr<IObserver>& observer, const std::string& topic) {
    if (!observer) return;
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    registerObserver(observer);

    auto& list = subscribers_[topic];
    const auto id = observer->getObserverId();
    const bool exists = std::any_of(list.begin(), list.end(), [&](const WeakObserver& w) {
        auto sp = w.lock();
        return sp && sp->getObserverId() == id;
    });
    if (!exists) list.push_back(observer);   // no duplicate subscriptions
}

void MessageBus::detach(const std::shared_ptr<IObserver>& observer, const std::string& topic) {
    if (!observer) return;
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    const auto id = observer->getObserverId();

    auto removeFrom = [&](std::vector<WeakObserver>& list) {
        list.erase(std::remove_if(list.begin(), list.end(), [&](const WeakObserver& w) {
                       auto sp = w.lock();
                       return !sp || sp->getObserverId() == id;
                   }),
                   list.end());
    };

    if (topic.empty()) {                       // remove completely
        for (auto& [pattern, list] : subscribers_) removeFrom(list);
        registry_.erase(id);
    } else if (auto it = subscribers_.find(topic); it != subscribers_.end()) {
        removeFrom(it->second);
    }
}

// ----------------------------------------------------------------------------
//  Resolve the recipient list (hold shared_ptr so observers cannot be
//  destroyed in the middle of delivery).
// ----------------------------------------------------------------------------
std::vector<std::shared_ptr<IObserver>> MessageBus::resolveRecipients(const Message& message) {
    std::vector<std::shared_ptr<IObserver>> result;

    if (message.isDirect()) {
        auto it = registry_.find(*message.receiver());
        if (it != registry_.end()) {
            if (auto sp = it->second.lock()) result.push_back(sp);
        }
        return result;
    }

    std::set<std::string> seen;  // avoid delivering twice (e.g. subscribed to both "*" and "task.*")
    for (auto& [pattern, list] : subscribers_) {
        if (!TopicMatcher::matches(pattern, message.topic())) continue;
        for (auto& weak : list) {
            auto sp = weak.lock();
            if (!sp) continue;
            const auto id = sp->getObserverId();
            if (id == message.sender()) continue;          // never deliver back to the sender
            if (seen.insert(id).second) result.push_back(sp);
        }
    }
    return result;
}

std::size_t MessageBus::notify(const Message& message) {
    std::vector<std::shared_ptr<IObserver>> recipients;
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        cleanupExpired();
        record(message);
        recipients = resolveRecipients(message);
        if (recipients.empty()) deadLetters_.push_back(message);
    }

    // Deliver OUTSIDE the lock: observers may publish again without deadlock
    std::size_t delivered = 0;
    for (auto& observer : recipients) {
        try {
            observer->update(message);
            ++delivered;
        } catch (const std::exception& e) {
            std::lock_guard<std::recursive_mutex> lock(mutex_);
            errors_.push_back({message.id(), observer->getObserverId(), e.what()});
        } catch (...) {
            std::lock_guard<std::recursive_mutex> lock(mutex_);
            errors_.push_back({message.id(), observer->getObserverId(), "unknown error"});
        }
    }
    return delivered;
}

// ----------------------------------------------------------------------------
void MessageBus::record(const Message& message) {
    if (maxHistory_ == 0) return;
    history_.push_back(message);
    while (history_.size() > maxHistory_) history_.pop_front();
}

void MessageBus::cleanupExpired() {
    for (auto& [pattern, list] : subscribers_) {
        list.erase(std::remove_if(list.begin(), list.end(),
                                  [](const WeakObserver& w) { return w.expired(); }),
                   list.end());
    }
    for (auto it = registry_.begin(); it != registry_.end();) {
        it = it->second.expired() ? registry_.erase(it) : std::next(it);
    }
}

std::vector<Message> MessageBus::getHistory(const std::optional<std::string>& topic,
                                            const std::optional<std::string>& sender) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::vector<Message> result;
    for (const auto& m : history_) {
        if (topic && !TopicMatcher::matches(*topic, m.topic())) continue;
        if (sender && m.sender() != *sender) continue;
        result.push_back(m);
    }
    return result;
}

std::vector<Message> MessageBus::getDeadLetters() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return deadLetters_;
}

std::vector<DeliveryError> MessageBus::getErrors() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return errors_;
}

std::size_t MessageBus::subscriberCount(const std::string& topic) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto it = subscribers_.find(topic);
    if (it == subscribers_.end()) return 0;
    return static_cast<std::size_t>(std::count_if(it->second.begin(), it->second.end(),
                                                  [](const WeakObserver& w) { return !w.expired(); }));
}

void MessageBus::clearHistory() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    history_.clear();
    deadLetters_.clear();
    errors_.clear();
}

} // namespace agentfw::communication
