#pragma once
#include <codecvt>

#include "Memory/memory_utils.hpp"

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

template <typename Type>
class DynamicArray;

template <typename CharacterType>
requires (std::is_same_v<CharacterType, Char  > ||
          std::is_same_v<CharacterType, Char8 > ||
          std::is_same_v<CharacterType, WChar > ||
          std::is_same_v<CharacterType, Char16> ||
          std::is_same_v<CharacterType, Char32>)
class BasicString
{
private:
    static constexpr UInt64 SSO_CAPACITY = 16 / sizeof(CharacterType) - 1;
    static constexpr UInt64 SSO_FLAG = 1UI64 << 63; // Last bit of size contains this flag
    AllocatorInfo *allocatorInfo;
    union
    {
        struct
        {
            CharacterType *elements;
            UInt64 capacity;
        };
        CharacterType smallText[SSO_CAPACITY + 1]; // Last character should be '\0'
    };
    UInt64 size;

public:
    Void initialize(AllocatorInfo* allocator = AllocatorInfo::get_default_allocator()) noexcept
    {
        allocatorInfo = allocator;
        if (!allocatorInfo)
        {
            SPDLOG_WARN("Allocator is nullptr!");
            return;
        }
        size = SSO_FLAG;
        smallText[0] = CharacterType();
    }

    Void initialize(const CharacterType* text,
                    AllocatorInfo* allocator = AllocatorInfo::get_default_allocator()) noexcept
    {
        allocatorInfo = allocator;
        if (!allocatorInfo)
        {
            SPDLOG_WARN("Allocator is nullptr!");
            return;
        }

        if (!text)
        {
            size = SSO_FLAG;
            smallText[0] = CharacterType();
            return;
        }

        size = BasicString::length(text);
        Bool useSSO = size <= SSO_CAPACITY;
        if (useSSO)
        {
            memcpy(smallText, text, (size + 1) * sizeof(CharacterType));
            size |= SSO_FLAG;
            return;
        }

        capacity = size + 1;
        elements = static_cast<CharacterType*>(allocatorInfo->allocate(allocator->allocator, 
                                               capacity * sizeof(CharacterType)));

        memcpy(smallText, text, capacity * sizeof(CharacterType));
    }

    Void reserve(const UInt64 newCapacity) noexcept
    {
        if (newCapacity + 1 <= capacity || newCapacity <= SSO_CAPACITY)
        {
            return;
        }

        CharacterType *newElements = static_cast<CharacterType *>(allocatorInfo->allocate(allocatorInfo->allocator, 
                                                                                          newCapacity * sizeof(CharacterType)));

        Bool isSSO = size & SSO_FLAG;
        if (isSSO)
        {
            size &= ~SSO_FLAG;
            memcpy(newElements, smallText, (size + 1) * sizeof(CharacterType));
            capacity = newCapacity;
            return;
        }

        if (elements)
        {
            memcpy(newElements, elements, (size + 1) * sizeof(CharacterType));
            allocatorInfo->deallocate(allocatorInfo->allocator, elements);
        }
        elements = newElements;
        elements[size] = CharacterType();
        capacity = newCapacity;
    }


    Bool check_prefix(const BasicString        &prefix) const noexcept
    {
        const UInt64 prefixSize = prefix.get_size();
        if (size < prefixSize)
        {
            return false;
        }

        return memcmp(begin(), prefix.begin(), prefixSize * sizeof(CharacterType)) == 0;
    }

    Bool check_prefix(const CharacterType *prefix) const noexcept
    {
        if (!prefix)
        {
            return false;
        }

        const UInt64 prefixSize = BasicString::length(prefix);
        if (size < prefixSize)
        {
            return false;
        }

        return std::memcmp(begin(), prefix, prefixSize * sizeof(CharacterType)) == 0;
    }


    Bool check_suffix(const BasicString        &suffix) const noexcept
    {
        const UInt64 suffixSize = suffix.get_size();
        if (size < suffixSize)
        {
            return false;
        }

        return memcmp(end() - suffixSize - 1, suffix.begin(), suffixSize * sizeof(CharacterType)) == 0;
    }

