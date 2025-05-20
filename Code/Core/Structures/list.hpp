#pragma once

template <typename Type>
class List
{
public:
    struct Node
    {
        Node *next;
        Node *previous;
        Type  data;
    };

    class Iterator
    {
        Node *lastElement;
        Node *currentElement;

        Type &operator*()
        {
            return currentElement->data;
        }

        Void operator++()
        {
            assert(currentElement != lastElement);
            currentElement = currentElement->next;
        }

        Void operator--()
        {
            assert(currentElement != lastElement);
            currentElement = currentElement->previous;
        }

        Bool operator==(const Iterator &other)
        {
            return currentElement == other.currentElement;
        }
    };

private:
    Node          lastElement;
    AllocatorInfo *allocatorInfo;

public:
    List() noexcept
        : lastElement(nullptr, nullptr, {})
        , allocatorInfo(AllocatorInfo::get_default_allocator())
    {}

    Void initialize(AllocatorInfo *allocator = AllocatorInfo::get_default_allocator()) noexcept
    {
        allocatorInfo = allocator;
    }

    Void initialize(const Type &initialElement,
                    AllocatorInfo *allocator = AllocatorInfo::get_default_allocator()) noexcept
    {
        allocatorInfo = allocator;

        if (!allocatorInfo)
        {
            SPDLOG_WARN("Allocator is nullptr!");
            return;
        }

        Node* firstElement = static_cast<Node *>(allocatorInfo->allocate(allocatorInfo->allocator, sizeof(Node)));
        if constexpr (requires(Type obj) { obj.copy(std::declval<Type>()); })
        {
            firstElement->data.copy(initialElement);
        } else {
            *firstElement->data = initialElement;
        }
        lastElement.next       = firstElement;
        lastElement.previous   = firstElement;

        firstElement->next     = &lastElement;
        firstElement->previous = &lastElement;
    }

    Type &push_back(const Type &element) noexcept
    {
        Node *target = static_cast<Node *>(allocatorInfo->allocate(allocatorInfo->allocator, sizeof(Node)));
        if constexpr (requires(Type obj) { obj.copy(std::declval<Type>()); })
        {
            target->data.copy(element);
        } else {
            *target->data = element;
        }
        
        target->next     = &lastElement;
        target->previous = lastElement.previous;

        lastElement.previous->next = target;
        lastElement.previous       = target;

        return target->data;
    }

    Type &push_front(const Type &element) noexcept
    {
        Node *target = static_cast<Node *>(allocatorInfo->allocate(allocatorInfo->allocator, sizeof(Node)));
        if constexpr (requires(Type obj) { obj.copy(std::declval<Type>()); })
        {
            target->data.copy(element);
        } else {
            *target->data = element;
        }

        target->previous = &lastElement;
        target->next     = lastElement.next;

        lastElement.next->previous = target;
        lastElement.next           = target;

        return target->data;
    }

    Type &emplace_back(Type &element) noexcept
    {
        Node *target = static_cast<Node *>(allocatorInfo->allocate(allocatorInfo->allocator, sizeof(Node)));
        if constexpr (requires(Type obj) { obj.move(std::declval<Type>()); })
        {
            target->data.move(element);
        } else {
            target->data = element;
        }
        
        target->next     = &lastElement;
        target->previous = lastElement.previous;

        lastElement.previous->next = target;
        lastElement.previous       = target;

        return target->data;
    }

    Type &emplace_front(Type &element) noexcept
    {
        Node *target = static_cast<Node *>(allocatorInfo->allocate(allocatorInfo->allocator, sizeof(Node)));
        if constexpr (requires(Type obj) { obj.move(std::declval<Type>()); })
        {
            target->data.move(element);
        } else {
            target->data = element;
        }

        target->previous = &lastElement;
        target->next     = lastElement.next;

        lastElement.next->previous = target;
        lastElement.next           = target;

        return target->data;
    }

    Type *begin() noexcept
    {
    }

    Type *end() noexcept
    {
    }

    Void move(List &source) noexcept
    {
        if (&source == this)
        {
            return;
        }

        finalize();
        allocatorInfo = source.allocatorInfo;
        lastElement   = source.lastElement;

        source = {};
    }

    Void copy(const List &source) noexcept
    {
        if (&source == this)
        {
            return;
        }

        finalize();
        initialize(source.allocatorInfo);


        if constexpr (requires(Type obj) { obj.copy(std::declval<Type>()); })
        {
            for (Node *node = source.lastElement->next; node != source.lastElement; node = node->next)
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
            for (USize i = size; i > 0; --i, ++data, ++sourceData)
            {
                *data = *sourceData;
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
            return std::memchr(elements, value, size) != nullptr;
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
    inline const Type &operator[](USize index) const noexcept
    {
        assert(index < size);
        return elements[index];
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
            for (USize i = size; i > 0; --i, ++data)
            {
                data->finalize();
            }
        } else if constexpr (!std::is_trivially_destructible_v<Type>)
        {
            Type *data = elements;
            for (USize i = size; i > 0; --i, ++data)
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