#pragma once
// Rename to be consistent with naming_convention
using Bool	  = bool;
using Void	  = void;

using Char   = char;
using WChar  = wchar_t;
using Char8  = char8_t;
using Char16 = char16_t;
using Char32 = char32_t;
              
using Int8	  = int8_t;
using Int16	  = int16_t;
using Int32	  = int32_t;
using Int64	  = int64_t;
              
using UInt8	  = uint8_t;
using UInt16  = uint16_t;
using UInt32  = uint32_t;
using UInt64  = uint64_t;

using USize   = size_t;

using Float32 = float;
using Float64 = double;

template <typename Type>
concept Manual =
std::is_trivially_copyable_v<Type> &&
std::is_nothrow_default_constructible_v<Type>;

template <typename Type>
concept Copyable =
requires(Type element, const Type &other) { element.copy(other); };

template <typename Type>
concept Moveable =
requires(Type element, Type &other) { element.move(other); };

template <typename Type>
concept Finalizable =
requires(Type element) { element.finalize(); };