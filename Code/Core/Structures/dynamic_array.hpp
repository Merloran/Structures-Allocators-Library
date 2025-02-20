#pragma once
#include "Memory/memory_utils.hpp"
//TODO: maybe add support for allocator independent data
template <typename Type>
class DynamicArray
{
private:
    static constexpr UInt64 EXPANSION_SIZE = 32;
    Type *elements;
    AllocatorInfo *allocatorInfo;
    UInt64 capacity, size;

public:
    DynamicArray() noexcept
    : elements(nullptr)
    , allocatorInfo(nullptr)
    , capacity(0)
    , size(0)
    {}

    Void initialize(AllocatorInfo *info)
    {
        allocatorInfo = info;
    }

    Void initialize(const UInt64 initialCapacity, AllocatorInfo *info)
    {
        allocatorInfo = info;

        if (!allocatorInfo)
        {
            SPDLOG_WARN("Allocator is nullptr!");
            return;
        }

        capacity = initialCapacity;
        size = 0;
        elements = static_cast<Type *>(allocatorInfo->allocate(allocatorInfo->allocator, capacity * sizeof(Type)));
    }

    Void initialize(const UInt64 initialSize, const Type& initialElement, AllocatorInfo *info)
    {
        allocatorInfo = info;

        if (!allocatorInfo)
        {
            SPDLOG_WARN("Allocator is nullptr!");
            return;
        }

        capacity = initialSize;
        size = initialSize;
        elements = static_cast<Type*>(allocatorInfo->allocate(allocatorInfo->allocator, capacity * sizeof(Type)));
        fill(initialElement);
    }

    Void reserve(const UInt64 newCapacity)
    {
        if (newCapacity <= capacity)
        {
            return;
        }

        Type *newElements = static_cast<Type *>(allocatorInfo->allocate(allocatorInfo->allocator, newCapacity * sizeof(Type)));
        if constexpr (requires(Type obj) { obj.move(std::declval<Type>()); })
        {
            Type *data = newElements;
            const Type *sourceData = elements;
            for (UInt64 i = size; i > 0; --i, ++data, ++sourceData)
            {
                data->move(*sourceData);
            }
        }
        else if constexpr (requires(Type obj) { obj.copy(std::declval<Type>()); })
        {
            Type *data = newElements;
            const Type *sourceData = elements;
            for (UInt64 i = size; i > 0; --i, ++data, ++sourceData)
            {
                data->copy(*sourceData);
            }
        }
        else if constexpr (std::is_trivially_copyable_v<Type>)
        {
            memcpy(newElements, elements, size * sizeof(Type));
        } else {
            Type *data = newElements;
            const Type *sourceData = elements;
            for (UInt64 i = size; i > 0; --i, ++data, ++sourceData)
            {
                *data = *sourceData;
            }
        }
        if (elements)
        {
            allocatorInfo->deallocate(allocatorInfo->allocator, elements);
        }
        elements = newElements;
        capacity = newCapacity;
    }

    Void resize(const UInt64 newSize, const Type &initialElement = {})
    {
        if (newSize == size)
        {
            return;
        }

        if (newSize > capacity)
        {
            Type *newElements = static_cast<Type *>(allocatorInfo->allocate(allocatorInfo->allocator, newSize * sizeof(Type)));
            if constexpr (requires(Type obj) { obj.move(std::declval<Type>()); })
            {
                Type *data = newElements;
                const Type *sourceData = elements;
                for (UInt64 i = size; i > 0; --i, ++data, ++sourceData)
                {
                    data->move(*sourceData);
                }
            }
            else if constexpr (requires(Type obj) { obj.copy(std::declval<Type>()); })
            {
                Type *data = newElements;
                const Type *sourceData = elements;
                for (UInt64 i = size; i > 0; --i, ++data, ++sourceData)
                {
                    data->copy(*sourceData);
                }
            }
            else if constexpr (std::is_trivially_copyable_v<Type>)
            {
                memcpy(newElements, elements, size * sizeof(Type));
            } else {
                Type *data = newElements;
                const Type *sourceData = elements;
                for (UInt64 i = size; i > 0; --i, ++data, ++sourceData)
                {
                    *data = *sourceData;
                }
            }
            if (elements)
            {
                allocatorInfo->deallocate(allocatorInfo->allocator, elements);
            }
            const UInt64 oldSize = size;
            size = newSize;
            capacity = newSize;
            fill(oldSize, size, initialElement);
        }
        else if (newSize > size)
        {
            const UInt64 oldSize = size;
            size = newSize;
            fill(oldSize, size, initialElement);
        } else { // newSize < size
            size = newSize;
        }
    }

