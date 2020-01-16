#ifndef GLACIUS_PUBSUB_HPP
#define GLACIUS_PUBSUB_HPP

#include <atomic>
#include <cstdint>
#include <deque>
#include <mutex>
#include <optional>
#include <set>
#include <shared_mutex>
#include <typeindex>
#include <unordered_map>

namespace PubSub {

class Broker;

struct MessageDataBlock {
    std::atomic_size_t refcount;
    Broker& broker;

    std::type_index type;
    void* data;
    void (*destroy)(void*);
};

class MessageRef {
public:
    MessageRef(MessageDataBlock* msg) : msg(msg) {}
    MessageRef(MessageRef&& other) : msg(other.msg) { other.msg = nullptr; }
    MessageRef(const MessageRef& other) = delete;
    ~MessageRef();

    template <typename T>
    T* cast() {
        if (std::type_index(typeid(T)) == msg->type) {
            return reinterpret_cast<T*>(msg->data);
        }
        else {
            return nullptr;
        }
    }

private:
    MessageDataBlock* msg;
};

class Pipe {
public:
    std::optional<MessageRef> poll();
    void push(MessageDataBlock* message);
//    void waitOn(timeout);

private:
    std::deque<MessageDataBlock*> messages;
    std::mutex mutex;
};

class Broker {
public:
    template <typename T>
    void subscribe(Pipe& pipe) {
        this->subscribe(pipe, typeid(T));
    }

    void unsubscribeAll(Pipe& pipe);

    template <typename T>
    void unsubscribe(Pipe& pipe) {
        this->unsubscribe(pipe, typeid(T));
    }

//    template <typename T>
//    void publish(T&& message) {
//        // TODO: consider first checking if any subscribers exist
//
//        auto msg = this->allocate_message(typeid(T), sizeof(T), &destroy_helper<T>);
//
//        new(msg->data) T {std::move(message)};
//
//        this->publish(msg);
//    }

    template <typename T, typename ...Args>
    void publish(Args&& ...args) {
        // TODO: consider first checking if any subscribers exist

        auto msg = this->allocate_message(typeid(T), sizeof(T), &destroy_helper<T>);

        new(msg->data) T {std::forward<Args>(args)...};

        this->publish(msg);
    }

    void removeReference(MessageDataBlock* msg);

private:
    MessageDataBlock* allocate_message(std::type_index type, size_t size, void (*destroy)(void*));
    void publish(MessageDataBlock* msg);
    void subscribe(Pipe& pipe, std::type_index type);
    void unsubscribe(Pipe& pipe, std::type_index type);

    template <typename T>
    static void destroy_helper(void* message) {
        reinterpret_cast<T*>(message)->~T();
    }

    std::shared_mutex mutex;
    std::unordered_map<std::type_index, std::set<Pipe*>> subscriptions_by_type;
};

class Subscription {
public:
    Subscription(Broker& broker, Pipe& pipe);
    ~Subscription();

    template <typename T>
    void add() {
        broker.subscribe<T>(pipe);
    }

private:
    Broker& broker;
    Pipe& pipe;
};

}

#endif
