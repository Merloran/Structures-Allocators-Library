#pragma once
#include "Memory/memory_utils.hpp"


template <Manual Type>
class List
{
public:
    struct Node
    {
        Node *next;
        Node *previous;
        Type  data;

        Node() noexcept
            : next(nullptr)
            , previous(nullptr)
            , data(Type())
        {}
    };

    class Iterator
    {
    private:
        Node *node;

    public:
        Iterator() noexcept
            : node(nullptr)
        {}

        Iterator(Node *initialNode) noexcept
            : node(initialNode)
        {}

        [[nodiscard]]
        Node *get_node() noexcept
        {
            return node;
        }

        [[nodiscard]]
        const Node* get_node() const noexcept
        {
            return node;
        }

        Type &operator*() noexcept
        {
            return node->data;
        }

        Type *operator->() noexcept
        {
            return &node->data;
        }

        const Type &operator*() const noexcept
        {
            return node->data;
        }

        const Type *operator->() const noexcept
        {
            return &node->data;
        }

        Void operator++() noexcept
        {
            node = node->next;
        }

        Void operator--() noexcept
        {
            node = node->previous;
        }

        Bool operator==(const Iterator &other) const noexcept
        {
            return node == other.node;
        }

        Bool operator!=(const Iterator &other) const noexcept
        {
            return node != other.node;
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
        assert(allocator && "Invalid pointer!");
        allocatorInfo = allocator;

        sentinel = Memory::allocate<Node, false>(allocatorInfo);

        sentinel->next     = sentinel;
        sentinel->previous = sentinel;
        size = 0;
    }

    Void initialize(const Type &initialElement, const USize count,
                    AllocatorInfo *allocator = AllocatorInfo::get_default_allocator()) noexcept
    {
        assert(allocator && "Invalid pointer!");
        assert(count > 0 && "Invalid initialization");
        allocatorInfo = allocator;

        sentinel = Memory::allocate<Node, false>(allocatorInfo);

        Node *current = sentinel;
        for (USize i = 0; i < count; ++i)
        {
            current->next = Memory::allocate<Node, false>(allocatorInfo);
            current->next->previous = current;
            current = current->next;

            if constexpr (Copyable<Type>)
            {
                current->data.copy(initialElement);
            } else {
                current->data = initialElement;
            }
        }

        current->next = sentinel;
        sentinel->previous = current;
        size = count;
    }

