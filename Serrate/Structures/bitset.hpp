#pragma once
#include "Serrate/Utilities/types.hpp"

#include <cassert>

template<USize BitCount>
class BitSet
{
public:
    class Iterator
    {
    private:
        USize *data;
        USize index;

    public:
        Iterator() noexcept
        : data(nullptr)
        , index(0)
        {}

        Iterator(USize *data, const USize index) noexcept
        : data(data)
        , index(index)
        {}

        [[nodiscard]]
        USize *get_data() noexcept
        {
            return data;
        }

        [[nodiscard]]
        const USize *get_data() const noexcept
        {
            return data;
        }

        [[nodiscard]]
        USize get_index() const noexcept
        {
            return index;
        }

        [[nodiscard]]
        constexpr operator bool() const noexcept
        {
            const USize block = data[index / BITS_PER_BLOCK];
            const USize mask = USize(1) << (index % BITS_PER_BLOCK);
            return (block & mask) != 0;
        }

        Iterator &operator=(const Bool value) noexcept
        {
            USize &block = data[index / BITS_PER_BLOCK];
            const USize mask = USize(1) << (index % BITS_PER_BLOCK);

            if (value)
            {
                block |= mask;
            } else {
                block &= ~mask;
            }

            return *this;
        }

        Iterator &flip() noexcept
        {
            USize &block = data[index / BITS_PER_BLOCK];
            const USize mask = USize(1) << (index % BITS_PER_BLOCK);
            block ^= mask;
            return *this;
        }

        Iterator &operator*() noexcept
        {
            return *this;
        }

        const Iterator &operator*() const noexcept
        {
            return *this;
        }

        Iterator &operator++() noexcept
        {
            ++index;
            return *this;
        }

        Iterator &operator--() noexcept
        {
            --index;
            return *this;
        }

        Bool operator==(const Iterator &other) const noexcept
        {
            return data == other.data && index == other.index;
        }

        Bool operator!=(const Iterator &other) const noexcept
        {
            return !(*this == other);
        }
    };

private:
    static constexpr USize BITS_PER_BLOCK = sizeof(USize) * USize(8);
    static constexpr USize BLOCK_COUNT = 1 + (BitCount - 1) / BITS_PER_BLOCK;
    USize elements[BLOCK_COUNT];

public:
    constexpr BitSet() noexcept
    {
        if (std::is_constant_evaluated())
        {
            for (USize i = 0; i < BLOCK_COUNT; ++i)
            {
                elements[i] = USize(0);
            }
        } else {
            memset(elements, 0, sizeof(elements));
        }
    }

    constexpr BitSet& reset() noexcept
    {
        for (USize& e : elements)
        {
            e = 0;
        }
        return *this;
    }

    constexpr BitSet &set() noexcept
    {
        for (USize &e : elements)
        {
            e = ~USize(0);
        }
        clear_last_block();
        return *this;
    }

    constexpr BitSet &set(const USize index, const Bool value) noexcept
    {
        if (value)
        {
            elements[index / BITS_PER_BLOCK] |= USize(1) << (index % BITS_PER_BLOCK);
        } else {
            elements[index / BITS_PER_BLOCK] &= ~(USize(1) << (index % BITS_PER_BLOCK));
        }
        return *this;
    }

    constexpr BitSet &flip() noexcept
    {
        for (USize &e : elements)
        {
            e = ~e;
        }
        clear_last_block();
        return *this;
    }

    constexpr BitSet &flip(const USize index) noexcept
    {
        elements[index / BITS_PER_BLOCK] ^= (USize(1) << (index % BITS_PER_BLOCK));
        return *this;
    }

    [[nodiscard]]
    constexpr USize count() const noexcept
    {
        USize total = 0;
        for (USize e : elements)
        {
            while (e) 
            {
                total += e & 1;
                e >>= 1;
            }
        }
        return total;
    }

