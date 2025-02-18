#pragma once

template <typename Type, UInt64 count>
class Array
{
private:
    Type elements[count];

public:
    constexpr Array() noexcept
    : elements{}
    {}

    constexpr Array(const Type &value) noexcept
    {
        fill(value);
    }
    
    template <typename... Args,
              std::enable_if_t<(sizeof...(Args) == count) && 
                               (std::is_same_v<std::decay_t<Args>, Type> && 
                                ...), Int32> = 0>
    constexpr Array(const Args&... values) noexcept
    : elements{ static_cast<Type>(values)... }
    {}

    [[nodiscard]]
    constexpr UInt64 get_size() const noexcept
    {
        return count;
    }

    Type &operator[](UInt64 index) noexcept
    {
        assert(index < count);
        return elements[index];
    }

    Type &get_first() noexcept
    {
        return elements[0];
    }

    Type &get_last() noexcept
    {
        return elements[count - 1];
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
        return elements + count;
    }

    constexpr Void copy(const Array &source) noexcept
    {
        memcpy(elements, source.elements, count * sizeof(Type));
    }

    constexpr Void fill(const Type &value) noexcept
    {
        Type *data = elements;
        for (UInt64 i = count; i > 0; --i, ++data)
        {
            *data = value;
        }
    }

    Void swap(UInt64 left, UInt64 right) noexcept
    {
        assert(left < count && right < count);
        Type temporary = elements[left];
        elements[left] = elements[right];
        elements[right] = temporary;
    }

    [[nodiscard]]
    constexpr Bool contains(const Type &value) const noexcept
    {
        for (UInt64 i = 0; i < count; ++i)
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
        assert(index < count);
        return elements[index];
    }

    [[nodiscard]]
    const Type &get_first() const noexcept
    {
        return elements[0];
    }

    [[nodiscard]]
    const Type &get_last() const noexcept
    {
        return elements[count - 1];
    }

    [[nodiscard]]
    const Type *get_data() const noexcept
    {
        return elements;
    }

    [[nodiscard]]
    const Type *begin() const noexcept
    {
        return elements;
    }

    [[nodiscard]]
    const Type *end() const noexcept
    {
        return elements + count;
    }
};