    Bool check_suffix(const CharacterType *suffix) const noexcept
    {
        if (!suffix)
        {
            return false;
        }

        const UInt64 suffixSize = BasicString::length(suffix);
        if (size < suffixSize)
        {
            return false;
        }

        return memcmp(end() - suffixSize - 1, suffix, suffixSize * sizeof(CharacterType)) == 0;
    }


    UInt64 find(const BasicString& substring,
                const EStringFindStrategy strategy = EStringFindStrategy::First) const noexcept
    {
        const UInt64 substringSize = substring.get_size();
        const UInt64 selfSize = size & ~SSO_FLAG;
        switch (strategy)
        {
            case EStringFindStrategy::First:
            {
                if (selfSize < substringSize)
                {
                    return ~UInt64(0);
                }
                CharacterType *selfData = begin();
                const CharacterType *substringData = substring.begin();
                for (UInt64 i = 0; i < selfSize - substringSize; ++i, ++selfData)
                {
                    if (memcmp(selfData, substringData, substringSize * sizeof(CharacterType)) == 0)
                    {
                        return i;
                    }
                }
                return ~UInt64(0);
            }
            case EStringFindStrategy::Last:
            {
                if (selfSize < substringSize)
                {
                    return ~UInt64(0);
                }
                CharacterType *selfData = end() - substringSize - 1;
                const CharacterType *substringData = substring.begin();
                for (UInt64 i = selfSize - substringSize + 1; i > 0; --i, --selfData)
                {
                    if (memcmp(selfData, substringData, substringSize * sizeof(CharacterType)) == 0)
                    {
                        return i - 1;
                    }
                }
                return ~UInt64(0);
            }
            case EStringFindStrategy::CountOverlapping:
            {
                if (selfSize < substringSize)
                {
                    return 0;
                }

                if  (substringSize == 0)
                {
                    return ~UInt64(0);
                }

                UInt64 occurrencesCount = 0;
                const CharacterType *substringData = substring.begin();
                for (CharacterType *selfData = begin(), *terminationElement = end() - substringSize - 1; 
                     selfData < terminationElement;
                     ++selfData)
                {
                    if (memcmp(selfData, substringData, substringSize * sizeof(CharacterType)) == 0)
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
                    return ~UInt64(0);
                }

                UInt64 occurrencesCount = 0;
                const CharacterType *substringData = substring.begin();
                for (CharacterType *selfData = begin(), *terminationElement = end() - substringSize - 1; 
                     selfData < terminationElement;
                     ++selfData)
                {
                    if (memcmp(selfData, substringData, substringSize * sizeof(CharacterType)) == 0)
                    {
                        ++occurrencesCount;
                        selfData += substringSize - 1;
                    }
                }
                return occurrencesCount;
            }
        }
        return ~UInt64(0); // Sanity check
    }

