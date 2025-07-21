#pragma once
#include "Serrate/Memory/memory_utils.hpp"
#include "Serrate/Utilities/cryptography.hpp"
#include "string_view.hpp"

#include <spdlog/spdlog.h>
#include <codecvt>


//TODO: Implement commented methods
//      Optimize replace and find

enum class EStringFindStrategy
{
    First,
    Last,
    CountOverlapping,
    CountNonOverlapping,
};

enum class EStringTrimStrategy
{
    Begin,
    End,
    All,
};

template <Manual Type>
class DynamicArray;


// Always initialize and finalize, do not make shallow copy by operator =,
// if you want to make reserve or any other method that can change size of string,
// do copy or move instead
template <Character Type>
class BasicString
{
private:
    static constexpr USize SSO_CAPACITY = 16 / sizeof(Type) - 1;
    static constexpr USize SSO_FLAG = 1UI64 << 63; // Last bit of size contains this flag
    AllocatorInfo *allocatorInfo;
    union
    {
        struct
        {
            Type *elements;
            USize capacity;
        };
        Type smallText[SSO_CAPACITY + 1]; // Last character should be '\0'
    };
    USize size;

public:
    BasicString() noexcept
    : allocatorInfo(AllocatorInfo::get_default_allocator())
    , elements(nullptr)
    , capacity(0)
    , size(SSO_FLAG)
    {}

    Void initialize(AllocatorInfo* allocator = AllocatorInfo::get_default_allocator()) noexcept
    {
        assert(allocator && "Invalid pointer!");

        allocatorInfo = allocator;
        size = SSO_FLAG;
        smallText[0] = Type();
    }

    Void initialize(const Type *text,
                    AllocatorInfo *allocator = AllocatorInfo::get_default_allocator()) noexcept
    {
        assert(allocator && "Invalid pointer!");
        assert(text && "Invalid pointer!");

        allocatorInfo = allocator;

        size = BasicStringView<Type>::length(text);
        Bool useSSO = size <= SSO_CAPACITY;
        if (useSSO)
        {
            memcpy(smallText, text, (size + 1) * sizeof(Type));
            size |= SSO_FLAG;
            return;
        }

        capacity = size + 1;
        elements = Memory::allocate<Type, false>(allocatorInfo, capacity);
        memcpy(elements, text, capacity * sizeof(Type));
    }

    template <USize Count>
    Void initialize(const Type text[Count],
                    AllocatorInfo *allocator = AllocatorInfo::get_default_allocator()) noexcept
    {
        assert(allocator && "Invalid pointer!");
        assert(text && "Invalid pointer!");

        allocatorInfo = allocator;

        size = Count;
        Bool useSSO = size <= SSO_CAPACITY;
        if (useSSO)
        {
            memcpy(smallText, text, (size + 1) * sizeof(Type));
            size |= SSO_FLAG;
            return;
        }

        capacity = size + 1;
        elements = Memory::allocate<Type, false>(allocatorInfo, capacity);
        memcpy(elements, text, capacity * sizeof(Type));
    }

    Void reserve(USize newCapacity) noexcept
    {
        ++newCapacity;
        if (newCapacity <= SSO_CAPACITY || (!(size & SSO_FLAG) && newCapacity <= capacity))
        {
            return;
        }

        Type *newElements = Memory::allocate<Type, false>(allocatorInfo, newCapacity);

        Bool isSSO = size & SSO_FLAG;
        if (isSSO)
        {
            size &= ~SSO_FLAG;
            memcpy(newElements, smallText, (size + 1) * sizeof(Type));
            elements = newElements;
            capacity = newCapacity;
            return;
        }

        if (elements)
        {
            memcpy(newElements, elements, (size + 1) * sizeof(Type));
            Memory::deallocate(allocatorInfo, elements);
        }
        elements = newElements;
        elements[size] = Type();
        capacity = newCapacity;
    }