    [[nodiscard]]
    constexpr Bool any() const noexcept
    {
        for (const USize e : elements)
        {
            if (e)
            {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]]
    constexpr Bool none() const noexcept
    {
        return !any();
    }

    [[nodiscard]]
    constexpr Bool all() const noexcept
    {
        USize fullMask = ~USize(0);
        for (USize i = 0; i < BLOCK_COUNT - 1; ++i) 
        {
            if (elements[i] != fullMask)
            {
                return false;
            }
        }

        constexpr USize USED_BITS_IN_LAST = BitCount % BITS_PER_BLOCK;
        if constexpr (USED_BITS_IN_LAST == 0)
        {
            return elements[BLOCK_COUNT - 1] == fullMask;
        } else {
            USize mask = (USize(1) << USED_BITS_IN_LAST) - 1;
            return (elements[BLOCK_COUNT - 1] & mask) == mask;
        }
    }


    constexpr Iterator begin() noexcept
	{
        return Iterator(elements, 0);
    }

    constexpr Iterator end() noexcept
	{
        return Iterator(elements, BitCount);
    }

    constexpr Iterator begin() const noexcept
	{
        return Iterator(elements, 0);
    }

    constexpr Iterator end() const noexcept
	{
        return Iterator(elements, BitCount);
    }

    constexpr Iterator operator[](USize index) noexcept
    {
        assert(index < BitCount && "Index out of bounds!");
        return Iterator(elements, index);
    }

    constexpr Bool operator[](const USize index) const noexcept
    {
        assert(index < BitCount && "Index out of bounds!");
        const USize block = index / BITS_PER_BLOCK;
        const USize bit = index % BITS_PER_BLOCK;
        return (elements[block] & (USize(1) << bit)) != 0;
    }

    constexpr Bool operator==(const BitSet &other) const noexcept
    {
        if (std::is_constant_evaluated())
        {
            for (USize i = 0; i < BLOCK_COUNT; ++i)
            {
                if (elements[i] != other.elements[i])
                {
                    return false;
                }
            }
            return true;
        } else {
            return memcmp(elements, other.elements, BLOCK_COUNT * sizeof(USize)) == 0;
        }
    }

    constexpr Bool operator!=(const BitSet &other) const noexcept
    {
        if (std::is_constant_evaluated())
        {
            for (USize i = 0; i < BLOCK_COUNT; ++i)
            {
                if (elements[i] != other.elements[i])
                {
                    return true;
                }
            }
            return false;
        } else {
            return memcmp(elements, other.elements, BLOCK_COUNT * sizeof(USize)) != 0;
        }
    }

    constexpr BitSet operator~() const noexcept
    {
        BitSet result;
        for (USize i = 0; i < BLOCK_COUNT; ++i)
        {
            result.elements[i] = ~elements[i];
        }
        result.clear_last_block();
        return result;
    }

    constexpr BitSet &operator&=(const BitSet &other) noexcept
    {
        for (USize i = 0; i < BLOCK_COUNT; ++i)
        {
            elements[i] &= other.elements[i];
        }
        return *this;
    }

    constexpr BitSet &operator|=(const BitSet &other) noexcept
    {
        for (USize i = 0; i < BLOCK_COUNT; ++i)
        {
            elements[i] |= other.elements[i];
        }
        return *this;
    }

    constexpr BitSet &operator^=(const BitSet &other) noexcept
    {
        for (USize i = 0; i < BLOCK_COUNT; ++i)
        {
            elements[i] ^= other.elements[i];
        }
        return *this;
    }

    constexpr BitSet operator&(const BitSet &other) const noexcept
    {
        BitSet result = *this;
        result &= other;
        return result;
    }

    constexpr BitSet operator|(const BitSet &other) const noexcept
    {
        BitSet result = *this;
        result |= other;
        return result;
    }

    constexpr BitSet operator^(const BitSet &other) const noexcept
    {
        BitSet result = *this;
        result ^= other;
        return result;
    }

private:
    constexpr Void clear_last_block() noexcept
    {
        constexpr USize USED_BITS_IN_LAST = BitCount % BITS_PER_BLOCK;
        if constexpr (USED_BITS_IN_LAST > 0)
        {
            const USize mask = (USize(1) << USED_BITS_IN_LAST) - 1;
            elements[BLOCK_COUNT - 1] &= mask;
        }
    }
};