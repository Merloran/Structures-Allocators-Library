#pragma once
#include "Serrate/Utilities/types.hpp"

template<typename... Types>
class View;

template<>
class View<> {};

template<typename Type, typename... Types>
class View<Type, Types...> : View<Types...>
{
private:
    Type value;

public:
    View() noexcept = default;

    View(Type v, Types... vs) noexcept
    : View<Types...>(vs...)
    , value(v)
    {}

    template<typename RequestedType>
    RequestedType &get_value() noexcept
    {
        if constexpr (std::is_same_v<RequestedType, Type>)
        {
            return value;
        } else {
            return View<Types...>::template get_value<RequestedType>();
        }
    }

    template<typename RequestedType>
    [[nodiscard]]
    const RequestedType &get_value() const noexcept
    {
        if constexpr (std::is_same_v<RequestedType, Type>)
        {
            return value;
        } else {
            return View<Types...>::template get_value<RequestedType>();
        }
    }

    template<USize Index>
    auto &get() noexcept
    {
        if constexpr (Index == 0)
        {
            return value;
        } else {
            return View<Types...>::template get<Index - 1>();
        }
    }

    template<USize Index>
    [[nodiscard]]
    const auto& get() const noexcept
    {
        if constexpr (Index == 0)
        {
            return value;
        } else {
            return View<Types...>::template get<Index - 1>();
        }
    }
};

// Tuple specialization for structure binding
namespace std
{
    template<typename... Ts>
    struct tuple_size<View<Ts...>> : std::integral_constant<std::size_t, sizeof...(Ts)> {};

    template<std::size_t I, typename Head, typename... Tail>
    struct tuple_element<I, View<Head, Tail...>> : tuple_element<I - 1, View<Tail...>> {};

    template<typename Head, typename... Tail>
    struct tuple_element<0, View<Head, Tail...>>
    {
        using type = Head;
    };
}