    UInt64 find(const CharacterType *substring,
                const EStringFindStrategy strategy = EStringFindStrategy::First) const noexcept
    {
        if (!substring)
        {
            return ~UInt64(0);
        }

        const UInt64 substringSize = BasicString::length(substring);
        const UInt64 selfSize = size & ~SSO_FLAG;
        switch (strategy)
        {
            case EStringFindStrategy::First:
            {
                if (selfSize < substringSize)
                {
                    return ~UInt64(0);
                }
                CharacterType *selfData = begin();
                const CharacterType *substringData = substring;
                for (UInt64 i = 0; i < selfSize - substringSize; ++i, ++selfData)
                {
                    if (memcmp(selfData, substringData, substringSize * sizeof(CharacterType)) == 0)
                    {
                        return i;
                    }
                }
                return ~UInt64(0);
            }
            case EStringFindStrategy::Last:
            {
                if (selfSize < substringSize)
                {
                    return ~UInt64(0);
                }
                CharacterType *selfData = end() - substringSize - 1;
                const CharacterType *substringData = substring;
                for (UInt64 i = selfSize - substringSize + 1; i > 0; --i, --selfData)
                {
                    if (memcmp(selfData, substringData, substringSize * sizeof(CharacterType)) == 0)
                    {
                        return i - 1;
                    }
                }
                return ~UInt64(0);
            }
            case EStringFindStrategy::CountOverlapping:
            {
                if (selfSize < substringSize)
                {
                    return 0;
                }

                if (substringSize == 0)
                {
                    return ~UInt64(0);
                }

                UInt64 occurrencesCount = 0;
                const CharacterType *substringData = substring;
                for (CharacterType *selfData = begin(), *terminationElement = end() - substringSize - 1;
                     selfData < terminationElement;
                     ++selfData)
                {
                    if (memcmp(selfData, substringData, substringSize * sizeof(CharacterType)) == 0)
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
                    return ~UInt64(0);
                }

                UInt64 occurrencesCount = 0;
                const CharacterType *substringData = substring;
                for (CharacterType *selfData = begin(), *terminationElement = end() - substringSize - 1;
                     selfData < terminationElement;
                     ++selfData)
                {
                    if (memcmp(selfData, substringData, substringSize * sizeof(CharacterType)) == 0)
                    {
                        ++occurrencesCount;
                        selfData += substringSize - 1;
                    }
                }
                return occurrencesCount;
            }
        }
        return ~UInt64(0); // Sanity check
    }


    Bool contains(const BasicString        &substring) const noexcept
    {
        const UInt64 selfSize = size & ~SSO_FLAG;
        const UInt64 substringSize = substring.get_size();
        if (selfSize < substringSize)
        {
            return false;
        }
        CharacterType *selfData = begin();
        const CharacterType *substringData = substring.begin();
        for (UInt64 i = 0; i < selfSize - substringSize; ++i, ++selfData)
        {
            if (memcmp(selfData, substringData, substringSize * sizeof(CharacterType)) == 0)
            {
                return true;
            }
        }
        return false;
    }

    Bool contains(const CharacterType *substring) const noexcept
    {
        if (!substring)
        {
            return false;
        }

        const UInt64 substringSize = BasicString::length(substring);
        const UInt64 selfSize = size & ~SSO_FLAG;

        if (selfSize < substringSize)
        {
            return false;
        }
        CharacterType *selfData = begin();
        const CharacterType *substringData = substring;
        for (UInt64 i = 0; i < selfSize - substringSize; ++i, ++selfData)
        {
            if (memcmp(selfData, substringData, substringSize * sizeof(CharacterType)) == 0)
            {
                return true;
            }
        }
        return false;
    }


    Int8 compare(const BasicString        &other) const noexcept
    {
        return strcmp(begin(), other.begin());
    }

    Int8 compare(const CharacterType *other) const noexcept
    {
        return strcmp(begin(), other);
    }


    Bool equals(const BasicString        &other) const noexcept
    {
        if (get_size() != other.get_size())
        {
            return false;
        }

        return strcmp(begin(), other.begin()) == 0;
    }

    Bool equals(const CharacterType *other) const noexcept
    {
        return strcmp(begin(), other) == 0;
    }


    Void insert(const UInt64 position, const BasicString        &other) noexcept
    {
        const UInt64 selfSize = size & ~SSO_FLAG;
        assert(position <= selfSize);

        const UInt64 otherSize = other.get_size();
        if (otherSize == 0)
        {
            return;
        }

        reserve(selfSize + otherSize);

        const UInt64 partSize = selfSize - position;
        CharacterType *partOldBegin = begin() + position;
        CharacterType *partNewBegin = partOldBegin + otherSize;
        memmove(partNewBegin, partOldBegin, partSize * sizeof(CharacterType));
        memcpy(partOldBegin, other.get_data(), otherSize * sizeof(CharacterType));
        size += otherSize;
        *end() = CharacterType();
    }

