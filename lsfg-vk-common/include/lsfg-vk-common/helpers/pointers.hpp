#pragma once

#include <functional>
#include <stdexcept>

namespace ls {
    /// helper alias for std::reference_wrapper
    template<typename T>
    using R = std::reference_wrapper<T>;

    /// simplified alternative to std::optional<std::unique_ptr>
    template<typename T>
    class owned_ptr {
    public:
        /// default constructor
        owned_ptr() = default;

        /// construct from raw pointer
        /// @param ptr raw pointer to own, must be valid for object lifetime
        /// @param deleter custom deleter function, called only on owned instances
        explicit owned_ptr(T* ptr, std::function<void(T&)> deleter)
            : ptr(ptr), deleter(std::move(deleter)) {}

        /// get reference to owned object
        /// @throws std::runtime_error if no object is owned
        T& get() const {
            if (!ptr)
                throw std::runtime_error("owned_ptr: no object owned");
            return *ptr;
        }

        // operator overloads
        T& operator*() const { return this->get(); }
        T* operator->() const { return &this->get(); }

        // moveable
        owned_ptr(owned_ptr&& other) noexcept :
                ptr(other.ptr),
                deleter(std::move(other.deleter)) {
            other.ptr = nullptr;
        }
        owned_ptr& operator=(owned_ptr&& other) noexcept {
            if (this != &other) {
                ptr = other.ptr;
                other.ptr = nullptr;
                deleter = std::move(other.deleter);
            }

            return *this;
        }

        // non-copyable
        owned_ptr(const owned_ptr&) = delete;
        owned_ptr& operator=(const owned_ptr&) = delete;

        // destructor
        ~owned_ptr() {
            if (ptr && deleter) {
                deleter(*ptr);
                delete ptr;
            }
        }
    private:
        T* ptr{};
        std::function<void(T&)> deleter{};
    };
}
