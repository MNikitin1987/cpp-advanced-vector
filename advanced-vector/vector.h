#pragma once
#include <cassert>
#include <cstdlib>
#include <new>
#include <utility>
#include <memory>
#include <string>
#include <stdexcept>

using namespace std;

template <typename T>
class RawMemory {
public:
    RawMemory() = default;
    RawMemory(const RawMemory&) = delete;
    RawMemory& operator=(const RawMemory& rhs) = delete;

    RawMemory(RawMemory&& other) noexcept {
        capacity_ = std::move(other.capacity_);
        buffer_ = std::move(other.buffer_);
        other.buffer_ = nullptr;
        other.capacity_ = 0;
    }

    RawMemory& operator=(RawMemory&& rhs) noexcept {
        Deallocate(buffer_);
        capacity_ = std::move(rhs.capacity_);
        buffer_ = std::move(rhs.buffer_);
        rhs.buffer_ = nullptr;
        rhs.capacity_ = 0;
        return *this;
    }

    explicit RawMemory(size_t capacity)
        : buffer_(Allocate(capacity))
        , capacity_(capacity) {
    }

    ~RawMemory() {
        Deallocate(buffer_);
    }

    T* operator+(size_t offset) noexcept {
        // Разрешается получать адрес ячейки памяти, следующей за последним элементом массива
        assert(offset <= capacity_);
        return buffer_ + offset;
    }

    const T* operator+(size_t offset) const noexcept {
        return const_cast<RawMemory&>(*this) + offset;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<RawMemory&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < capacity_);
        return buffer_[index];
    }

    void Swap(RawMemory& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

    const T* GetAddress() const noexcept {
        return buffer_;
    }

    T* GetAddress() noexcept {
        return buffer_;
    }

    size_t Capacity() const {
        return capacity_;
    }

private:
    // Выделяет сырую память под n элементов и возвращает указатель на неё
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    // Освобождает сырую память, выделенную ранее по адресу buf при помощи Allocate
    static void Deallocate(T* buf) noexcept {
        operator delete(buf);
    }

    T* buffer_ = nullptr;
    size_t capacity_ = 0;
};








template <typename T>
class Vector {
public:

    using iterator = T*;
    using const_iterator = const T*;

    iterator begin() noexcept {
        return data_.GetAddress();
    }

    iterator end() noexcept {
        return data_.GetAddress() + size_;
    }

    const_iterator begin() const noexcept {
        return data_.GetAddress();
    }

    const_iterator end() const noexcept {
        return data_.GetAddress() + size_;
    }

    const_iterator cbegin() const noexcept {
        return data_.GetAddress();
    }

    const_iterator cend() const noexcept {
        return data_.GetAddress() + size_;
    }


    Vector() = default;

    explicit Vector(size_t size)
        : data_(size)
        , size_(size)  //
    {
        std::uninitialized_value_construct_n(data_.GetAddress(), size);
    }


    Vector(const Vector& other)
        : data_(other.size_)
        , size_(other.size_)  //
    {
        std::uninitialized_copy_n(other.data_.GetAddress(), size_, data_.GetAddress());
    }

    Vector(Vector&& other)
        : data_(std::move(other.data_))
        , size_(std::move(other.size_))
    {
        other.size_ = 0;
    }



    ~Vector() {
        std::destroy_n(data_.GetAddress(), size_);
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity <= Capacity()) {
            return;
        }