    Void insert(const UInt64 position, const CharacterType *other) noexcept
    {
        if (!other)
        {
            return;
        }

        const UInt64 selfSize = size & ~SSO_FLAG;
        assert(position <= selfSize);

        const UInt64 otherSize = BasicString::length(other);
        if (otherSize == 0)
        {
            return;
        }

        reserve(selfSize + otherSize);

        const UInt64 partSize = selfSize - position;
        CharacterType *partOldBegin = begin() + position;
        CharacterType *partNewBegin = partOldBegin + otherSize;
        memmove(partNewBegin, partOldBegin, partSize * sizeof(CharacterType));
        memcpy(partOldBegin, other, otherSize * sizeof(CharacterType));
        size += otherSize;
        *end() = CharacterType();
    }


    Void replace(const BasicString        &newSubstring, const BasicString        &oldSubstring) noexcept
    {
        const UInt64 oldSubstringSize = oldSubstring.get_size();

        if (oldSubstringSize == 0)
        {
            SPDLOG_WARN("Tried to replace empty substring.");
            return;
        }

        const UInt64 selfSize = get_size();
        const UInt64 newSubstringSize = newSubstring.get_size();
        const Int64 sizeDifference = newSubstringSize - oldSubstringSize;
        if (oldSubstringSize < newSubstringSize)
        {
            const UInt64 occurrences = find(oldSubstring, EStringFindStrategy::CountNonOverlapping);
            reserve(selfSize + occurrences * sizeDifference);
            size += occurrences * sizeDifference;
        }

        const CharacterType *oldSubstringData = oldSubstring.begin();
        const CharacterType *newSubstringData = newSubstring.get_data();
        UInt64 currentOffset = 0;
        for (CharacterType *selfData = begin(), *terminationElement = end() - oldSubstringSize - 1;
             selfData < terminationElement;
             ++selfData, ++currentOffset)
        {
            if (memcmp(selfData, oldSubstringData, oldSubstringSize * sizeof(CharacterType)) == 0)
            {
                currentOffset += oldSubstringSize;
                CharacterType *stringTail = selfData + oldSubstringSize;
                memmove(stringTail + sizeDifference, stringTail, (selfSize - currentOffset) * sizeof(CharacterType));
                memcpy(selfData, newSubstringData, newSubstringSize * sizeof(CharacterType));
                currentOffset += sizeDifference - 1;
                selfData += newSubstringSize - 1;
            }
        }
    }
    //
    // Void replace(const CharacterType *newSubstring, const CharacterType *oldSubstring) noexcept
    // {
    // }
    //
    // Void replace(const String        &newSubstring, const CharacterType *oldSubstring) noexcept
    // {
    // }
    //
    // Void replace(const CharacterType *newSubstring, const String        &oldSubstring) noexcept
    // {
    // }

    template<typename = Void>
    requires std::is_same_v<CharacterType, Char8>
    Void to_upper() noexcept
    {
        for (CharacterType *element = begin(); *element != Char8(); ++element)
        {
            if (*element >= Char8('a') && *element <= Char8('z'))
            {
                *element &= ~Char8(32);
            }
        }
    }