    Bool check_prefix(const BasicString &prefix) const noexcept
    {
        const USize prefixSize = prefix.get_size();
        if (get_size() < prefixSize)
        {
            return false;
        }

        return memcmp(begin(), prefix.begin(), prefixSize * sizeof(Type)) == 0;
    }

    Bool check_prefix(const Type        *prefix) const noexcept
    {
        if (!prefix)
        {
            return false;
        }

        const USize prefixSize = BasicStringView<Type>::length(prefix);
        if (get_size() < prefixSize)
        {
            return false;
        }

        return memcmp(begin(), prefix, prefixSize * sizeof(Type)) == 0;
    }


    Bool check_suffix(const BasicString &suffix) const noexcept
    {
        const USize suffixSize = suffix.get_size();
        if (get_size() < suffixSize)
        {
            return false;
        }

        return memcmp(end() - suffixSize, suffix.begin(), suffixSize * sizeof(Type)) == 0;
    }

    Bool check_suffix(const Type        *suffix) const noexcept
    {
        if (!suffix)
        {
            return false;
        }

        const USize suffixSize = BasicStringView<Type>::length(suffix);
        if (get_size() < suffixSize)
        {
            return false;
        }
        return memcmp(end() - suffixSize, suffix, suffixSize * sizeof(Type)) == 0;
    }


    USize find(const BasicString &substring,
                const EStringFindStrategy strategy = EStringFindStrategy::First) const noexcept
    {
        const USize substringSize = substring.get_size();
        const USize selfSize = size & ~SSO_FLAG;
        switch (strategy)
        {
            case EStringFindStrategy::First:
            {
                if (selfSize < substringSize)
                {
                    return ~USize(0);
                }
                const Type *selfData = begin();
                const Type *substringData = substring.begin();
                for (USize i = 0; i < selfSize - substringSize; ++i, ++selfData)
                {
                    if (memcmp(selfData, substringData, substringSize * sizeof(Type)) == 0)
                    {
                        return i;
                    }
                }
                return ~USize(0);
            }
            case EStringFindStrategy::Last:
            {
                if (selfSize < substringSize)
                {
                    return ~USize(0);
                }
                const Type *selfData = end() - substringSize;
                const Type *substringData = substring.begin();
                for (USize i = selfSize - substringSize + 1; i > 0; --i, --selfData)
                {
                    if (memcmp(selfData, substringData, substringSize * sizeof(Type)) == 0)
                    {
                        return i - 1;
                    }
                }
                return ~USize(0);
            }
            case EStringFindStrategy::CountOverlapping:
            {
                if (selfSize < substringSize)
                {
                    return 0;
                }

                if  (substringSize == 0)
                {
                    return ~USize(0);
                }

                USize occurrencesCount = 0;
                const Type *substringData = substring.begin();
                for (const Type *selfData = begin(), *terminationElement = end() - substringSize;
                     selfData <= terminationElement;
                     ++selfData)
                {
                    if (memcmp(selfData, substringData, substringSize * sizeof(Type)) == 0)
                    {
                        ++occurrencesCount;
                    }
                }
                return occurrencesCount;
            }
            case EStringFindStrategy::CountNonOverlapping:
            {
                if (selfSize < substringSize)
                {
                    return 0;
                }

                if  (substringSize == 0)
                {
                    return ~USize(0);
                }

                USize occurrencesCount = 0;
                const Type *substringData = substring.begin();
                for (const Type *selfData = begin(), *terminationElement = end() - substringSize; 
                     selfData <= terminationElement;
                     ++selfData)
                {
                    if (memcmp(selfData, substringData, substringSize * sizeof(Type)) == 0)
                    {
                        ++occurrencesCount;
                        selfData += substringSize - 1;
                    }
                }
                return occurrencesCount;
            }
        }
        return ~USize(0); // Sanity check
    }

