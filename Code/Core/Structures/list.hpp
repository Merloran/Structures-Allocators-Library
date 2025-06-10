#pragma once

template <Manual Type>
class List
{
public:
    struct Node
    {
        Node *next;
        Node *previous;
        Type  data;

        Node()
            : next(nullptr)
            , previous(nullptr)
            , data(Type())
        {}
    };

    class Iterator
    {
    public:
        Node *currentElement;

        Iterator() noexcept
            : currentElement(nullptr)
        {}

        Type &operator*() noexcept
        {
            return currentElement->data;
        }

        Type *operator->() noexcept
        {
            return &currentElement->data;
        }

        Void operator++() noexcept
        {
            currentElement = currentElement->next;
        }

        Void operator--() noexcept
        {
            currentElement = currentElement->previous;
        }

        Bool operator==(const Iterator &other) const noexcept
        {
            return currentElement == other.currentElement;
        }

        Bool operator!=(const Iterator &other) const noexcept
        {
            return currentElement != other.currentElement;
        }
    };

private:
    AllocatorInfo *allocatorInfo;
    Node          *sentinel;
    USize          size;

public:
    List() noexcept
        : allocatorInfo(AllocatorInfo::get_default_allocator())
        , sentinel(nullptr)
        , size(0)
    {}

    Void initialize(AllocatorInfo *allocator = AllocatorInfo::get_default_allocator()) noexcept
    {
        assert(allocatorInfo && "Invalid pointer!");
        allocatorInfo = allocator;

        sentinel = Memory::allocate<Node>(allocatorInfo);

        sentinel->next     = sentinel;
        sentinel->previous = sentinel;
    }

    Void initialize(const Type &initialElement,
                    AllocatorInfo *allocator = AllocatorInfo::get_default_allocator()) noexcept
    {
        assert(allocatorInfo && "Invalid pointer!");
        allocatorInfo = allocator;

        sentinel = Memory::allocate<Node>(allocatorInfo);

        sentinel->next     = sentinel;
        sentinel->previous = sentinel;
    }

    Type &push_back(const Type &element) noexcept
    {
        Node *target = Memory::allocate<Node>(allocatorInfo);
        if constexpr (Copyable<Type>)
        {
            target->data.copy(element);
        } else {
            target->data = element;
        }

        target->previous = sentinel->previous;
        target->next     = sentinel;

        sentinel->previous->next = target;
        sentinel->previous       = target;

        ++size;

        return target->data;
    }

    Type &push_front(const Type &element) noexcept
    {
        Node *target = Memory::allocate<Node>(allocatorInfo);
        if constexpr (Copyable<Type>)
        {
            target->data.copy(element);
        } else {
            target->data = element;
        }
        
        target->next     = sentinel->next;
        target->previous = sentinel;

        sentinel->next->previous = target;
        sentinel->next           = target;

        ++size;

        return target->data;
    }

    Type &emplace_back(Type &element) noexcept
    {
        Node *target = Memory::allocate<Node>(allocatorInfo);
        if constexpr (Moveable<Type>)
        {
            target->data.move(element);
        }
        else if constexpr (Copyable<Type>)
        {
            target->data.copy(element);
        } else {
            target->data = element;
        }

        target->previous = sentinel->previous;
        target->next     = sentinel;

        sentinel->previous->next = target;
        sentinel->previous       = target;

        ++size;

        return target->data;
    }

    Type &emplace_front(Type &element) noexcept
    {
        Node *target = Memory::allocate<Node>(allocatorInfo);
        if constexpr (Moveable<Type>)
        {
            target->data.move(element);
        }
        else if constexpr (Copyable<Type>)
        {
            target->data.copy(element);
        } else {
            target->data = element;
        }
        
        target->next     = sentinel->next;
        target->previous = sentinel;

        sentinel->next->previous = target;
        sentinel->next           = target;

        ++size;

        return target->data;
    }

    Void pop_back() noexcept
    {
        
    }

    Iterator begin() noexcept
    {
        return Iterator{ sentinel->next };
    }

    Iterator end() noexcept
    {
        return Iterator{ sentinel };
    }

    Void move(List &source) noexcept
    {
        if (&source == this)
        {
            return;
        }

        finalize();
        allocatorInfo = source.allocatorInfo;
        sentinel      = source.sentinel;

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

        for (const Type &element : source)
        {
            push_front(element);
        }
    }

    [[nodiscard]]
    Bool contains(const Type &value) const noexcept
    {
        for (const Type &element : *this)
        {
            if (element == value)
            {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]]
    Bool is_empty() const noexcept
    {
        return size == 0;
    }

    [[nodiscard]]
    const Type &operator[](const USize index) const noexcept
    {
        assert(index < size);
        if (index < size / 2)
        {
            Iterator iterator = begin();
            for (USize i = 0; i < index; ++i)
            {
                ++iterator;
            }
            return *iterator;
        }

        const USize reversedIndex = size - 1 - index;
        Iterator iterator = end();
        for (USize i = 0; i < reversedIndex; ++i) 
        {
            --iterator;
        }
        return *iterator;
    }

    [[nodiscard]]
    Iterator begin() const noexcept
    {
        return Iterator{ sentinel->next };
    }

    [[nodiscard]]
    Iterator end() const noexcept
    {
        return Iterator{ sentinel };
    }

    Void clear() noexcept
    {
        if constexpr (Finalizable<Type>)
        {
            for (Iterator iterator = begin(); iterator != end();)
            {
                iterator->finalize();
                Node *toRemove = iterator.currentElement;
                ++iterator;
                allocatorInfo->deallocate(allocatorInfo->allocator, byte_cast(toRemove));
            }
        } else {
            for (Iterator iterator = begin(); iterator != end();)
            {
                Node *toRemove = iterator.currentElement;
                ++iterator;
                allocatorInfo->deallocate(allocatorInfo->allocator, byte_cast(toRemove));
            }
        }
        sentinel->previous = sentinel;
        sentinel->next     = sentinel;
        size = 0;
    }

    Void finalize() noexcept
    {
        assert(allocatorInfo);
        if (!sentinel)
        {
            *this = {};
            return;
        }

        clear();
        allocatorInfo->deallocate(allocatorInfo->allocator, byte_cast(sentinel));

        *this = {};
    }
};