    Type &append(const Type &element)
    {
        if (capacity == size)
        {
            reserve(max(capacity, EXPANSION_SIZE) * 2);
        }

        Type &target = elements[size];
        ++size;

        if constexpr (requires(Type obj) { obj.copy(std::declval<Type>()); }) 
        {
            target.copy(element);
        } else {
            target = element;
        }

        return target;
    }

    Type &emplace(Type &element)
    {
        if (capacity == size)
        {
            reserve(capacity + EXPANSION_SIZE);
        }

        Type &target = elements[size];
        ++size;

        if constexpr (requires(Type obj) { obj.move(std::declval<Type>()); })
        {
            target.move(element);
        } else {
            target = element;
        }

        return target;
    }

    [[nodiscard]]
    UInt64 get_capacity() const noexcept
    {
        return capacity;
    }

    [[nodiscard]]
    UInt64 get_size() const noexcept
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
        assert(size > 0);
        return elements[0];
    }

    Type &get_last() noexcept
    {
        assert(size > 0);
        return elements[size - 1];
    }

    Type *get_data() noexcept
    {
        assert(size > 0);
        return elements;
    }

    Type *begin() noexcept
    {
        assert(size > 0);
        return elements;
    }

    Type *end() noexcept
    {
        assert(size > 0);
        return elements + size;
    }

    Void move(DynamicArray &source) noexcept
    {
        finalize();
        elements      = source.elements;
        capacity      = source.capacity;
        size          = source.size;
        allocatorInfo = source.allocatorInfo;

        source = {};
    }

    Void copy(const DynamicArray &source) noexcept
    {
        finalize();
        initialize(source.capacity, source.allocatorInfo);

        size = source.size;

        if constexpr (requires(Type obj) { obj.copy(std::declval<Type>()); })
        {
            Type *data = elements;
            const Type *sourceData = source.elements;
            for (UInt64 i = size; i > 0; --i, ++data, ++sourceData)
            {
                data->copy(*sourceData);
            }
        }
        else if constexpr (std::is_trivially_copyable_v<Type>)
        {
            memcpy(elements, source.elements, size * sizeof(Type));
        } else {
            Type *data = elements;
            const Type *sourceData = source.elements;
            for (UInt64 i = size; i > 0; --i, ++data, ++sourceData)
            {
                *data = *sourceData;
            }
        }
    }

    Void fill(const Type &value) noexcept
    {
        Type *data = elements;
        for (UInt64 i = size; i > 0; --i, ++data)
        {   
            *data = value;
        }
    }

    Void fill(UInt64 begin, const UInt64 end, const Type &value) noexcept
    {
        assert(begin < size && end <= size);
        Type *data = elements + begin;
        for (UInt64 i = end; i > begin; --i, ++data)
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
    Bool contains(const Type &value) const noexcept
    {
        if constexpr (std::is_trivial_v<Type> && sizeof(Type) == 1)
        {
            return std::memchr(elements, value, size) != nullptr;
        } else {
            for (UInt64 i = 0; i < size; ++i)
            {
                if (elements[i] == value)
                {
                    return true;
                }
            }
            return false;
        }
    }

    [[nodiscard]]
    inline const Type &operator[](UInt64 index) const noexcept
    {
        assert(index < size);
        return elements[index];
    }

    [[nodiscard]]
    const Type &get_first() const noexcept
    {
        assert(size > 0);
        return elements[0];
    }

    [[nodiscard]]
    const Type &get_last() const noexcept
    {
        assert(size > 0);
        return elements[size - 1];
    }

    [[nodiscard]]
    const Type *get_data() const noexcept
    {
        assert(size > 0);
        return elements;
    }

    [[nodiscard]]
    const Type *begin() const noexcept
    {
        assert(size > 0);
        return elements;
    }

    [[nodiscard]]
    const Type *end() const noexcept
    {
        assert(size > 0);
        return elements + size;
    }

    Void clear()
    {
        if constexpr (requires(Type obj) { obj.finalize(); })
        {
            Type *data = elements;
            for (UInt64 i = size; i > 0; --i, ++data)
            {
                data->finalize();
            }
        }
        else if constexpr (!std::is_trivially_destructible_v<Type>) 
        {
            Type *data = elements;
            for (UInt64 i = size; i > 0; --i, ++data)
            {
                data->~Type();
            }
        }
        size = 0;
    }

    Void finalize()
    {
        if (!elements)
        {
            *this = {};
            return;
        }

        clear();

        if (allocatorInfo)
        {
            allocatorInfo->deallocate(allocatorInfo->allocator, elements);
        }
        *this = {};
    }
};