    template<typename = Void>
    requires std::is_same_v<CharacterType, Char8>
    Void to_lower() noexcept
    {
        for (CharacterType *element = begin(); *element != Char8(); ++element)
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
                UInt64 offset = 0;
                CharacterType *selfBegin = begin();
                for (CharacterType *element = selfBegin; BasicString::is_white_space(*element); ++element)
                {
                    ++offset;
                }
                size -= offset;
                memcpy(selfBegin, selfBegin + offset, size & ~SSO_FLAG);
                *end() = CharacterType();
                break;
            }
            case EStringTrimStrategy::End:
            {
                for (CharacterType *element = end() - 1; BasicString::is_white_space(*element); --element)
                {
                    --size;
                }
                *end() = CharacterType();
                break;
            }
            case EStringTrimStrategy::All:
            {
                UInt64 offset = 0;
                for (CharacterType *element = begin(); *element != CharacterType(); ++element)
                {
                    if (BasicString::is_white_space(*element))
                    {
                        ++offset;
                    } else {
                        *(element - offset) = *element;
                    }
                }
                size -= offset;
                *end() = CharacterType();
                break;
            }
        }
    }

    Void reverse() noexcept
    {
        CharacterType holder;
        CharacterType *beginElement = begin();
        CharacterType *endElement   = end() - 1;

        for (; beginElement < endElement; ++beginElement, --endElement)
        {
            std::swap(*beginElement, *endElement);
        }
    }

    Void remove(const UInt64 position, const UInt64 length) noexcept
    {
        const UInt64 selfSize = size & ~SSO_FLAG;
        assert(position + length < selfSize);

        CharacterType *elementsDestination = begin() + position;
        CharacterType *elementsToMove      = elementsDestination + length;
        const UInt64   elementsSize        = selfSize - (position + length);

        memcpy(elementsDestination, elementsToMove, elementsSize);
        size -= elementsSize;
        *end() = CharacterType();
    }

    Void remove(CharacterType character) noexcept
    {
        UInt64 offset = 0;
        for (CharacterType *element = begin(); *element != CharacterType(); ++element)
        {
            if (*element == character)
            {
                ++offset;
            } else {
                *(element - offset) = *element;
            }
        }
        size -= offset;
        *end() = CharacterType();
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
        if (!(source.size & SSO_FLAG))
        {
            memcpy(elements, source.elements, size * sizeof(CharacterType));
        } else {
            elements = source.elements;
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
    UInt64 get_size() const noexcept
    {
        return size & ~SSO_FLAG;
    }

    // UInt64 hash() const;

    CharacterType *get_data() noexcept
    {
        return size & SSO_FLAG ? smallText : elements;
    }

    const CharacterType *get_data() const noexcept
    {
        return size & SSO_FLAG ? smallText : elements;
    }


    CharacterType *begin() noexcept
    {
        return size & SSO_FLAG ? smallText : elements;
    }

    [[nodiscard]]
    const CharacterType *begin() const noexcept
    {
        return size & SSO_FLAG ? smallText : elements;
    }


    CharacterType *end() noexcept
    {
        return size & SSO_FLAG ? smallText[size & ~SSO_FLAG] : elements[size & ~SSO_FLAG];
    }

    [[nodiscard]]
    const CharacterType *end() const noexcept
    {
        return size & SSO_FLAG ? smallText[size & ~SSO_FLAG] : elements[size & ~SSO_FLAG];
    }

#pragma region OPERATORS

    CharacterType &operator[](UInt64 index) noexcept
    {
        assert(index < size & ~SSO_FLAG);
        return size & SSO_FLAG ? smallText[index] : elements[index];
    }

    CharacterType  operator[](UInt64 index) const noexcept
    {
        assert(index < size & ~SSO_FLAG);
        return size & SSO_FLAG ? smallText[index] : elements[index];
    }

    // String operator+(const String        &other) const;
    // String operator+(const CharacterType *other) const;
    // String operator+(      CharacterType  other) const;
    //
    // Void   operator+=(const String        &other);
    // Void   operator+=(const CharacterType *other);
    // Void   operator+=(      CharacterType  other);

    
    inline Bool operator!=(const BasicString &other) const noexcept 
    {
        return equals(other);
    }
    
    inline Bool operator==(const BasicString &other) const noexcept
    {
        return equals(other);
    }
    
    inline Bool operator< (const BasicString &other) const noexcept
    {
        return compare(other) == -1I8;
    }
    
    inline Bool operator<=(const BasicString &other) const noexcept
    {
        return compare(other) != 1I8;
    }
    
    inline Bool operator> (const BasicString &other) const noexcept
    {
        return compare(other) == 1I8;
    }
    
    inline Bool operator>=(const BasicString &other) const noexcept
    {
        return compare(other) != -1I8;
    }


    inline Bool operator!=(const CharacterType *other) const noexcept
    {
        return equals(other);
    }

    inline Bool operator==(const CharacterType *other) const noexcept
    {
        return equals(other);
    }

    inline Bool operator< (const CharacterType *other) const noexcept
    {
        return compare(other) == -1I8;
    }

    inline Bool operator<=(const CharacterType *other) const noexcept
    {
        return compare(other) != 1I8;
    }

    inline Bool operator> (const CharacterType *other) const noexcept
    {
        return compare(other) == 1I8;
    }

    inline Bool operator>=(const CharacterType *other) const noexcept
    {
        return compare(other) != -1I8;
    }

#pragma endregion 

    Void clear() noexcept
    {
        *begin() = CharacterType();
        size &= SSO_FLAG;
    }

    Void finalize() noexcept
    {
        if (size & SSO_FLAG)
        {
            clear();
            return;
        }

        if (!elements)
        {
            *this = {};
            return;
        }

        clear();
        allocatorInfo->allocate(allocatorInfo->allocator, capacity);
        *this = {};
    }

    // static String format(const CharacterType *fmt, ...);

    template <typename = std::enable_if_t<!std::is_same_v<std::nullptr_t, CharacterType>>>
    static constexpr UInt64 length(const CharacterType *text) noexcept
    {
        UInt64 length = 0;
        while (*text != CharacterType())
        {
            ++length;
            ++text;
        }

        return length;
    }

    static constexpr Bool is_white_space(CharacterType character)
    {
        if (character == CharacterType(0x20) || (character >= CharacterType(0x09) && character <= CharacterType(0x0D)))
        {
            return true;
        }

        if constexpr (!std::is_same_v<CharacterType, Char8> && !std::is_same_v<CharacterType, Char>)
        {
            return character >= CharacterType(0x00A0) && character <= CharacterType(0x3000) &&
                   (
                       character == CharacterType(0x00A0)                                         ||
                       character == CharacterType(0x1680)                                         ||
                       (character >= CharacterType(0x2000) && character <= CharacterType(0x200A)) ||
                       character == CharacterType(0x2028)                                         ||
                       character == CharacterType(0x2029)                                         ||
                       character == CharacterType(0x202F)                                         ||
                       character == CharacterType(0x205F)                                         ||
                       character == CharacterType(0x3000)
                   );
        }

        return false;
    }
};


template <typename CharacterType>
requires (std::is_same_v<CharacterType, Char> || std::is_same_v<CharacterType, Char8>)
std::ostream &operator<<(std::ostream &os, const BasicString<CharacterType> &string)
{
    if constexpr (std::is_same_v<CharacterType, Char>)
    {
        return os << string.get_data();
    } else {
        return os << reinterpret_cast<const Char *>(string.get_data());
    }
}

template <typename CharacterType>
requires (std::is_same_v<CharacterType, WChar > ||
          std::is_same_v<CharacterType, Char16> ||
          std::is_same_v<CharacterType, Char32>)
std::wostream &operator<<(std::wostream &os, const BasicString<CharacterType> &string)
{
    if constexpr (std::is_same_v<CharacterType, WChar>)
    {
        return os << string.get_data();
    } else if constexpr (std::is_same_v<CharacterType, Char16>)
    {
        if constexpr (sizeof(WChar) == 2)
        {
            return os << reinterpret_cast<const WChar *>(string.get_data());
        } else {
            std::wstring_convert<std::codecvt_utf8_utf16<Char16>, Char16> converter;
            return os << converter.to_bytes(string.get_data());
        }
    } else {
        if constexpr (sizeof(WChar) == 4)
        {
            return os << reinterpret_cast<const WChar *>(string.get_data());
        } else {
            std::wstring_convert<std::codecvt_utf16<Char32>, Char32> converter;
            return os << converter.to_bytes(string.get_data());
        }
    }
}

using String   = BasicString<Char  >;
using String8  = BasicString<Char8 >;
using WString  = BasicString<WChar >;
using String16 = BasicString<Char16>;
using String32 = BasicString<Char32>;