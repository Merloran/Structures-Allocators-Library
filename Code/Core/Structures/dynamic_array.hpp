#pragma once
#include "Memory/memory_utils.hpp"

template <Manual Type>
class DynamicArray
{
private:
    static constexpr USize EXPANSION_SIZE = 32;
    AllocatorInfo *allocatorInfo;
    Type *elements;
    USize capacity, size;

public:
    DynamicArray() noexcept
    : allocatorInfo(AllocatorInfo::get_default_allocator())
    , elements(nullptr)
    , capacity(0)
    , size(0)
    {}

    Void initialize(AllocatorInfo *allocator = AllocatorInfo::get_default_allocator()) noexcept
    {
        assert(allocator && "Allocator is nullptr!");
        allocatorInfo = allocator;
        elements = nullptr;
        capacity = 0;
        size = 0;
    }

    Void initialize(const USize initialCapacity, 
                    AllocatorInfo *allocator = AllocatorInfo::get_default_allocator()) noexcept
    {
        assert(allocator && "Allocator is nullptr!");
        assert(initialCapacity > 0 && "Initial capacity should be bigger than 0!");

        allocatorInfo = allocator;

        capacity = initialCapacity;
        size = 0;
        elements = Memory::allocate<Type>(allocatorInfo, capacity);
    }

    Void initialize(const USize initialSize, 
                    const Type& initialElement, 
                    AllocatorInfo *allocator = AllocatorInfo::get_default_allocator()) noexcept
    {
        assert(allocator && "Allocator is nullptr!");
        assert(initialSize > 0 && "Initial capacity should be bigger than 0!");
        allocatorInfo = allocator;

        capacity = initialSize;
        size = initialSize;
        elements = Memory::allocate<Type>(allocatorInfo, capacity);

        fill(initialElement);
    }

    Void reserve(const USize newCapacity) noexcept
    {
        if (newCapacity <= capacity)
        {
            return;
        }

        Type *newElements = Memory::allocate<Type>(allocatorInfo, newCapacity);
        if constexpr (Moveable<Type>)
        {
            Type *data = newElements;
            const Type *sourceData = elements;
            const Type *sourceDataEnd = elements + size;
            for (; sourceData < sourceDataEnd; ++data, ++sourceData)
            {
                data->move(*sourceData);
            }
        }
        else if constexpr (Copyable<Type>)
        {
            Type *data = newElements;
            const Type *sourceData = elements;
            const Type *sourceDataEnd = elements + size;
            for (; sourceData < sourceDataEnd; ++data, ++sourceData)
            {
                data->copy(*sourceData);
            }
        } else {
            memcpy(newElements, elements, size * sizeof(Type));
        }

        if (elements)
        {
            if constexpr (Finalizable<Type>)
            {
                const Type *dataEnd = elements + size;
                for (Type *data = elements; data < dataEnd; ++data)
                {
                    data->finalize();
                }
            }
            Memory::deallocate(allocatorInfo, elements);
        }
        elements = newElements;
        capacity = newCapacity;
    }

    Void resize(const USize newSize, const Type &initialElement = {}) noexcept
    {
        if (newSize == size)
        {
            return;
        }

        if (newSize > capacity)
        {
            Type *newElements = Memory::allocate<Type>(allocatorInfo, newSize);
            if constexpr (Moveable<Type>)
            {
                Type *data = newElements;
                const Type *sourceData = elements;
                const Type *sourceDataEnd = elements + size;
                for (; sourceData < sourceDataEnd; ++data, ++sourceData)
                {
                    data->move(*sourceData);
                }
            }
            else if constexpr (Copyable<Type>)
            {
                Type *data = newElements;
                const Type *sourceData = elements;
                const Type *sourceDataEnd = elements + size;
                for (; sourceData < sourceDataEnd; ++data, ++sourceData)
                {
                    data->copy(*sourceData);
                }
            } else {
                memcpy(newElements, elements, size * sizeof(Type));
            }

            if (elements)
            {
                if constexpr (Finalizable<Type>)
                {
                    const Type *dataEnd = elements + size;
                    for (Type *data = elements; data < dataEnd; ++data)
                    {
                        data->finalize();
                    }
                }
                Memory::deallocate(allocatorInfo, elements);
            }
            const USize oldSize = size;
            size = newSize;
            capacity = newSize;
            fill(oldSize, size, initialElement);
        }
        else if (newSize > size)
        {
            const USize oldSize = size;
            size = newSize;
            fill(oldSize, size, initialElement);
        } else { // newSize < size
            size = newSize;
        }
    }