    Type &push_back(const Type &element) noexcept
    {
        Node *target = Memory::allocate<Node, false>(allocatorInfo);
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
        Node *target = Memory::allocate<Node, false>(allocatorInfo);
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

    Type &push(const Type &element, const USize frontIndex) noexcept
    {
        assert(frontIndex < size && "Index should fit in list range!");
        Node *target = Memory::allocate<Node, false>(allocatorInfo);
        if constexpr (Copyable<Type>)
        {
            target->data.copy(element);
        } else {
            target->data = element;
        }
        USize currentIndex = 0;

        for (Iterator current = begin(); current != end(); ++current)
        {
            if (currentIndex != frontIndex)
            {
                ++currentIndex;
                continue;
            }
            current.get_node()->previous->next = target;
            target->previous                   = current.get_node()->previous;

            current.get_node()->previous = target;
            target->next                 = current.get_node();
            break;
        }

        ++size;

        return target->data;
    }

    Type &push(const Type &element, Iterator &nextElement) noexcept
    {
        Node *target = Memory::allocate<Node, false>(allocatorInfo);
        if constexpr (Copyable<Type>)
        {
            target->data.copy(element);
        } else {
            target->data = element;
        }
        nextElement.get_node()->previous->next = target;
        target->previous                       = nextElement.get_node()->previous;

        nextElement.get_node()->previous = target;
        target->next                     = nextElement.get_node();

        ++size;

        return target->data;
    }


    Type &emplace_back(Type &element) noexcept
    {
        Node *target = Memory::allocate<Node, false>(allocatorInfo);
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
        Node *target = Memory::allocate<Node, false>(allocatorInfo);
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

    Type &emplace(const Type &element, const USize frontIndex) noexcept
    {
        assert(frontIndex < size && "Index should fit in list range!");
        Node *target = Memory::allocate<Node, false>(allocatorInfo);
        if constexpr (Moveable<Type>)
        {
            target->data.move(element);
        } else {
            target->data = element;
        }
        USize currentIndex = 0;

        for (Iterator current = begin(); current != end(); ++current)
        {
            if (currentIndex != frontIndex)
            {
                ++currentIndex;
                continue;
            }
            current.get_node()->previous->next = target;
            target->previous                   = current.get_node()->previous;

            current.get_node()->previous = target;
            target->next                 = current.get_node();
            break;
        }

        ++size;

        return target->data;
    }

    Type &emplace(const Type &element, Iterator &nextElement) noexcept
    {
        Node *target = Memory::allocate<Node, false>(allocatorInfo);
        if constexpr (Moveable<Type>)
        {
            target->data.move(element);
        } else {
            target->data = element;
        }
        nextElement.get_node()->previous->next = target;
        target->previous                       = nextElement.get_node()->previous;

        nextElement.get_node()->previous = target;
        target->next                     = nextElement.get_node();

        ++size;

        return target->data;
    }


    [[nodiscard]]
    Type pop_back() noexcept
    {
        Node *target = sentinel->previous;

        sentinel->previous = target->previous;
        sentinel->previous->next = sentinel;

        Type element;
        if constexpr (Moveable<Type>)
        {
            element.move(target->data);
        }
        else if constexpr (Copyable<Type>)
        {
            element.copy(target->data);
        } else {
            element = target->data;
        }

        --size;

        if constexpr (Finalizable<Type>)
        {
            target->data.finalize();
        }
        Memory::deallocate(allocatorInfo, target);
        return element;
    }

    [[nodiscard]]
    Type pop_front() noexcept
    {
        Node *target = sentinel->next;

        sentinel->next = target->next;
        sentinel->next->previous = sentinel;

        Type element;
        if constexpr (Moveable<Type>)
        {
            element.move(target->data);
        }
        else if constexpr (Copyable<Type>)
        {
            element.copy(target->data);
        } else {
            element = target->data;
        }

        --size;

        if constexpr (Finalizable<Type>)
        {
            target->data.finalize();
        }
        Memory::deallocate(allocatorInfo, target);
        return element;
    }

    [[nodiscard]]
    Type pop(const USize frontIndex) noexcept
    {
        assert(frontIndex < size && "Index should fit in list range!");
        USize currentIndex = 0;
        Type element;
        for (Iterator current = begin(); current != end(); ++current)
        {
            if (currentIndex != frontIndex)
            {
                ++currentIndex;
                continue;
            }
            current.get_node()->previous->next = current.get_node()->next;
            current.get_node()->next->previous = current.get_node()->previous;
            if constexpr (Moveable<Type>)
            {
                element.move(*current);
            }
            else if constexpr (Copyable<Type>)
            {
                element.copy(*current);
            } else {
                element = *current;
            }

            --size;

            if constexpr (Finalizable<Type>)
            {
                current.get_node()->data.finalize();
            }
            Memory::deallocate(allocatorInfo, current.get_node());
            break;
        }

        return element;
    }

    [[nodiscard]]
    Type pop(const Iterator &toPop) noexcept
    {
        Type element;
        toPop.get_node()->previous->next = toPop.get_node()->next;
        toPop.get_node()->next->previous = toPop.get_node()->previous;
        if constexpr (Moveable<Type>)
        {
            element.move(*toPop);
        } else if constexpr (Copyable<Type>)
        {
            element.copy(*toPop);
        } else {
            element = *toPop;
        }

        --size;

        if constexpr (Finalizable<Type>)
        {
            toPop.get_node()->data.finalize();
        }
        Memory::deallocate(allocatorInfo, toPop.get_node());
        return element;
    }


    USize remove_back() noexcept
    {
        if (size == 0)
        {
            return 0;
        }

        Node *target = sentinel->previous;

        sentinel->previous = target->previous;
        sentinel->previous->next = sentinel;

        --size;

        if constexpr (Finalizable<Type>)
        {
            target->data.finalize();
        }
        Memory::deallocate(allocatorInfo, target);

        return 1;
    }

    USize remove_front() noexcept
    {
        if (size == 0)
        {
            return 0;
        }

        Node *target = sentinel->next;

        sentinel->next = target->next;
        sentinel->next->previous = sentinel;

        --size;

        if constexpr (Finalizable<Type>)
        {
            target->data.finalize();
        }
        Memory::deallocate(allocatorInfo, target);

        return 1;
    }

    USize remove(const USize frontIndex) noexcept
    {
        if (frontIndex >= size)
        {
            return 0;
        }
        USize currentIndex = 0;
        for (Iterator current = begin(); current != end(); ++current)
        {
            if (currentIndex != frontIndex)
            {
                ++currentIndex;
                continue;
            }
            current.get_node()->previous->next = current.get_node()->next;
            current.get_node()->next->previous = current.get_node()->previous;

            --size;

            if constexpr (Finalizable<Type>)
            {
                current.get_node()->data.finalize();
            }
            Memory::deallocate(allocatorInfo, current.get_node());
            break;
        }

        return  1;
    }

    USize remove(const Iterator &toDrop) noexcept
    {
        if (size == 0)
        {
            return 0;
        }
        toDrop.get_node()->previous->next = toDrop.get_node()->next;
        toDrop.get_node()->next->previous = toDrop.get_node()->previous;

        --size;

        if constexpr (Finalizable<Type>)
        {
            toDrop.get_node()->data.finalize();
        }
        Memory::deallocate(allocatorInfo, toDrop.get_node());

        return 1;
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
    USize get_size() const noexcept
    {
        return size;
    }

    [[nodiscard]]
    const Type &operator[](const USize index) const noexcept
    {
        assert(index < size);
        if (index < size / 2) [[likely]]
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
    Iterator begin() noexcept
    {
        return Iterator{ sentinel->next };
    }

    [[nodiscard]]
    Iterator end() noexcept
    {
        return Iterator{ sentinel };
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

    [[nodiscard]]
    Type &front() noexcept
    {
        assert(size > 0 && "List is empty!");
        return sentinel->next->data;
    }

    [[nodiscard]]
    Type &back() noexcept
    {
        assert(size > 0 && "List is empty!");
        return sentinel->previous->data;
    }

    [[nodiscard]]
    const Type &front() const noexcept
    {
        assert(size > 0 && "List is empty!");
        return sentinel->next->data;
    }

    [[nodiscard]]
    const Type &back() const noexcept
    {
        assert(size > 0 && "List is empty!");
        return sentinel->previous->data;
    }

    Void clear() noexcept
    {
        for (Iterator iterator = begin(); iterator != end();)
        {
            if constexpr (Finalizable<Type>)
            {
                iterator->finalize();
            }
            Node *toRemove = iterator.get_node();
            ++iterator;
            Memory::deallocate(allocatorInfo, toRemove);
        }

        sentinel->previous = sentinel;
        sentinel->next     = sentinel;
        size = 0;
    }

    Void finalize() noexcept
    {
        assert(allocatorInfo && "Allocator is nullptr!");
        if (!sentinel)
        {
            *this = {};
            return;
        }

        clear();

        Memory::deallocate(allocatorInfo, sentinel);

        *this = {};
    }
};