#pragma once

template <typename Type, UInt64 size>
class Array
{
private:
    Type elements[size];

public:
    constexpr Array() noexcept
    : elements{}
    {}

    constexpr Array(const Type &value) noexcept
    {
        fill(value);
    }
    
    template <typename... Args,
              std::enable_if_t<(sizeof...(Args) == size) && 
                               (std::is_same_v<std::decay_t<Args>, Type> && 
                                ...), Int32> = 0>
    constexpr Array(const Args&... values) noexcept
    : elements{ static_cast<Type>(values)... }
    {}

    [[nodiscard]]
    constexpr UInt64 get_size() const noexcept
    {
        return size;
    }

    Type &operator[](UInt64 index) noexcept
    {
        assert(index < size);
        return elements[index];
    }

    Type &get_first() noexcept
    {
        return elements[0];
    }

    Type &get_last() noexcept
    {
        return elements[size - 1];
    }

    Type *get_data() noexcept
    {
        return elements;
    }

    Type *begin() noexcept
    {
        return elements;
    }

    Type *end() noexcept
    {
        return elements + size;
    }

    constexpr Void copy(const Array &source) noexcept
    {
        memcpy(elements, source.elements, size * sizeof(Type));
    }

    constexpr Void fill(const Type &value) noexcept
    {
        Type *data = elements;
        for (UInt64 i = size; i > 0; --i, ++data)
        {
            *data = value;
        }
    }

    Void swap(UInt64 left, UInt64 right) noexcept
    {
        assert(left < size && right < size);
        const Type &temporary = elements[left];
        elements[left] = elements[right];
        elements[right] = temporary;
    }

    [[nodiscard]]
    constexpr Bool contains(const Type &value) const noexcept
    {
        for (UInt64 i = 0; i < size; ++i)
        {
            if (elements[i] == value)
            {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]]
    constexpr Type &operator[](UInt64 index) const noexcept
    {
        assert(index < size);
        return elements[index];
    }

    [[nodiscard]]
    constexpr Type &get_first() const noexcept
    {
        return elements[0];
    }

    [[nodiscard]]
    constexpr Type &get_last() const noexcept
    {
        return elements[size - 1];
    }

    [[nodiscard]]
    constexpr Type *get_data() const noexcept
    {
        return elements;
    }

    [[nodiscard]]
    constexpr Type *begin() const noexcept
    {
        return elements;
    }

    [[nodiscard]]
    constexpr Type *end() const noexcept
    {
        return elements + size;
    }
};