#pragma once
#include "Utilities/cryptography.hpp"

template<typename Type>
concept MethodHashable = requires(Type &element)
{
    requires Manual<Type>;
    { element.hash() } -> std::convertible_to<UInt64>;
    { element == element } -> std::convertible_to<Bool>;
};

template<typename Type>
concept FunctionHashable = requires(Type &element)
{
    requires Manual<Type>;
    { Cryptography::hash(element) } -> std::convertible_to<UInt64>;
    { element == element } -> std::convertible_to<Bool>;
};

template<typename Type>
concept Hashable = MethodHashable<Type> || FunctionHashable<Type>;


template <Hashable KeyType, Manual ValueType>
class HashMap
{
public:
    struct Node
    {
        Node *bucketNext;
        Node *elementNext;
        Node *elementPrevious;
        ValueType value;
        KeyType key;

        Node() noexcept
        : bucketNext(nullptr)
        , elementNext(nullptr)
        , elementPrevious(nullptr)
        , value(ValueType())
        , key(KeyType())
        {}
    };

    struct Iterator
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

        ValueType &operator*() noexcept
        {
            return node->value;
        }

        ValueType *operator->() noexcept
        {
            return &node->value;
        }

        const ValueType &operator*() const noexcept
        {
            return node->value;
        }

        const ValueType *operator->() const noexcept
        {
            return &node->value;
        }

        Void operator++() noexcept
        {
            node = node->elementNext;
        }

