#ifndef NAMED_TYPE_HPP
#define NAMED_TYPE_HPP

#include <functional>
#include <ratio>
#include <type_traits>
#include "underlying_functionalities.hpp"

namespace fluent
{
    
template<typename T, T (*from)(T), T(*to)(T)>
struct Convert
{
    static T convertFrom(T t) { return from(t); }
    static T convertTo(T t) { return to(t); }
};
    
template<typename T>
using IsNotReference = typename std::enable_if<!std::is_reference<T>::value, void>::type;

template <typename T, typename Parameter, typename Converter, template<typename> class... Skills>
class NamedTypeImpl : public Skills<NamedTypeImpl<T, Parameter, Converter, Skills...>>...
{
public:
    using UnderlyingType = T;

    // constructor
    NamedTypeImpl() : value_{} {}
    explicit NamedTypeImpl(T const& value) : value_(value) {}
    template<typename T_ = T, typename = IsNotReference<T_>>
    explicit NamedTypeImpl(T&& value) : value_(std::move(value)) {}

    // get
    T& get() { return value_; }
    T const& get() const {return value_; }

    // conversions
    template <typename Converter2>
    operator NamedTypeImpl<T, Parameter, Converter2, Skills...>() const
    {
        return NamedTypeImpl<T, Parameter, Converter2, Skills...>(Converter2::convertFrom(Converter::convertTo(get())));
    }
    
    struct conversions
    {
        template<T(*f)(T), T(*g)(T)>
        struct compose
        {
            static T func(T t) { return f(g(t)); }
        };
        template<typename Converter1, typename Converter2>
        using ComposeConverter = Convert<T, compose<Converter1::convertFrom, Converter2::convertFrom>::func, compose<Converter1::convertTo, Converter2::convertTo>::func>;
        template <typename Converter2>
        using GetConvertible = NamedTypeImpl<T, Parameter, ComposeConverter<Converter, Converter2>, Skills...>;
    };

private:
    T value_;
};

template <typename T, typename Parameter, template<typename> class... Skills>
using NamedType = NamedTypeImpl<T, Parameter, ConvertWithRatio<T, std::ratio<1>>, Skills...>;

template <typename StrongType, typename Ratio>
using MultipleOf = typename StrongType::conversions::template GetConvertible<ConvertWithRatio<typename StrongType::UnderlyingType, Ratio>>;

template <typename StrongType, typename Converter>
using ConvertibleTo = typename StrongType::conversions::template GetConvertible<Converter>;

template<template<typename T> class StrongType, typename T>
StrongType<T> make_named(T const& value)
{
    return StrongType<T>(value);
}
    
} // namespace fluent

#endif
