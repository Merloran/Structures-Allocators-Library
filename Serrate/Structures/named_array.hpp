#pragma once
#include "dynamic_array.hpp"
#include "hash_map.hpp"
#include "string.hpp"

template <Manual Type, typename IndexType>
requires std::is_convertible_v<IndexType, USize> &&
         std::is_constructible_v<IndexType, USize>
class NamedArray
{
private:
    DynamicArray<Type> elements;
    HashMap<String, IndexType> nameMap;

public:
    Void initialize(AllocatorInfo *mainAllocator, AllocatorInfo *nodesAllocator)
    {
        elements.initialize(mainAllocator);
        nameMap.initialize(nodesAllocator);
    }

    IndexType push_back(const String &name, const Type &element) noexcept
    {
        IndexType index{ elements.get_size() };
        nameMap.push(name, index);
        elements.push_back(element);
        return index;
    }

    IndexType emplace_back(const String &name, Type &element) noexcept
    {
        IndexType index{ elements.get_size() };
        nameMap.push(name, index);
        elements.emplace_back(element);
        return index;
    }

    IndexType emplace_back(String &name, const Type &element) noexcept
    {
        IndexType index{ elements.get_size() };
        nameMap.emplace(name, index);
        elements.push_back(element);
        return index;
    }

    IndexType emplace_back(String &name, Type &element) noexcept
    {
        IndexType index{ elements.get_size() };
        nameMap.emplace(name, index);
        elements.emplace_back(element);
        return index;
    }

    Void remove_back() noexcept
    {
        elements.remove_back();
        auto iterator = nameMap.find_value(IndexType(elements.get_size() - 1));
        nameMap.remove(iterator);
    }

    auto find(const String &name) const noexcept
    {
        return nameMap.find(name);
    }

    auto find(const String &name) noexcept
    {
        return nameMap.find(name);
    }

    auto find(const StringView &name) const noexcept
    {
        return nameMap.find(name);
    }

    auto find(const StringView &name) noexcept
    {
        return nameMap.find(name);
    }

    Bool contains(const String &name) const noexcept
    {
        return nameMap.contains(name);
    }

    Bool contains(const StringView &name) const noexcept
    {
        return nameMap.contains(name);
    }

    [[nodiscard]]
    Type &operator[](const USize index) noexcept
    {
        return elements[index];
    }

    [[nodiscard]]
    IndexType get_index(const String &name) const noexcept
    {
        return nameMap[name];
    }

    [[nodiscard]]
    IndexType get_index(const StringView &name) const noexcept
    {
        return nameMap[name];
    }

    Type &get_element(const String &name) noexcept
    {
        return elements[USize(nameMap[name])];
    }

    [[nodiscard]]
    const Type &get_element(const String &name) const noexcept
    {
        return elements[USize(nameMap[name])];
    }

    Type &get_element(const IndexType &index) noexcept
    {
        return elements[USize(index)];
    }

    [[nodiscard]]
    const Type &get_element(const IndexType &index) const noexcept
    {
        return elements[USize(index)];
    }

    Type *get_data() noexcept
    {
        return elements.get_data();
    }

    [[nodiscard]]
    const Type *get_data() const noexcept
    {
        return elements.get_data();
    }

    Type *begin() noexcept
    {
        return elements.begin();
    }

    [[nodiscard]]
    const Type *begin() const noexcept
    {
        return elements.begin();
    }

    Type *end() noexcept
    {
        return elements.end();
    }

    [[nodiscard]]
    const Type *end() const noexcept
    {
        return elements.end();
    }

    auto not_found() const noexcept
    {
        return nameMap.end();
    }

    [[nodiscard]]
    USize get_size() const noexcept
    {
        return elements.get_size();
    }

    Void clear() noexcept
    {
        elements.clear();
        nameMap.clear();
    }

    Void finalize() noexcept
    {
        elements.finalize();
        nameMap.finalize();
    }
};