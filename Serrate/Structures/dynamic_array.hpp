#pragma once
#include "Serrate/Utilities/types.hpp"
#include "Serrate/Memory/memory_utils.hpp"

#include <spdlog/spdlog.h>


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

    template <Manual... Types>
    Void initialize(AllocatorInfo *allocator, const Types&... params) noexcept
    requires (std::is_convertible_v<std::decay_t<Types>, Type> && ...)
    {
        assert(allocator && "Allocator is nullptr!");
        allocatorInfo = allocator;

        size     = sizeof...(Types);
        capacity = size;
        elements = Memory::allocate<Type>(allocatorInfo, capacity);

        USize index = 0;
        ([&] (const Type& value)
        {
            if constexpr (Copyable<Type>)
            {
                elements[index].copy(value);
            } else {
                elements[index] = value;
            }
            ++index;
        }(params), ...);
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
            Type *sourceData = elements;
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


    Type &push_back(const Type &element) noexcept
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

    Type &push(const Type &element, const USize index) noexcept
    {
        assert(index > size && "Element cannot be push after back!");
        if (capacity == size)
        {
            SPDLOG_WARN("Reallocation during append, try reserve more memory: {}", capacity + EXPANSION_SIZE);
            reserve(capacity + EXPANSION_SIZE);
        }

        Type* currentElement = &elements[size];
        Type* previousElement = (currentElement - 1);
        for (USize i = size; i > index; --i, --currentElement, --previousElement)
        {
            if constexpr (Moveable<Type>)
            {
                currentElement->move(*previousElement);
            }
            else if constexpr (Copyable<Type>)
            {
                currentElement->copy(*previousElement);

                if constexpr (Finalizable<Type>) // This is not necessary, but if someone doesn't want to have move, this should help avoid leaks
                {
                    previousElement->finalize();
                }
            } else {
                *currentElement = *previousElement;
            }
        }

        Type& target = elements[index];
        if constexpr (Copyable<Type>)
        {
            target.copy(element);
        } else {
            target = element;
        }

        ++size;

        return target;
    }

    Type &push_front(const Type &element) noexcept
    {
        return push(element, 0);
    }


    Type &emplace_back(Type &element) noexcept
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

    Type &emplace(Type& element, const USize index) noexcept
    {
        assert(index > size && "Element cannot be push after back!");
        if (capacity == size)
        {
            SPDLOG_WARN("Reallocation during append, try reserve more memory: {}", capacity + EXPANSION_SIZE);
            reserve(capacity + EXPANSION_SIZE);
        }

        Type* currentElement = &elements[size];
        Type* previousElement = (currentElement - 1);
        for (USize i = size; i > index; --i, --currentElement, --previousElement)
        {
            if constexpr (Moveable<Type>)
            {
                currentElement->move(*previousElement);
            }
            else if constexpr (Copyable<Type>)
            {
                currentElement->copy(*previousElement);

                if constexpr (Finalizable<Type>) // This is not necessary, but if someone doesn't want to have move, this should help avoid leaks
                {
                    previousElement->finalize();
                }
            } else {
                *currentElement = *previousElement;
            }
        }


        Type& target = elements[index];
        if constexpr (Moveable<Type>)
        {
            target.move(element);
        }
        else if constexpr (Copyable<Type>)
        {
            target.copy(element);
        } else {
            target = element;
        }


        ++size;

        return target;
    }

    Type &emplace_front(Type& element) noexcept
    {
        return emplace(element, 0);
    }


    Void remove_back() noexcept
    {
        if constexpr (Finalizable<Type>)
        {
            elements[size].finalize();
        }
        --size;
    }

    Void remove(const USize index) noexcept
    {
        if constexpr (Finalizable<Type>)
        {
            elements[index].finalize();
        }

        Type* currentElement = &elements[index];
        Type* nextElement = (currentElement + 1);
        for (USize i = index; i < size; ++i, ++currentElement, ++nextElement)
        {
            if constexpr (Moveable<Type>)
            {
                currentElement->move(*nextElement);
            }
            else if constexpr (Copyable<Type>)
            {
                currentElement->copy(*nextElement);

                if constexpr (Finalizable<Type>) // This is not necessary, but if someone doesn't want to have move, this should help avoid leaks
                {
                    nextElement->finalize();
                }
            } else {
                *currentElement = *nextElement;
            }
        }

        --size;
    }

    Void remove_front() noexcept
    {
        remove(0);
    }

    // Remove element and swap with last element
    Void remove_swap(USize index)
    {
        if constexpr (Finalizable<Type>)
        {
            elements[index].finalize();
        }

        if constexpr (Moveable<Type>)
        {
            elements[index].move(elements[size]);
        }
        else if constexpr (Copyable<Type>)
        {
            elements[index].copy(elements[size]);

            if constexpr (Finalizable<Type>)
            {
                elements[size].finalize();
            }
        } else {
            elements[index] = elements[size];
        }

        --size;
    }


    [[nodiscard("Use remove_back")]]
    Type pop_back() noexcept
    {
        Type element; 

        if constexpr (Moveable<Type>)
        {
            element.move(elements[size]);
        }
        else if constexpr (Copyable<Type>)
        {
            element.copy(elements[size]);

            if constexpr (Finalizable<Type>)
            {
                elements[size].finalize();
            }
        } else {
            element = elements[size];
        }
        --size;

        return element;
    }

    [[nodiscard("Use remove")]]
    Type pop(const USize index) noexcept
    {
        Type element;

        if constexpr (Moveable<Type>)
        {
            element.move(elements[index]);
        }
        else if constexpr (Copyable<Type>)
        {
            element.copy(elements[index]);

            if constexpr (Finalizable<Type>)
            {
                elements[index].finalize();
            }
        } else {
            element = elements[index];
        }

        Type* currentElement = &elements[index];
        Type* nextElement = (currentElement + 1);
        for (USize i = index; i < size; ++i, ++currentElement, ++nextElement)
        {
            if constexpr (Moveable<Type>)
            {
                currentElement->move(*nextElement);
            }
            else if constexpr (Copyable<Type>)
            {
                currentElement->copy(*nextElement);

                if constexpr (Finalizable<Type>) // This is not necessary, but if someone doesn't want to have move, this should help avoid leaks
                {
                    nextElement->finalize();
                }
            } else {
                *currentElement = *nextElement;
            }
        }

        --size;

        return element;
    }

    [[nodiscard("Use remove_front")]]
    Type pop_front() noexcept
    {
        return pop(0);
    }

    // Remove element and swap with last element
    [[nodiscard("Use remove_swap")]]
    Type pop_swap(USize index)
    {
        Type element; 

        if constexpr (Moveable<Type>)
        {
            element.move(elements[index]);
        }
        else if constexpr (Copyable<Type>)
        {
            element.copy(elements[index]);

            if constexpr (Finalizable<Type>)
            {
                elements[index].finalize();
            }
        } else {
            element = elements[index];
        }

        if constexpr (Moveable<Type>)
        {
            elements[index].move(elements[size]);
        }
        else if constexpr (Copyable<Type>)
        {
            elements[index].copy(elements[size]);

            if constexpr (Finalizable<Type>)
            {
                elements[size].finalize();
            }
        }
        else {
            elements[index] = elements[size];
        }

        --size;

        return element;
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