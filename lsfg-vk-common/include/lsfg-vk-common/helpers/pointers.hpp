/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <cassert>
#include <functional>
#include <optional>
#include <stdexcept>

namespace ls {
    /// helper alias for std::reference_wrapper
    template<typename T>
    using R = std::reference_wrapper<T>;

    /// helper (eyecandy) alias for std::optional
    template<typename T>
    class lazy {
    public:
        /// default constructor
        lazy() = default;

        /// emplace value
        /// @param args constructor arguments
        /// @return reference to constructed value
        /// @throws std::logic_error if value already present
        template<typename... Args>
        T& emplace(Args&&... args) {
            if (this->opt.has_value())
                throw std::logic_error("lazy: value already present");

            this->opt.emplace(std::forward<Args>(args)...);
            return *this->opt;
        }

        /// check if value is present
        /// @return true if value is present
        [[nodiscard]] bool has_value() const { return this->opt.has_value(); }

        /// get reference to value
        /// @return reference to value
        /// @throws std::logic_error if no value present
        const T& operator*() const {
            if (!this->opt.has_value())
                throw std::logic_error("lazy: no value present");
            return *this->opt;
        }

        /// get pointer to value
        /// @return pointer to value
        /// @throws std::logic_error if no value present
        const T* operator->() const {
            if (!this->opt.has_value())
                throw std::logic_error("lazy: no value present");
            return &(*this->opt);
        }

        /// get a mutable reference to value
        /// @return mutable reference to value
        /// @throws std::logic_error if no value present
        T& mut() {
            if (!this->opt.has_value())
                throw std::logic_error("lazy: no value present");
            return *this->opt;
        }
    private:
        std::optional<T> opt{};
    };

    /// simplified alternative to std::optional<std::unique_ptr>
    template<typename T>
    class owned_ptr {
    public:
        /// default constructor
        owned_ptr() = default;

        /// construct from raw pointer without deleter
        /// @param ptr raw pointer to own, must be valid for object lifetime
        explicit owned_ptr(T* ptr)
            : ptr(ptr) {}

        /// construct from raw pointer
        /// @param ptr raw pointer to own, must be valid for object lifetime
        /// @param deleter custom deleter function, called only on owned instances
        explicit owned_ptr(T* ptr, std::function<void(T&)> deleter)
            : ptr(ptr), deleter(std::move(deleter)) {}

        /// get reference to owned object
        T& get() const {
            assert(ptr != nullptr && "owned_ptr: no object owned");
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
                if (this->ptr) {
                    if (deleter) deleter(*this->ptr);
                    delete this->ptr;
                }

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
            if (ptr) {
                if (deleter) deleter(*ptr);
                delete ptr;
            }
        }
    private:
        T* ptr{};
        std::function<void(T&)> deleter{};
    };
}