    USize find(const Type *substring,
                const EStringFindStrategy strategy = EStringFindStrategy::First) const noexcept
    {
        if (!substring)
        {
            return ~USize(0);
        }

        const USize substringSize = BasicStringView<Type>::length(substring);
        const USize selfSize = size & ~SSO_FLAG;
        switch (strategy)
        {
            case EStringFindStrategy::First:
            {
                if (selfSize < substringSize)
                {
                    return ~USize(0);
                }
                const Type *selfData = begin();
                const Type *substringData = substring;
                for (USize i = 0; i < selfSize - substringSize; ++i, ++selfData)
                {
                    if (memcmp(selfData, substringData, substringSize * sizeof(Type)) == 0)
                    {
                        return i;
                    }
                }
                return ~USize(0);
            }
            case EStringFindStrategy::Last:
            {
                if (selfSize < substringSize)
                {
                    return ~USize(0);
                }
                const Type *selfData = end() - substringSize; //TODO: check it
                const Type *substringData = substring;
                for (USize i = selfSize - substringSize + 1; i > 0; --i, --selfData)
                {
                    if (memcmp(selfData, substringData, substringSize * sizeof(Type)) == 0)
                    {
                        return i - 1;
                    }
                }
                return ~USize(0);
            }
            case EStringFindStrategy::CountOverlapping:
            {
                if (selfSize < substringSize)
                {
                    return 0;
                }

                if (substringSize == 0)
                {
                    return ~USize(0);
                }

                USize occurrencesCount = 0;
                const Type *substringData = substring;
                for (const Type *selfData = begin(), *terminationElement = end() - substringSize;
                     selfData <= terminationElement;
                     ++selfData)
                {
                    if (memcmp(selfData, substringData, substringSize * sizeof(Type)) == 0)
                    {
                        ++occurrencesCount;
                    }
                }
                return occurrencesCount;
            }
            case EStringFindStrategy::CountNonOverlapping:
            {
                if (selfSize < substringSize)
                {
                    return 0;
                }

                if (substringSize == 0)
                {
                    return ~USize(0);
                }

                USize occurrencesCount = 0;
                const Type *substringData = substring;
                for (const Type *selfData = begin(), *terminationElement = end() - substringSize;
                     selfData <= terminationElement;
                     ++selfData)
                {
                    if (memcmp(selfData, substringData, substringSize * sizeof(Type)) == 0)
                    {
                        ++occurrencesCount;
                        selfData += substringSize - 1;
                    }
                }
                return occurrencesCount;
            }
        }
        return ~USize(0); // Sanity check
    }


    Bool contains(const BasicString &substring) const noexcept
    {
        const USize selfSize = size & ~SSO_FLAG;
        const USize substringSize = substring.get_size();
        if (selfSize < substringSize)
        {
            return false;
        }
        Type *selfData = begin();
        const Type *substringData = substring.begin();
        for (USize i = 0; i < selfSize - substringSize; ++i, ++selfData)
        {
            if (memcmp(selfData, substringData, substringSize * sizeof(Type)) == 0)
            {
                return true;
            }
        }
        return false;
    }

    Bool contains(const Type        *substring) const noexcept
    {
        if (!substring)
        {
            return false;
        }

        const USize substringSize = BasicStringView<Type>::length(substring);
        const USize selfSize = size & ~SSO_FLAG;

        if (selfSize < substringSize)
        {
            return false;
        }
        Type *selfData = begin();
        const Type *substringData = substring;
        for (USize i = 0; i < selfSize - substringSize; ++i, ++selfData)
        {
            if (memcmp(selfData, substringData, substringSize * sizeof(Type)) == 0)
            {
                return true;
            }
        }
        return false;
    }


    Int8 compare(const BasicString &other) const noexcept
    {
        if (get_size() != other.get_size())
        {
            return false;
        }

        return memcmp(begin(), other.begin(), size & ~SSO_FLAG);
    }

    Int8 compare(const BasicStringView<Type> &other) const noexcept
    {
        if (get_size() != other.get_size())
        {
            return false;
        }

        return memcmp(begin(), other.begin(), size & ~SSO_FLAG);
    }