    Type &append(const Type &element) noexcept
    {
        if (capacity == size)
        {
            SPDLOG_WARN("Reallocation during append, try reserve more memory: {}", capacity + EXPANSION_SIZE);
            reserve(capacity + EXPANSION_SIZE);
        }

        Type &target = elements[size];
        ++size;

        if constexpr (Copyable<Type>) 
        {
            target.copy(element);
        } else {
            target = element;
        }

        return target;
    }

    Type &emplace(Type &element) noexcept
    {
        if (capacity == size)
        {
            SPDLOG_WARN("Reallocation during emplace, try reserve more memory: {}", capacity + EXPANSION_SIZE);
            reserve(capacity + EXPANSION_SIZE);
        }

        Type &target = elements[size];
        ++size;

        if constexpr (Moveable<Type>)
        {
            target.move(element);
        }
        else if (Copyable<Type>)
        {
            target.copy(element);
        } else {
            target = element;
        }

        return target;
    }

    [[nodiscard]]
    USize get_capacity() const noexcept
    {
        return capacity;
    }

    [[nodiscard]]
    USize get_size() const noexcept
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
        assert(&source != this && "Tried to move dynamic array into itself!");

        finalize();
        elements      = source.elements;
        capacity      = source.capacity;
        size          = source.size;
        allocatorInfo = source.allocatorInfo;

        source = {};
    }

    Void copy(const DynamicArray &source) const noexcept
    {
        assert(&source != this && "Tried to copy dynamic array into itself!");

        finalize();
        initialize(source.capacity, source.allocatorInfo);

        size = source.size;

        if constexpr (Copyable<Type>)
        {
            Type *data = elements;
            const Type *sourceData = source.elements;
            const Type *sourceDataEnd = elements + size;
            for (; sourceData < sourceDataEnd; ++data, ++sourceData)
            {
                data->copy(*sourceData);
            }
        } else {
            memcpy(elements, source.elements, size * sizeof(Type));
        }
    }

    Void fill(const Type &value) noexcept
    {
        if constexpr (Copyable<Type>)
        {
            Type *data = elements;
            const Type *dataEnd = elements + size;
            for (; data < dataEnd; ++data)
            {
                data->copy(value);
            }
        } else {
            Type *data = elements;
            const Type *dataEnd = elements + size;
            for (; data < dataEnd; ++data)
            {
                data = value;
            }
        }
    }

    Void fill(USize begin, const USize end, const Type &value) noexcept
    {
        assert(begin < size && end <= size);
        if constexpr (Copyable<Type>)
        {
            Type *data = elements + begin;
            const Type *dataEnd = elements + end;
            for (; data < dataEnd; ++data)
            {
                data->copy(value);
            }
        } else {
            Type *data = elements + begin;
            const Type *dataEnd = elements + end;
            for (; data < dataEnd; ++data)
            {
                data = value;
            }
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
    Bool contains(const Type &value) const noexcept
    {
        if constexpr (std::is_trivial_v<Type> && sizeof(Type) == 1)
        {
            return memchr(elements, value, size) != nullptr;
        } else {
            for (USize i = 0; i < size; ++i)
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
    Bool is_empty() const noexcept
    {
        return size == 0;
    }

    [[nodiscard]]
    const Type &operator[](USize index) const noexcept
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

    Void clear() noexcept
    {
        if constexpr (Finalizable<Type>)
        {
            const Type *dataEnd = elements + size;
            for (Type *data = elements; data < dataEnd; ++data)
            {
                data->finalize();
            }
        }

        size = 0;
    }

    Void finalize() noexcept
    {
        assert(allocatorInfo && "Allocator is nullptr!");
        if (!elements)
        {
            *this = {};
            return;
        }

        clear();

        Memory::deallocate(allocatorInfo, elements);

        *this = {};
    }
};