        Void operator--() noexcept
        {
            node = node->elementPrevious;
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
    AllocatorInfo *bucketsAllocatorInfo;
    AllocatorInfo *nodesAllocatorInfo;
    Node          **buckets;
    Node          *sentinel; // Global list of nodes
    USize         capacity;
    USize         size;
    Float32       maxLoadFactor; // Not less than 0.5f

public:
    HashMap() noexcept
    : bucketsAllocatorInfo(AllocatorInfo::get_default_allocator())
    , nodesAllocatorInfo(AllocatorInfo::get_default_allocator())
    , buckets(nullptr)
    , sentinel(nullptr)
    , capacity(0)
    , size(0)
    , maxLoadFactor(1.0f)
    {}

    Void initialize(AllocatorInfo *nodesAllocator = AllocatorInfo::get_default_allocator(),
                    AllocatorInfo *bucketsAllocator = AllocatorInfo::get_default_allocator()) noexcept
    {
        assert(bucketsAllocator && nodesAllocator && "Allocator is nullptr!");
        bucketsAllocatorInfo = bucketsAllocator;
        nodesAllocatorInfo = nodesAllocator;
        buckets = nullptr;
        capacity = 0;
        size = 0;
        maxLoadFactor = 1.0f;
        sentinel = Memory::allocate<Node, false>(nodesAllocatorInfo);
        sentinel->elementNext = sentinel;
        sentinel->elementPrevious = sentinel;
    }

    Void initialize(const USize initialCapacity, 
                    AllocatorInfo *nodesAllocator = AllocatorInfo::get_default_allocator(),
                    AllocatorInfo *bucketsAllocator = AllocatorInfo::get_default_allocator()) noexcept
    {
        assert(bucketsAllocator && nodesAllocator && "Allocator is nullptr!");
        assert(initialCapacity > 0 && "Initial capacity should be bigger than 0!");
        bucketsAllocatorInfo = bucketsAllocator;
        nodesAllocatorInfo = nodesAllocator;
        if (initialCapacity < USize(32))
        {
            capacity = USize(32);
        } else {
            capacity = Memory::align_binary(initialCapacity);
        }

        size = 0;
        maxLoadFactor = 1.0f;
        buckets = Memory::allocate<Node*>(bucketsAllocatorInfo, capacity);
        sentinel = Memory::allocate<Node, false>(nodesAllocatorInfo);
        sentinel->elementNext = sentinel;
        sentinel->elementPrevious = sentinel;
    }

    Void reserve(const USize newCapacity)
    {
        assert(bucketsAllocatorInfo && nodesAllocatorInfo && "Allocator is nullptr!");
        assert(newCapacity > 0 && "New capacity should be bigger than 0!");
        if (newCapacity < capacity)
        {
            return;
        }

        capacity = Memory::align_binary(newCapacity);
        rehash();
    }

    ValueType &push(const KeyType &key, const ValueType &value) noexcept
    {
        if (size >= capacity * maxLoadFactor || capacity == 0)
        {
            rehash();
        }

        UInt64 hash;
        if constexpr (FunctionHashable<KeyType>)
        {
            hash = Cryptography::hash(key);
        } else {
            hash = key.hash();
        }

        USize index = hash & (capacity - 1);
        Node *current = buckets[index];

        while (current != nullptr) // Check if exists
        {
            if (current->key == key) 
            {
                if constexpr (Copyable<ValueType>) 
                {
                    current->value.copy(value);
                } else {
                    current->value = value;
                }
                return current->value;
            }
            current = current->bucketNext;
        }

        Node *newNode = Memory::allocate<Node>(nodesAllocatorInfo);

        if constexpr (Copyable<KeyType>)
        {
            newNode->key.copy(key);
        } else {
            newNode->key = key;
        }

        if constexpr (Copyable<ValueType>)
        {
            newNode->value.copy(value);
        } else {
            newNode->value = value;
        }

        newNode->bucketNext = buckets[index];
        buckets[index] = newNode;

        newNode->elementNext = sentinel->elementNext;
        newNode->elementPrevious = sentinel;
        sentinel->elementNext->elementPrevious = newNode;
        sentinel->elementNext = newNode;

        ++size;
        return newNode->value;
    }

    ValueType &emplace(KeyType &key, ValueType &value) noexcept
    {
        if (size >= capacity * maxLoadFactor || capacity == 0)
        {
            rehash();
        }

        UInt64 hash;
        if constexpr (FunctionHashable<KeyType>)
        {
            hash = Cryptography::hash(key);
        } else {
            hash = key.hash();
        }

        USize index = hash & (capacity - 1);
        Node *current = buckets[index];

        while (current != nullptr) // Check if exists
        {
            if (current->key == key) 
            {
                if constexpr (Moveable<ValueType>) 
                {
                    current->value.move(value);
                }
                else if constexpr (Copyable<ValueType>) 
                {
                    current->value.copy(value);
                } else {
                    current->value = value;
                }
                return current->value;
            }
            current = current->bucketNext;
        }

        Node *newNode = Memory::allocate<Node>(nodesAllocatorInfo);

        if constexpr (Moveable<KeyType>)
        {
            newNode->key.move(key);
        }
        else if constexpr (Copyable<KeyType>)
        {
            newNode->key.copy(key);
        } else {
            newNode->key = key;
        }

        if constexpr (Moveable<ValueType>)
        {
            newNode->value.move(value);
        } 
        else if constexpr (Copyable<ValueType>)
        {
            newNode->value.copy(value);
        } else {
            newNode->value = value;
        }

        newNode->bucketNext = buckets[index];
        buckets[index] = newNode;

        newNode->elementNext = sentinel->elementNext;
        newNode->elementPrevious = sentinel;
        sentinel->elementNext->elementPrevious = newNode;
        sentinel->elementNext = newNode;

        ++size;
        return newNode->value;
    }

    ValueType &operator[](KeyType &key) noexcept
    {
        if (size >= USize(Float32(capacity) * maxLoadFactor) || capacity == 0)
        {
            rehash();
        }

        UInt64 hash;
        if constexpr (FunctionHashable<KeyType>)
        {
            hash = Cryptography::hash(key);
        } else {
            hash = key.hash();
        }

        USize index = hash & (capacity - 1);
        Node *current = buckets[index];

        while (current != nullptr) // Check if exists
        {
            if (current->key == key)
            {
                return current->value;
            }
            current = current->bucketNext;
        }

        Node *newNode = Memory::allocate<Node>(nodesAllocatorInfo);

        if constexpr (Moveable<KeyType>)
        {
            newNode->key.move(key);
        }
        else if constexpr (Copyable<KeyType>)
        {
            newNode->key.copy(key);
        } else {
            newNode->key = key;
        }

        newNode->value = ValueType{};

        newNode->bucketNext = buckets[index];
        buckets[index] = newNode;

        newNode->elementNext = sentinel->elementNext;
        newNode->elementPrevious = sentinel;
        sentinel->elementNext->elementPrevious = newNode;
        sentinel->elementNext = newNode;

        ++size;
        return newNode->value;
    }

    ValueType &operator[](const KeyType &key) noexcept
    {
        if (size >= USize(Float32(capacity) * maxLoadFactor) || capacity == 0)
        {
            rehash();
        }

        UInt64 hash;
        if constexpr (FunctionHashable<KeyType>)
        {
            hash = Cryptography::hash(key);
        } else {
            hash = key.hash();
        }

        USize index = hash & (capacity - 1);
        Node *current = buckets[index];

        while (current != nullptr) // Check if exists
        {
            if (current->key == key)
            {
                return current->value;
            }
            current = current->bucketNext;
        }

        Node *newNode = Memory::allocate<Node>(nodesAllocatorInfo);

        if constexpr (Copyable<KeyType>)
        {
            newNode->key.copy(key);
        } else {
            newNode->key = key;
        }

        newNode->value = ValueType{};

        newNode->bucketNext = buckets[index];
        buckets[index] = newNode;

        newNode->elementNext = sentinel->elementNext;
        newNode->elementPrevious = sentinel;
        sentinel->elementNext->elementPrevious = newNode;
        sentinel->elementNext = newNode;

        ++size;
        return newNode->value;
    }

    const ValueType &operator[](const KeyType &key) const noexcept
    {
        UInt64 hash;
        if constexpr (FunctionHashable<KeyType>)
        {
            hash = Cryptography::hash(key);
        } else {
            hash = key.hash();
        }

        USize index = hash & (capacity - 1);
        Node *current = buckets[index];

        while (current != nullptr) // Check if exists
        {
            if (current->key == key)
            {
                return current->value;
            }
            current = current->bucketNext;
        }

        assert(false && "Given key does not exists in map!");
        // NEVER REACHED - this is undefined behavior by design
        // Using this return value is a programming error
        return *reinterpret_cast<const ValueType *>(nullptr);
    }

    USize remove(const KeyType &key) noexcept
    {
        UInt64 hash;
        if constexpr (FunctionHashable<KeyType>)
        {
            hash = Cryptography::hash(key);
        } else {
            hash = key.hash();
        }

        USize index = hash & (capacity - 1);
        Node *current = buckets[index];
        Node *previous =  nullptr;

        while (current != nullptr) // Check if exists
        {
            if (current->key == key)
            {
                if constexpr (Finalizable<KeyType>)
                {
                    current->key.finalize();
                }
                if constexpr (Finalizable<ValueType>)
                {
                    current->value.finalize();
                }

                if (previous)
                {
                    previous->bucketNext = current->bucketNext;
                } else {
                    buckets[index] = current->bucketNext;
                }

                current->elementNext->elementPrevious = current->elementPrevious;
                current->elementPrevious->elementNext = current->elementNext;

                Memory::deallocate(nodesAllocatorInfo, current);

                --size;

                return 1;
            }
            previous = current;
            current = current->bucketNext;
        }
        return 0;
    }

    USize remove(const Iterator &iterator) noexcept
    {
        return remove(iterator.get_node()->key);
    }

    [[nodiscard]]
    Iterator find(const KeyType &key) const noexcept
    {
        UInt64 hash;
        if constexpr (FunctionHashable<KeyType>)
        {
            hash = Cryptography::hash(key);
        } else {
            hash = key.hash();
        }

        USize index = hash & (capacity - 1);
        Node *current = buckets[index];
        while (current != nullptr) // Check if exists
        {
            if (current->key == key) [[likely]]
            {
                return Iterator{ current };
            }
            current = current->bucketNext;
        }

        return end();
    }

    [[nodiscard]]
    Bool contains(const KeyType &key) const noexcept
    {
        UInt64 hash;
        if constexpr (FunctionHashable<KeyType>)
        {
            hash = Cryptography::hash(key);
        } else {
            hash = key.hash();
        }

        USize index = hash & (capacity - 1);
        Node *current = buckets[index];
        while (current != nullptr) // Check if exists
        {
            if (current->key == key)
            {
                return true;
            }
            current = current->bucketNext;
        }

        return false;
    }

    Void set_max_load_factor(const Float32 loadFactor) noexcept
    {
        assert(loadFactor > 0.0f && "This will cause infinite loop!");
        maxLoadFactor = loadFactor;
        if (size >= capacity * maxLoadFactor)
        {
            rehash();
        }
    }

    [[nodiscard]]
    Iterator begin() noexcept
    {
        return Iterator{ sentinel->elementNext };
    }

    [[nodiscard]]
    Iterator begin() const noexcept
    {
        return Iterator{ sentinel->elementNext };
    }

    [[nodiscard]]
    Iterator end() noexcept
    {
        return Iterator{ sentinel };
    }

    [[nodiscard]]
    Iterator end() const noexcept
    {
        return Iterator{ sentinel };
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
    USize get_capacity() const noexcept
    {
        return capacity;
    }

    [[nodiscard]]
    Float32 get_max_load_factor() const noexcept
    {
        return maxLoadFactor;
    }

    [[nodiscard]]
    Float32 get_load_factor() const noexcept
    {
        return capacity > 0 ? Float32(size) / Float32(capacity) : 0.0f;
    }

    Void rehash() noexcept
    {
        capacity = std::max(capacity, USize(32));
        while (size >= USize(Float32(capacity) * maxLoadFactor))
        {
            capacity <<= 1;
        }

        Memory::deallocate(bucketsAllocatorInfo, buckets);
        Node **newBuckets = Memory::allocate<Node *>(bucketsAllocatorInfo, capacity);
        for (Node *current = sentinel->elementNext; current != sentinel; current = current->elementNext)
        {
            UInt64 hash;
            if constexpr (FunctionHashable<KeyType>)
            {
                hash = Cryptography::hash(current->key);
            } else {
                hash = current->key.hash();
            }

            const USize index = hash & (capacity - 1);
            current->bucketNext = newBuckets[index];
            newBuckets[index] = current;
        }

        buckets = newBuckets;
    }

    Void clear() noexcept
    {
        for (Node* current = sentinel->elementNext; current != sentinel;)
        {
            Node *toClear = current;
            current = current->elementNext;

            if constexpr (Finalizable<KeyType>)
            {
                toClear->key.finalize();
            }
            if constexpr (Finalizable<ValueType>)
            {
                toClear->value.finalize();
            }

            Memory::deallocate(nodesAllocatorInfo, toClear);
        }

        sentinel->elementNext = sentinel;
        sentinel->elementPrevious = sentinel;

        for (USize i = 0; i < capacity; ++i)
        {
            buckets[i] = nullptr;
        }
        size = 0;
    }

    Void finalize() noexcept
    {
        assert(bucketsAllocatorInfo && "Allocator is nullptr!");
        if (size > 0)
        {
            clear();
        }

        Memory::deallocate(bucketsAllocatorInfo, buckets);
        Memory::deallocate(nodesAllocatorInfo, sentinel);

        *this = {};
    }
};