    Int8 compare(const Type        *other) const noexcept
    {
        return strcmp(begin(), other);
    }


    Bool equals(const BasicString &other) const noexcept
    {
        if (get_size() != other.get_size())
        {
            return false;
        }

        return memcmp(begin(), other.begin(), size & ~SSO_FLAG) == 0;
    }

    Bool equals(const BasicStringView<Type> &other) const noexcept
    {
        if (get_size() != other.get_size())
        {
            return false;
        }

        return memcmp(begin(), other.begin(), size & ~SSO_FLAG) == 0;
    }

    Bool equals(const Type        *other) const noexcept
    {
        return strcmp(begin(), other) == 0;
    }


    Void push(const USize position, const BasicString &other) noexcept
    {
        const USize selfSize = size & ~SSO_FLAG;
        assert(position <= selfSize);

        const USize otherSize = other.get_size();
        if (otherSize == 0)
        {
            return;
        }

        reserve(selfSize + otherSize);

        const USize partSize = selfSize - position;
        Type *partOldBegin = begin() + position;
        Type *partNewBegin = partOldBegin + otherSize;
        memmove(partNewBegin, partOldBegin, partSize * sizeof(Type));
        memcpy(partOldBegin, other.get_data(), otherSize * sizeof(Type));
        size += otherSize;
        *end() = Type();
    }

    Void push(const USize position, const Type        *other) noexcept
    {
        if (!other)
        {
            return;
        }

        const USize selfSize = size & ~SSO_FLAG;
        assert(position <= selfSize);

        const USize otherSize = BasicStringView<Type>::length(other);
        if (otherSize == 0)
        {
            return;
        }

        reserve(selfSize + otherSize);

        const USize partSize = selfSize - position;
        Type *partOldBegin = begin() + position;
        Type *partNewBegin = partOldBegin + otherSize;
        memmove(partNewBegin, partOldBegin, partSize * sizeof(Type));
        memcpy(partOldBegin, other, otherSize * sizeof(Type));
        size += otherSize;
        *end() = Type();
    }


    Void push_back(const BasicString &other) noexcept
    {
        const USize selfSize = get_size();
        const USize otherSize = other.get_size();

        reserve(selfSize + otherSize);

        memcpy(end(), other.get_data(), otherSize * sizeof(Type));
        size += otherSize;
        *end() = Type();
    }

    Void push_back(const Type        *other) noexcept
    {
        if (!other)
        {
            return;
        }

        const USize selfSize = get_size();
        const USize otherSize = BasicStringView<Type>::length(other);

        reserve(selfSize + otherSize);

        memcpy(end(), other, otherSize * sizeof(Type));
        size += otherSize;
        *end() = Type();
    }

    Void push_back(const Type         other) noexcept
    {
        reserve(get_size() + 1);

        *end() = other;
        ++size;
        *end() = Type();
    }


    Void replace(const BasicString &newSubstring, const BasicString &oldSubstring) noexcept
    {
        const USize oldSubstringSize = oldSubstring.get_size();

        if (oldSubstringSize == 0)
        {
            SPDLOG_WARN("Tried to replace empty substring.");
            return;
        }

        const USize selfSize = get_size();
        const USize newSubstringSize = newSubstring.get_size();
        const Int64 sizeDifference = newSubstringSize - oldSubstringSize;
        if (oldSubstringSize < newSubstringSize)
        {
            const USize occurrences = find(oldSubstring, EStringFindStrategy::CountNonOverlapping);
            reserve(selfSize + occurrences * sizeDifference);
            size += occurrences * sizeDifference;
        }

        const Type *oldSubstringData = oldSubstring.begin();
        const Type *newSubstringData = newSubstring.begin();
        USize currentOffset = 0;
        for (Type *selfData = begin(), *terminationElement = end() - oldSubstringSize;
             selfData <= terminationElement;
             ++selfData, ++currentOffset)
        {
            if (memcmp(selfData, oldSubstringData, oldSubstringSize * sizeof(Type)) == 0)
            {
                currentOffset += oldSubstringSize;
                Type *stringTail = selfData + oldSubstringSize;
                memmove(stringTail + sizeDifference, stringTail, (selfSize - currentOffset) * sizeof(Type));
                memcpy(selfData, newSubstringData, newSubstringSize * sizeof(Type));
                currentOffset += sizeDifference - 1;
                selfData += newSubstringSize - 1;
            }
        }
    }
    
