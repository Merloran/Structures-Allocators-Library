#pragma once

template <typename Type, USize size>
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
    
    template <typename... Args>
    requires (sizeof...(Args) == size) && (std::same_as<std::decay_t<Args>, Type> && ...)
    constexpr Array(const Args&... values) noexcept
    : elements{ static_cast<Type>(values)... }
    {}

    [[nodiscard]]
    consteval USize get_size() const noexcept
    {
        return size;
    }

    Type &operator[](USize index) noexcept
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
        for (USize i = size; i > 0; --i, ++data)
        {
            *data = value;
        }
    }

    Void swap(USize left, USize right) noexcept
    {
        assert(left < size && right < size);
        const Type &temporary = elements[left];
        elements[left] = elements[right];
        elements[right] = temporary;
    }

    [[nodiscard]]
    constexpr Bool contains(const Type &value) const noexcept
    {
        for (USize i = 0; i < size; ++i)
        {
            if (elements[i] == value)
            {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]]
    constexpr Type &operator[](USize index) const noexcept
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