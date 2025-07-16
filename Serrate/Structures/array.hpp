#pragma once
#include "Serrate/Utilities/types.hpp"

#include <cassert>
#include <concepts>

template <Manual Type, USize Size>
class Array
{
private:
    Type elements[Size];

public:
    constexpr Array() noexcept
    : elements{}
    {}

    constexpr Array(const Type &value) noexcept
    {
        fill(value);
    }
    
    template <typename... Args>
    requires (sizeof...(Args) == Size) && (std::same_as<std::decay_t<Args>, Type> && ...)
    constexpr Array(const Args&... values) noexcept
    : elements{ static_cast<Type>(values)... }
    {}

    [[nodiscard]]
    static consteval USize get_size() noexcept
    {
        return Size;
    }

    Type &operator[](USize index) noexcept
    {
        assert(index < Size);
        return elements[index];
    }

    Type &get_first() noexcept
    {
        return elements[0];
    }

    Type &get_last() noexcept
    {
        return elements[Size - 1];
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
        return elements + Size;
    }

    constexpr Void copy(const Array &source) noexcept
    {
        memcpy(elements, source.elements, Size * sizeof(Type));
    }

    constexpr Void fill(const Type &value) noexcept
    {
        if constexpr (Copyable<Type>)
        {
            Type *data = elements;
            const Type *dataEnd = elements + Size;
            for (; data < dataEnd; ++data)
            {
                data->copy(value);
            }
        } else {
            Type *data = elements;
            const Type *dataEnd = elements + Size;
            for (; data < dataEnd; ++data)
            {
                *data = value;
            }
        }
    }

    Void swap(USize left, USize right) noexcept
    {
        assert(left < Size && right < Size);
        const Type &temporary = elements[left];
        elements[left] = elements[right];
        elements[right] = temporary;
    }

    [[nodiscard]]
    constexpr Bool contains(const Type &value) const noexcept
    {
        for (USize i = 0; i < Size; ++i)
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
        assert(index < Size);
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
        return elements[Size - 1];
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
        return elements + Size;
    }
};