    Void replace(const Type        *newSubstring, const Type        *oldSubstring) noexcept
    {
        if (!newSubstring || !oldSubstring)
        {
            SPDLOG_WARN("Tried to pass nullptr to replace.");
            return;
        }

        const USize oldSubstringSize = BasicStringView<Type>::length(oldSubstring);

        if (oldSubstringSize == 0)
        {
            SPDLOG_WARN("Tried to replace empty substring.");
            return;
        }

        USize selfSize = get_size();
        const USize newSubstringSize = BasicStringView<Type>::length(newSubstring);
        const Int64 sizeDifference = newSubstringSize - oldSubstringSize;
        if (oldSubstringSize < newSubstringSize)
        {
            const USize occurrences = find(oldSubstring, EStringFindStrategy::CountNonOverlapping);
            reserve(selfSize + occurrences * sizeDifference);
            size += occurrences * sizeDifference;
            selfSize = get_size();
        }

        const Type *oldSubstringData = oldSubstring;
        const Type *newSubstringData = newSubstring;
        USize currentOffset = 0;
        for (Type *selfData = begin(), *terminationElement = end() - oldSubstringSize;
             selfData <= terminationElement;
             ++selfData, ++currentOffset)
        {
            if (memcmp(selfData, oldSubstringData, oldSubstringSize * sizeof(Type)) == 0)
            {
                currentOffset += oldSubstringSize;
                Type *stringTail = selfData + oldSubstringSize;
                memmove(stringTail + sizeDifference, stringTail, (selfSize - currentOffset) * sizeof(Type));
                memcpy(selfData, newSubstringData, newSubstringSize * sizeof(Type));
                currentOffset += sizeDifference - 1;
                selfData += newSubstringSize - 1;
            }
        }
    }
    
    Void replace(const BasicString &newSubstring, const Type        *oldSubstring) noexcept
    {
        if (!oldSubstring)
        {
            SPDLOG_WARN("Tried to pass nullptr to replace.");
            return;
        }

        const USize oldSubstringSize = BasicStringView<Type>::length(oldSubstring);

        if (oldSubstringSize == 0)
        {
            SPDLOG_WARN("Tried to replace empty substring.");
            return;
        }

        const USize selfSize = get_size();
        const USize newSubstringSize = newSubstring.get_size();
        const Int64 sizeDifference = newSubstringSize - oldSubstringSize;
        if (oldSubstringSize < newSubstringSize)
        {
            const USize occurrences = find(oldSubstring, EStringFindStrategy::CountNonOverlapping);
            reserve(selfSize + occurrences * sizeDifference);
            size += occurrences * sizeDifference;
        }

        const Type *oldSubstringData = oldSubstring;
        const Type *newSubstringData = newSubstring.begin();
        USize currentOffset = 0;
        for (Type *selfData = begin(), *terminationElement = end() - oldSubstringSize;
             selfData <= terminationElement;
             ++selfData, ++currentOffset)
        {
            if (memcmp(selfData, oldSubstringData, oldSubstringSize * sizeof(Type)) == 0)
            {
                currentOffset += oldSubstringSize;
                Type *stringTail = selfData + oldSubstringSize;
                memmove(stringTail + sizeDifference, stringTail, (selfSize - currentOffset) * sizeof(Type));
                memcpy(selfData, newSubstringData, newSubstringSize * sizeof(Type));
                currentOffset += sizeDifference - 1;
                selfData += newSubstringSize - 1;
            }
        }
    }
    
