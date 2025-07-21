#pragma once

template <typename Type>
concept Character =
std::is_same_v<Type, Char  > ||
std::is_same_v<Type, Char8 > ||
std::is_same_v<Type, WChar > ||
std::is_same_v<Type, Char16> ||
std::is_same_v<Type, Char32>;


template <Character Type>
class BasicStringView
{
private:
    USize size;
    const Type *elements;

public:
    constexpr BasicStringView()
    : size(0)
    , elements(nullptr)
    {}

    constexpr BasicStringView(const Type *text)
    {
        initialize(text);
    }

    template <USize Count>
    constexpr BasicStringView(const Type text[Count])
    {
        initialize(text);
    }

    constexpr Void initialize(const Type *text) noexcept
    {
        assert(text && "Invalid pointer!");

        size = BasicStringView::length(text);
        elements = text;
    }

    constexpr Void initialize(const Type *text, const USize textEnd) noexcept
    {
        assert(text && "Invalid pointer!");
        assert(textEnd <= BasicStringView::length(text) && "End cannot exceed text length");

        size = textEnd;
        elements = text;
    }

    template <USize Count>
    requires Count > 0
    constexpr Void initialize(const Type text[Count]) noexcept
    {
        size = Count - 1;
        elements = text;
    }

    template <USize Count>
    requires Count > 0
    constexpr Void initialize(const Type text[Count], const USize textEnd) noexcept
    {
        assert(textEnd < Count && "End cannot exceed text length");
        size = textEnd;
        elements = text;
    }

    [[nodiscard]]
    const Type *begin() const noexcept
    {
        return elements;
    }

    [[nodiscard]]
    const Type *end() const noexcept
    {
        return elements + size;
    }

    [[nodiscard]]
    const Type *get_data() const noexcept
    {
        return elements;
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

    Type operator[](const USize index) const noexcept
    {
        assert(index < size);
        return elements[index];
    }

    UInt64 hash() const noexcept
    {
        return Cryptography::hash(elements, size);
    }

    static constexpr USize length(const Type *text) noexcept
    requires !std::is_same_v<std::nullptr_t, Type>
    {
        if constexpr (std::is_same_v<Type, Char> || std::is_same_v<Type, Char8>)
        {
            return __builtin_strlen(reinterpret_cast<const Char *>(text));
        }
        else if constexpr (std::is_same_v<Type, WChar>)
        {
            return __builtin_wcslen(text);
        }
        else if constexpr (std::is_same_v<Type, Char16> && sizeof(WChar) == 2)
        {
            return __builtin_wcslen(reinterpret_cast<const WChar *>(text));
        }
        else if constexpr (std::is_same_v<Type, Char32> && sizeof(WChar) == 4)
        {
            return __builtin_wcslen(reinterpret_cast<const WChar *>(text));
        }
        else {
            USize length = 0;
            while (*text != Type())
            {
                ++length;
                ++text;
            }

            return length;
        }
    }

    static constexpr Bool is_white_space(Type character)
    {
        if (character == Type(0x20) || (character >= Type(0x09) && character <= Type(0x0D)))
        {
            return true;
        }

        if constexpr (!std::is_same_v<Type, Char8> && !std::is_same_v<Type, Char>)
        {
            return character >= Type(0x00A0) && character <= Type(0x3000) &&
                    (
                    character == Type(0x00A0) ||
                    character == Type(0x1680) ||
                    (character >= Type(0x2000) && character <= Type(0x200A)) ||
                    character == Type(0x2028) ||
                    character == Type(0x2029) ||
                    character == Type(0x202F) ||
                    character == Type(0x205F) ||
                    character == Type(0x3000)
                    );
        }

        return false;
    }
};

using StringView = BasicStringView<Char>;