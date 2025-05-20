#pragma once
#include "Memory/memory_utils.hpp"

template <typename Type>
class DynamicArray
{
private:
    static constexpr USize EXPANSION_SIZE = 32;
    Type *elements;
    AllocatorInfo *allocatorInfo;
    USize capacity, size;

public:
    DynamicArray() noexcept
    : elements(nullptr)
    , allocatorInfo(AllocatorInfo::get_default_allocator())
    , capacity(0)
    , size(0)
    {}

    Void initialize(AllocatorInfo *allocator = AllocatorInfo::get_default_allocator())
    {
        allocatorInfo = allocator;
    }

    Void initialize(const USize initialCapacity, 
                    AllocatorInfo *allocator = AllocatorInfo::get_default_allocator())
    {
        allocatorInfo = allocator;

        assert(allocatorInfo != nullptr && "Allocator is nullptr!");

        capacity = initialCapacity;
        size = 0;
        elements = reinterpret_cast<Type *>(allocatorInfo->allocate(allocatorInfo->allocator, 
                                                                    capacity * sizeof(Type), 
                                                                    alignof(Type)));
        fill(Type());
    }

    Void initialize(const USize initialSize, 
                    const Type& initialElement, 
                    AllocatorInfo *allocator = AllocatorInfo::get_default_allocator())
    {
        allocatorInfo = allocator;

        assert(allocatorInfo != nullptr && "Allocator is nullptr!");

        capacity = initialSize;
        size = initialSize;
        elements = reinterpret_cast<Type *>(allocatorInfo->allocate(allocatorInfo->allocator,
                                                                    capacity * sizeof(Type), 
                                                                    alignof(Type)));
        fill(initialElement);
    }

