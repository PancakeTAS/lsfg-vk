/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <new>
#include <stdexcept>
#include <utility>
#include <vector>

namespace lsfgvk::pipeline {

    /// C++26 backported inplace_vector
    template<typename T, size_t N>
    class inplace_vector {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
    public:
        // Constructors
        constexpr inplace_vector() = default;
        constexpr inplace_vector(std::initializer_list<T> init) {
            if (init.size() > N) throw std::bad_alloc();
            for (auto& elem : init)
                this->m_data.at(this->m_size++) = elem;
        }
        constexpr inplace_vector(const std::vector<T>& vec) {
            if (vec.size() > N) throw std::bad_alloc();
            for (const auto& elem : vec)
                this->m_data.at(this->m_size++) = elem;
        }

        // Appending elements
        constexpr void push_back(const T& value) {
            if (this->m_size >= N) throw std::bad_alloc();
            this->m_data.at(this->m_size++) = value;
        }
        constexpr void push_back(T&& value) {
            if (this->m_size >= N) throw std::bad_alloc();
            this->m_data.at(this->m_size++) = std::move(value);
        }
        template<typename... Args>
        constexpr T& emplace_back(Args&&... args) {
            if (this->m_size >= N) throw std::bad_alloc();
            this->m_data.at(this->m_size) = T(std::forward<Args>(args)...);
            return this->m_data.at(this->m_size++);
        }
        constexpr void clear() { this->m_size = 0; }

        // Accessing elements
        constexpr T& operator[](size_t idx) { return this->m_data.at(idx); }
        constexpr const T& operator[](size_t idx) const { return this->m_data.at(idx); }
        [[nodiscard]] constexpr T& at(size_t idx) {
            if (idx >= this->m_size) throw std::out_of_range("Index out of range");
            return this->m_data.at(idx);
        }
        [[nodiscard]] constexpr const T& at(size_t idx) const {
            if (idx >= this->m_size) throw std::out_of_range("Index out of range");
            return this->m_data.at(idx);
        }
        [[nodiscard]] constexpr T& front() { return this->m_data.front(); }
        [[nodiscard]] constexpr const T& front() const { return this->m_data.front(); }
        [[nodiscard]] constexpr T& back() { return this->m_data.at(this->m_size - 1); }
        [[nodiscard]] constexpr const T& back() const { return this->m_data.at(this->m_size - 1); }

        // Iterating elements
        [[nodiscard]] constexpr T* begin() { return this->m_data.data(); }
        [[nodiscard]] constexpr const T* begin() const { return this->m_data.data(); }
        [[nodiscard]] constexpr const T* cbegin() const { return this->m_data.data(); }
        [[nodiscard]] constexpr T* end() { return this->m_data.data() + this->m_size; } // NOLINT (pointer arithmetic)
        [[nodiscard]] constexpr const T* end() const { return this->m_data.data() + this->m_size; } // NOLINT (pointer arithmetic)
        [[nodiscard]] constexpr const T* cend() const { return this->m_data.data() + this->m_size; } // NOLINT (pointer arithmetic)

        // Removing elements
        constexpr void pop_back() {
            if (this->m_size == 0) throw std::out_of_range("Vector is empty");
            this->m_size--;
        }

        // Query capacity
        [[nodiscard]] constexpr size_t size() const { return this->m_size; }
        [[nodiscard]] constexpr size_t capacity() const { return N; }
        [[nodiscard]] constexpr bool empty() const { return this->m_size == 0; }
    private:
        std::array<T, N> m_data{};
        size_t m_size{0};
#pragma clang diagnostic pop
    };

    /// Sequence of operations to apply to the base extent
    class ExtentOp {
    public:
        /// Default constructor for no operations and no flow scaling
        constexpr ExtentOp() = default;
        /// Constructor for no operations aside from flow scale
        constexpr ExtentOp(bool flow) : m_flow(flow) {}
        /// Constructor for a single operation
        constexpr ExtentOp(bool flow, uint32_t add, uint32_t shift)
            : m_flow(flow), m_operations({{add, shift}}) {}
        /// Constructor for a single operation starting from the flow base extent
        constexpr ExtentOp(uint32_t add, uint32_t shift)
            : m_flow(true), m_operations({{add, shift}}) {}
        // Combine two extents
        constexpr ExtentOp operator+(const ExtentOp& other) const {
            ExtentOp result{*this};
            for (const auto& [add, shift] : other.m_operations)
                result.m_operations.emplace_back(add, shift);
            return result;
        }
        // Combine two extends
        constexpr ExtentOp operator+=(const ExtentOp& other) {
            for (const auto& [add, shift] : other.m_operations)
                this->m_operations.emplace_back(add, shift);
            return *this;
        }
        /// Get the flow value
        [[nodiscard]] constexpr auto flow() const { return this->m_flow; }
        /// Get the operations
        [[nodiscard]] constexpr const auto& operations() const { return this->m_operations; }
    private:
        bool m_flow{false};
        inplace_vector<std::pair<uint32_t, uint32_t>, 8> m_operations;
    };

}
