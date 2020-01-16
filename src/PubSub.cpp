// TODO: smarter locking (e.g. multiple readers are usually fine)
// TODO: thread-safety of message payload is at sole discretion of subscribers!

#include "PubSub.hpp"

#include <algorithm>

using std::nullopt;
using std::shared_lock;
using std::unique_lock;

namespace PubSub {

MessageDataBlock* Broker::allocate_message(std::type_index type, size_t size, void (*destroy)(void*)) {
    // TODO: re-use buffers!
    void* buffer = malloc(size);

    return new MessageDataBlock {0, *this, type, buffer, destroy};
}

void Broker::publish(MessageDataBlock* msg) {
    shared_lock lock(mutex);

    auto it = subscriptions_by_type.find(msg->type);

    if (it != subscriptions_by_type.end()) {
        for (auto& pipe : it->second) {
            msg->refcount++;
            pipe->push(msg);
        }
    }

    if (msg->refcount == 0) {
        msg->destroy(msg->data);
        free(msg->data);
        delete msg;
    }
}

void Broker::removeReference(MessageDataBlock* msg) {
    if (--msg->refcount == 0) {
        msg->destroy(msg->data);
        free(msg->data);
        delete msg;
    }
}

void Broker::subscribe(Pipe& pipe, std::type_index type) {
    unique_lock lock(mutex);

    subscriptions_by_type[type].insert(&pipe);
}

void Broker::unsubscribeAll(Pipe& pipe) {
    unique_lock lock(mutex);

    // go over all type bins
    for (auto& [type, subscribers] : subscriptions_by_type) {
        // if pipe is present in set of subscribers, erase it
        subscribers.erase(&pipe);
    }
}

void Broker::unsubscribe(Pipe& pipe, std::type_index type) {
    unique_lock lock(mutex);

    // see if a bin for this type exists at all
    auto it = subscriptions_by_type.find(type);

    if (it != subscriptions_by_type.end()) {
        // if pipe is present in set of subscribers, erase it
        auto& [_, subscribers] = *it;
        subscribers.erase(&pipe);
    }
}

MessageRef::~MessageRef() {
    if (msg != nullptr) {
        msg->broker.removeReference(msg);
    }
}

std::optional<MessageRef> Pipe::poll() {
    unique_lock lock(mutex);

    if (!this->messages.empty()) {
        auto msg = messages.front();
        messages.pop_front();
        return MessageRef {msg};
    }
    else {
        return nullopt;
    };
}

void Pipe::push(MessageDataBlock* message) {
    unique_lock lock(mutex);

    this->messages.push_back(message);
}

Subscription::Subscription(Broker& broker, Pipe& pipe) : broker(broker), pipe(pipe) {
}

Subscription::~Subscription() {
    broker.unsubscribeAll(pipe);
}

}