    Void reserve(const USize newCapacity)
    {
        if (newCapacity <= capacity)
        {
            return;
        }

        Type *newElements = reinterpret_cast<Type *>(allocatorInfo->allocate(allocatorInfo->allocator,
                                                                             newCapacity * sizeof(Type), 
                                                                             alignof(Type)));
        if constexpr (requires(Type obj) { obj.move(std::declval<Type>()); })
        {
            Type *data = newElements;
            const Type *sourceData = elements;
            for (USize i = 0; i < size; ++i, ++data, ++sourceData)
            {
                new (data) Type();
                data->move(*sourceData);
            }
        }
        else if constexpr (requires(Type obj) { obj.copy(std::declval<Type>()); })
        {
            Type *data = newElements;
            const Type *sourceData = elements;
            for (USize i = 0; i < size; ++i, ++data, ++sourceData)
            {
                new (data) Type();
                data->copy(*sourceData);
            }
        }
        else if constexpr (std::is_trivially_copyable_v<Type>)
        {
            Type *data = newElements;
            for (USize i = 0; i < size; ++i, ++data)
            {
                new (data) Type();
            }
            memcpy(newElements, elements, size * sizeof(Type));
        }
        else if constexpr (std::is_copy_constructible_v<Type>)
        {
            Type *data = elements;
            const Type *sourceData = elements;
            for (USize i = 0; i < size; ++i, ++data, ++sourceData)
            {
                new (data) Type(*sourceData);
            }
        }
        else if constexpr (std::is_copy_assignable_v<Type>)
        {
            Type *data = elements;
            const Type *sourceData = elements;
            for (USize i = 0; i < size; ++i, ++data, ++sourceData)
            {
                new (data) Type();
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

    Void resize(const USize newSize, const Type &initialElement = {})
    {
        if (newSize == size)
        {
            return;
        }

        if (newSize > capacity)
        {
            Type *newElements = reinterpret_cast<Type *>(allocatorInfo->allocate(allocatorInfo->allocator,
                                                                                 newSize * sizeof(Type),
                                                                                 alignof(Type)));
            if constexpr (requires(Type obj) { obj.move(std::declval<Type>()); })
            {
                Type *data = newElements;
                const Type *sourceData = elements;
                for (USize i = 0; i < size; ++i, ++data, ++sourceData)
                {
                    new (data) Type();
                    data->move(*sourceData);
                }
            }
            else if constexpr (requires(Type obj) { obj.copy(std::declval<Type>()); })
            {
                Type *data = newElements;
                const Type *sourceData = elements;
                for (USize i = 0; i < size; ++i, ++data, ++sourceData)
                {
                    new (data) Type();
                    data->copy(*sourceData);
                }
            }
            else if constexpr (std::is_trivially_copyable_v<Type>)
            {
                Type *data = newElements;
                for (USize i = 0; i < size; ++i, ++data)
                {
                    new (data) Type();
                }
                memcpy(newElements, elements, size * sizeof(Type));
            }
            else if constexpr (std::is_copy_constructible_v<Type>)
            {
                Type *data = elements;
                const Type *sourceData = elements;
                for (USize i = 0; i < size; ++i, ++data, ++sourceData)
                {
                    new (data) Type(*sourceData);
                }
            }
            else if constexpr (std::is_copy_assignable_v<Type>)
            {
                Type *data = elements;
                const Type *sourceData = elements;
                for (USize i = 0; i < size; ++i, ++data, ++sourceData)
                {
                    new (data) Type();
                    *data = *sourceData;
                }
            }

            if (elements)
            {
                allocatorInfo->deallocate(allocatorInfo->allocator, elements);
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

    Type &append(const Type &element)
    {
        if (capacity == size)
        {
            SPDLOG_WARN("Reallocation during append, try reserve more memory: {}", capacity + EXPANSION_SIZE);
            reserve(capacity + EXPANSION_SIZE);
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
            SPDLOG_WARN("Reallocation during emplace, try reserve more memory: {}", capacity + EXPANSION_SIZE);
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

    Void copy(const DynamicArray &source) noexcept
    {
        assert(&source != this && "Tried to copy dynamic array into itself!");

        finalize();
        initialize(source.capacity, source.allocatorInfo);

        size = source.size;

        if constexpr (requires(Type obj) { obj.copy(std::declval<Type>()); })
        {
            Type *data = elements;
            const Type *sourceData = source.elements;
            for (USize i = 0; i < size; ++i, ++data, ++sourceData)
            {
                new (data) Type();
                data->copy(*sourceData);
            }
        }
        else if constexpr (std::is_trivially_copyable_v<Type>)
        {
            Type *data = elements;
            for (USize i = 0; i < size; ++i, ++data)
            {
                new (data) Type();
            }
            memcpy(elements, source.elements, size * sizeof(Type));
        }
        else if constexpr (std::is_copy_constructible_v<Type>)
        {
            Type *data = elements;
            const Type *sourceData = source.elements;
            for (USize i = 0; i < size; ++i, ++data, ++sourceData)
            {
                new (data) Type(*sourceData);
            }
        }
        else if constexpr (std::is_copy_assignable_v<Type>)
        {
            Type *data = elements;
            const Type *sourceData = source.elements;
            for (USize i = 0; i < size; ++i, ++data, ++sourceData)
            {
                new (data) Type();
                *data = *sourceData;
            }
        }
    }

    Void fill(const Type &value) noexcept
    {
        if constexpr (requires(Type obj) { obj.copy(Type()); })
        {
            Type *data = elements;
            for (USize i = 0; i < size; ++i, ++data)
            {
                new (data) Type();
                data->copy(value);
            }
        }
        else if constexpr (std::is_copy_constructible_v<Type>)
        {
            Type *data = elements;
            for (USize i = 0; i < size; ++i, ++data)
            {
                new (data) Type(value);
            }
        }
        else if constexpr (std::is_copy_assignable_v<Type>)
        {
            Type *data = elements;
            for (USize i = 0; i < size; ++i, ++data)
            {
                new (data) Type();
                *data = value;
            }
        }
    }

    Void fill(USize begin, const USize end, const Type &value) noexcept
    {
        assert(begin < size && end <= size);
        if constexpr (requires(Type obj) { obj.copy(Type()); })
        {
            Type *data = elements + begin;
            for (USize i = begin; i < end; ++i, ++data)
            {
                new (data) Type();
                data->copy(value);
            }
        }
        else if constexpr (std::is_copy_constructible_v<Type>)
        {
            Type *data = elements + begin;
            for (USize i = begin; i < end; ++i, ++data)
            {
                new (data) Type(value);
            }
        }
        else if constexpr (std::is_copy_assignable_v<Type>)
        {
            Type *data = elements + begin;
            for (USize i = begin; i < end; ++i, ++data)
            {
                new (data) Type();
                *data = value;
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

    Void clear()
    {
        if constexpr (requires(Type obj) { obj.finalize(); })
        {
            Type *data = elements;
            for (USize i = 0; i < size; ++i, ++data)
            {
                data->finalize();
            }
        }
        else if constexpr (!std::is_trivially_destructible_v<Type>) 
        {
            Type *data = elements;
            for (USize i = 0; i < size; ++i, ++data)
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