    Void replace(const Type        *newSubstring, const BasicString &oldSubstring) noexcept
    {
        if (!newSubstring)
        {
            SPDLOG_WARN("Tried to pass nullptr to replace.");
            return;
        }

        const USize oldSubstringSize = oldSubstring.get_size();

        if (oldSubstringSize == 0)
        {
            SPDLOG_WARN("Tried to replace empty substring.");
            return;
        }

        const USize selfSize = get_size();
        const USize newSubstringSize = BasicStringView<Type>::length(newSubstring);
        const Int64 sizeDifference = newSubstringSize - oldSubstringSize;
        if (oldSubstringSize < newSubstringSize)
        {
            const USize occurrences = find(oldSubstring, EStringFindStrategy::CountNonOverlapping);
            reserve(selfSize + occurrences * sizeDifference);
            size += occurrences * sizeDifference;
        }

        const Type *oldSubstringData = oldSubstring.begin();
        const Type *newSubstringData = newSubstring;
        USize currentOffset = 0;
        for (Type *selfData = begin(), *terminationElement = end() - oldSubstringSize;
             selfData <= terminationElement;
             ++selfData, ++currentOffset)
        {
            if (memcmp(selfData, oldSubstringData, oldSubstringSize * sizeof(Type)) == 0)
            {
                currentOffset += oldSubstringSize;
                Type *stringTail = selfData + oldSubstringSize;
                memmove(stringTail + sizeDifference, stringTail, (selfSize - currentOffset) * sizeof(Type));
                memcpy(selfData, newSubstringData, newSubstringSize * sizeof(Type));
                currentOffset += sizeDifference - 1;
                selfData += newSubstringSize - 1;
            }
        }
    }


    template<typename = Void>
    requires (std::is_same_v<Type, Char8> || std::is_same_v<Type, Char>)
    Void to_upper() noexcept
    {
        for (Type *element = begin(); *element != Char8(); ++element)
        {
            if (*element >= Char8('a') && *element <= Char8('z'))
            {
                *element &= ~Char8(32);
            }
        }
    }

    template<typename = Void>
    requires (std::is_same_v<Type, Char8> || std::is_same_v<Type, Char>)
    Void to_lower() noexcept
    {
        for (Type *element = begin(); *element != Char8(); ++element)
        {
            if (*element >= Char8('A') && *element <= Char8('Z'))
            {
                *element |= Char8(32);
            }
        }
    }

    Void trim(const EStringTrimStrategy strategy = EStringTrimStrategy::All) noexcept
    {
        switch (strategy)
        {
            case EStringTrimStrategy::Begin:
            {
                USize offset = 0;
                Type *selfBegin = begin();
                for (Type *element = selfBegin; BasicStringView<Type>::is_white_space(*element); ++element)
                {
                    ++offset;
                }
                size -= offset;
                memcpy(selfBegin, selfBegin + offset, size & ~SSO_FLAG);
                *end() = Type();
                break;
            }
            case EStringTrimStrategy::End:
            {
                for (Type *element = end() - 1; BasicStringView<Type>::is_white_space(*element); --element)
                {
                    --size;
                }
                *end() = Type();
                break;
            }
            case EStringTrimStrategy::All:
            {
                USize offset = 0;
                for (Type *element = begin(); *element != Type(); ++element)
                {
                    if (BasicStringView<Type>::is_white_space(*element))
                    {
                        ++offset;
                    } else {
                        *(element - offset) = *element;
                    }
                }
                size -= offset;
                *end() = Type();
                break;
            }
        }
    }

    Void reverse() noexcept
    {
        Type  holder;
        Type *beginElement = begin();
        Type *endElement   = end() - 1;

        for (; beginElement < endElement; ++beginElement, --endElement)
        {
            std::swap(*beginElement, *endElement);
        }
    }

