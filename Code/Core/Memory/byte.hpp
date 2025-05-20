#pragma once
//TODO: Think of linux support
#if defined(_WIN32)
#include <windows.h>
static constexpr USize GET_PAGE_SIZE()
{
    return 4096;
}

constexpr USize align_memory(const USize bytes)
{
    return (bytes + GET_PAGE_SIZE() - 1) & ~(GET_PAGE_SIZE() - 1);
}
#endif

template<typename Type>
concept Integral = std::is_integral_v<Type>;

enum class Byte : UInt8 {};

#pragma region OPERATORS
template <Integral Type>
[[nodiscard]]
constexpr Byte operator<<(const Byte element, const Type shift) noexcept
{
    return static_cast<Byte>(static_cast<UInt8>(static_cast<UInt8>(element) << shift));
}

template <Integral Type>
[[nodiscard]]
constexpr Byte operator>>(const Byte element, const Type shift) noexcept
{
    return static_cast<Byte>(static_cast<UInt8>(static_cast<UInt8>(element) >> shift));
}

[[nodiscard]]
constexpr Byte operator|(const Byte element1, const Byte element2) noexcept
{
    return static_cast<Byte>(static_cast<UInt8>(static_cast<UInt8>(element1) | static_cast<UInt8>(element2)));
}

[[nodiscard]]
constexpr Byte operator&(const Byte element1, const Byte element2) noexcept
{
    return static_cast<Byte>(static_cast<UInt8>(static_cast<UInt8>(element1) & static_cast<UInt8>(element2)));
}

[[nodiscard]]
constexpr Byte operator^(const Byte element1, const Byte element2) noexcept
{
    return static_cast<Byte>(static_cast<UInt8>(static_cast<UInt8>(element1) ^ static_cast<UInt8>(element2)));
}

[[nodiscard]]
constexpr Byte operator~(const Byte element) noexcept
{
    return static_cast<Byte>(static_cast<UInt8>(~static_cast<UInt8>(element)));
}

template <Integral Type>
constexpr Byte &operator<<=(Byte &element, const Type shift) noexcept
{
    return element = element << shift;
}

template <Integral Type>
constexpr Byte &operator>>=(Byte &element, const Type shift) noexcept
{
    return element = element >> shift;
}

constexpr Byte &operator|=(Byte &element1, const Byte element2) noexcept
{
    return element1 = element1 | element2;
}

constexpr Byte &operator&=(Byte &element1, const Byte element2) noexcept
{
    return element1 = element1 & element2;
}

constexpr Byte &operator^=(Byte &element1, const Byte element2) noexcept
{
    return element1 = element1 ^ element2;
}
#pragma endregion

template <typename Type>
constexpr Byte *byte_cast(Type *element) noexcept
{
    return reinterpret_cast<Byte *>(element);
}

template <typename Type>
constexpr const Byte *byte_cast(const Type *element) noexcept
{
    return reinterpret_cast<const Byte *>(element);
}

inline Byte *byte_cast(const USize element) noexcept
{
    return reinterpret_cast<Byte *>(element);
}

#pragma region LITERALS
constexpr Byte operator""_B(const USize value)
{
    return Byte(value);
}

constexpr USize operator""_KiB(const USize value)
{
    return value * USize(1024);
}

constexpr USize operator""_MiB(const USize value)
{
    return value * 1024_KiB;
}

constexpr USize operator""_GiB(const USize value)
{
    return value * 1024_MiB;
}

constexpr USize operator""_TiB(const USize value)
{
    return value * 1024_GiB;
}
#pragma endregion