        RawMemory<T>  new_data(new_capacity);
        CopyAndSwap(new_data);
    }

    size_t Size() const noexcept {
        return size_;
    }

    size_t Capacity() const noexcept {
        return data_.Capacity();
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }

    Vector& operator=(const Vector& rhs) {
        if (this == &rhs) {
            return *this;
        }

        if (rhs.size_ > Capacity()) {
            Vector rhs_copy(rhs);
            Swap(rhs_copy);
            return *this;
        }

        if (rhs.size_ <= size_) {
            auto it = std::copy_n(rhs.data_.GetAddress(), rhs.size_, data_.GetAddress());
            std::destroy_n(it, size_ - rhs.size_);
        }
        else {
            auto it = std::copy_n(rhs.data_.GetAddress(), size_, data_.GetAddress());
            std::uninitialized_copy_n(
                rhs.data_.GetAddress() + size_, rhs.size_ - size_, it);
        }

        size_ = rhs.size_;

        return *this;
    }


    Vector& operator=(Vector&& rhs) noexcept {
        data_ = std::move(rhs.data_);
        size_ = rhs.size_;
        
        rhs.size_ = 0;
        return *this;
    }


    void Swap(Vector& other) noexcept {
        std::swap(data_, other.data_);
        std::swap(size_, other.size_);
    }


    void Resize(size_t new_size) {
        if (new_size < size_) {
            destroy_n(data_.GetAddress() + new_size, size_ - new_size);
        }
        else {
            Reserve(new_size);
            std::uninitialized_default_construct_n(data_.GetAddress() + size_, new_size - size_);
        }

        size_ = new_size;
    }


    void PushBack(const T& value) {
        if (size_ == Capacity()) {
            RawMemory<T>  new_data(size_ == 0 ? 1 : Capacity() * 2);
            std::uninitialized_copy_n(&value, 1, new_data.GetAddress() + size_);
            CopyAndSwap(new_data);
        }
        else {
            std::uninitialized_copy_n(&value, 1, data_.GetAddress() + size_);
        }
        ++size_;
    }

    void PushBack(T&& value) {
        if (size_ == Capacity()) {
            RawMemory<T>  new_data(size_ == 0 ? 1 : Capacity() * 2);
            std::uninitialized_move_n(&value, 1, new_data.GetAddress() + size_);
            CopyAndSwap(new_data);
        }
        else {
            std::uninitialized_move_n(&value, 1, data_.GetAddress() + size_);
        }

        ++size_;
    }

    void PopBack() /* noexcept */ {
        assert(size_ != 0);
        --size_;
        destroy_n(data_.GetAddress() + size_, 1);
    }

    template <typename... Args>
    T& EmplaceBack(Args&&... args) {
        if (size_ == Capacity()) {
            RawMemory<T>  new_data(size_ == 0 ? 1 : Capacity() * 2);
            new (new_data.GetAddress() + size_) T(std::forward<decltype(args)>(args)...);
            CopyAndSwap(new_data);
        }
        else {
            new (data_.GetAddress() + size_) T(std::forward<decltype(args)>(args)...);
        }
        ++size_;

        return data_[size_ - 1];
    }


    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args) {
        assert(begin() <= pos && pos <= end());

        size_t num = pos - begin();

        if (size_ == Capacity()) {
            RawMemory<T>  new_data(size_ == 0 ? 1 : Capacity() * 2);
            new (new_data.GetAddress() + num) T(std::forward<decltype(args)>(args)...);

            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), num, new_data.GetAddress());
                std::uninitialized_move_n(data_.GetAddress() + num, size_ - num, new_data.GetAddress() + num + 1);
            }
            else {
                std::uninitialized_copy_n(data_.GetAddress(), num, new_data.GetAddress());
                std::uninitialized_copy_n(data_.GetAddress() + num, size_ - num, new_data.GetAddress() + num + 1);
            }

            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(new_data);
        }
        else {
            if (num != size_) {
                std::uninitialized_move_n(data_.GetAddress() + size_ - 1, 1, data_.GetAddress() + size_);
                std::move_backward(data_.GetAddress() + num, end() - 1, end());
                data_[num] = T(std::forward<decltype(args)>(args)...);
            }
            else {
                new (data_.GetAddress() + num) T(std::forward<decltype(args)>(args)...);
            }
        }
        ++size_;

        return data_.GetAddress() + num;
    }


    iterator Insert(const_iterator pos, const T& value) {
        return Emplace(pos, value);
    }

    iterator Insert(const_iterator pos, T&& value) {
        return Emplace(pos, std::move(value));
    }

    iterator Erase(const_iterator pos) {
        assert(begin() <= pos && pos < end());

        size_t num = pos - begin();

        std::move(data_.GetAddress() + num + 1, data_.GetAddress() + size_, data_.GetAddress() + num);
        std::destroy_n(data_.GetAddress() + size_ - 1, 1);
        --size_;

        return data_.GetAddress() + num;
    }


private:

    void CopyAndSwap(RawMemory<T>& new_data) {
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        else {
            std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(new_data);
    }

private:
    RawMemory<T> data_;
    size_t size_ = 0;
};