    USize remove(const USize position, const USize length) noexcept
    {
        const USize selfSize = size & ~SSO_FLAG;
        if (position + length > selfSize)
        {
            return 0;
        }

        Type *elementsDestination = begin() + position;
        Type *elementsToMove      = elementsDestination + length;
        const USize elementsSize  = selfSize - (position + length);

        memcpy(elementsDestination, elementsToMove, elementsSize);
        size -= length;
        *end() = Type();
        return length;
    }

    USize remove(Type character) noexcept
    {
        USize offset = 0;
        for (Type *element = begin(); *element != Type(); ++element)
        {
            if (*element == character)
            {
                ++offset;
            } else {
                *(element - offset) = *element;
            }
        }
        size -= offset;
        *end() = Type();
        return offset;
    }

    Void move(BasicString &source) noexcept
    {
        if (&source == this)
        {
            return;
        }
        finalize();
        allocatorInfo = source.allocatorInfo;
        elements      = source.elements;
        capacity      = source.capacity;
        size          = source.size;

        source = {};
    }

    Void copy(const BasicString &source) noexcept
    {
        if (&source == this)
        {
            return;
        }
        finalize();
        allocatorInfo = source.allocatorInfo;
        size          = source.size;
        capacity      = source.capacity;
        if (source.size & SSO_FLAG)
        {
            elements = source.elements;
        } else {
            elements = Memory::allocate<Type, false>(allocatorInfo, capacity);
            memcpy(elements, source.elements, size * sizeof(Type));
        }
    }

    // [[nodiscard]]
    // String substring(UInt64 start, UInt64 length) const;
    // [[nodiscard]]
    // DynamicArray<String> split(CharacterType separator) const;

    [[nodiscard]]
    Bool is_empty() const noexcept
    {
        return (size & ~SSO_FLAG) == 0;
    }

    [[nodiscard]]
    USize get_size() const noexcept
    {
        return size & ~SSO_FLAG;
    }

    [[nodiscard]]
    USize get_capacity() const noexcept
    {
        return size & SSO_FLAG ? SSO_CAPACITY : capacity;
    }

    UInt64 hash() const noexcept
    {
        if (size & SSO_FLAG)
        {
            return Cryptography::hash(smallText, size & ~SSO_FLAG);
        }
        return Cryptography::hash(elements, size & ~SSO_FLAG);
    }

    Type *get_data() noexcept
    {
        return size & SSO_FLAG ? smallText : elements;
    }

    [[nodiscard]]
    const Type *get_data() const noexcept
    {
        return size & SSO_FLAG ? smallText : elements;
    }


    Type *begin() noexcept
    {
        return size & SSO_FLAG ? smallText : elements;
    }

    [[nodiscard]]
    const Type *begin() const noexcept
    {
        return size & SSO_FLAG ? smallText : elements;
    }


    Type *end() noexcept
    {
        return (size & SSO_FLAG) ? smallText + (size & ~SSO_FLAG) : elements + (size & ~SSO_FLAG);
    }

    [[nodiscard]]
    const Type *end() const noexcept
    {
        return (size & SSO_FLAG) ? smallText + (size & ~SSO_FLAG) : elements + (size & ~SSO_FLAG);
    }

#pragma region OPERATORS

    Type &operator[](USize index) noexcept
    {
        assert(index < size & ~SSO_FLAG);
        return size & SSO_FLAG ? smallText[index] : elements[index];
    }

    Type  operator[](USize index) const noexcept
    {
        assert(index < size & ~SSO_FLAG);
        return size & SSO_FLAG ? smallText[index] : elements[index];
    }


    BasicString operator+(const BasicString   &other) const noexcept
    {
        BasicString copy;
        copy.copy(*this);
        copy += other;
        return copy;
    }

    BasicString operator+(const Type *other) const noexcept
    {
        BasicString copy;
        copy.copy(*this);
        copy += other;
        return copy;
    }

