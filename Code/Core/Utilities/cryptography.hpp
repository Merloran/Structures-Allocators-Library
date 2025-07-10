#pragma once
#include <xxhash.h>

namespace Cryptography
{
    template <typename Type>
    UInt64 hash(Type value)
    requires std::is_arithmetic_v<Type> || std::is_enum_v<Type> || std::is_pointer_v<Type>
    {
        // if constexpr (std::is_pointer_v<Type>)
        // {
        //     return XXH3_64bits(value, sizeof(*value));
        // } else {
        //     return XXH3_64bits(&value, sizeof(Type));
        // }
        return USize(value);
    }

    template <typename Type>
    UInt64 hash(Type value, const USize count)
    requires std::is_arithmetic_v<Type> || std::is_enum_v<Type> || std::is_pointer_v<Type>
    {
        if constexpr (std::is_pointer_v<Type>)
        {
            return XXH3_64bits(value, sizeof(*value) * count);
        } else {
            return XXH3_64bits(&value, sizeof(Type) * count);
        }
    }
} 