    BasicString operator+(      Type  other) const noexcept
    {
        BasicString copy;
        copy.copy(*this);
        copy += other;
        return copy;
    }


    Void operator+=(const BasicString   &other)  noexcept
    {
        append(other);
    }

    Void operator+=(const Type *other) noexcept
    {
        append(other);
    }

    Void operator+=(      Type  other) noexcept
    {
        append(other);
    }

    
    Bool operator!=(const BasicString &other) const noexcept 
    {
        return !equals(other);
    }
    
    Bool operator==(const BasicString &other) const noexcept
    {
        return equals(other);
    }
    
    Bool operator< (const BasicString &other) const noexcept
    {
        return compare(other) < 0I8;
    }
    
    Bool operator<=(const BasicString &other) const noexcept
    {
        return compare(other) <= 0I8;
    }
    
    Bool operator> (const BasicString &other) const noexcept
    {
        return compare(other) > 0I8;
    }
    
    Bool operator>=(const BasicString &other) const noexcept
    {
        return compare(other) >= 0I8;
    }


    Bool operator!=(const Type *other) const noexcept
    {
        return equals(other);
    }

    Bool operator==(const Type *other) const noexcept
    {
        return equals(other);
    }

    Bool operator< (const Type *other) const noexcept
    {
        return compare(other) < 0I8;
    }

    Bool operator<=(const Type *other) const noexcept
    {
        return compare(other) <= 0I8;
    }

    Bool operator> (const Type *other) const noexcept
    {
        return compare(other) > 0I8;
    }

    Bool operator>=(const Type *other) const noexcept
    {
        return compare(other) >= 0I8;
    }


    Bool operator!=(const BasicStringView<Type> &other) const noexcept
    {
        return equals(other);
    }

    Bool operator==(const BasicStringView<Type> &other) const noexcept
    {
        return equals(other);
    }

    Bool operator< (const BasicStringView<Type> &other) const noexcept
    {
        return compare(other) < 0I8;
    }

    Bool operator<=(const BasicStringView<Type> &other) const noexcept
    {
        return compare(other) <= 0I8;
    }

    Bool operator> (const BasicStringView<Type> &other) const noexcept
    {
        return compare(other) > 0I8;
    }

    Bool operator>=(const BasicStringView<Type> &other) const noexcept
    {
        return compare(other) >= 0I8;
    }

#pragma endregion 

    Void clear() noexcept
    {
        if (size & SSO_FLAG)
        {
            smallText[0] = Type();
        } else {
            elements[0] = Type();
        }
    }

    Void finalize() noexcept
    {
        if (size & SSO_FLAG)
        {
            smallText[0] = Type();
            return;
        }

        if (!elements)
        {
            *this = {};
            return;
        }

        clear();
        Memory::deallocate(allocatorInfo, elements);
        *this = {};
    }

    // static String format(const CharacterType *fmt, ...);
};

template <Character Type>
std::ostream &operator<<(std::ostream &os, const BasicString<Type> &string) noexcept
{
    if constexpr (std::is_same_v<Type, Char>)
    {
        return os << string.get_data();
    }
    else if constexpr (std::is_same_v<Type, Char8>)
    {
        return os << reinterpret_cast<const Char *>(string.get_data());
    } else {
        std::wstring wstring;

        for (Type c : string) 
        {
            if (c == 0)
            {
                break;
            }

            if (c <= 0xFFFF) 
            {
                wstring += static_cast<WChar>(c);
            } else {
                c -= 0x10000;
                wstring += static_cast<WChar>((c >> 10) + 0xD800);
                wstring += static_cast<WChar>((c & 0x3FF) + 0xDC00);
            }
        }

        return os << std::wstring_convert<std::codecvt_utf8<WChar>>().to_bytes(wstring);
    }
}

using String   = BasicString<Char  >;
using String8  = BasicString<Char8 >;
using WString  = BasicString<WChar >;
using String16 = BasicString<Char16>;
using String32 = BasicString<Char32>;