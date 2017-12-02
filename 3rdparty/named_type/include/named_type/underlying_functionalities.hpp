#ifndef UNDERLYING_FUNCTIONALITIES_HPP
#define UNDERLYING_FUNCTIONALITIES_HPP

#include "crtp.hpp"
#include "named_type.fwd.hpp"

#include <iostream>
#include <memory>

namespace fluent
{

template <typename T>
struct Incrementable : crtp<T, Incrementable>
{
    T& operator+=(T const& other) { this->underlying().get() += other.get(); return this->underlying(); }
    T& operator++() { ++(this->underlying().get()); return this->underlying(); }
};

template <typename T>
struct Addable : crtp<T, Addable>
{
    T operator+(T const& other) const { return T(this->underlying().get() + other.get()); }
};

template <typename T>
struct Decrementable : crtp<T, Decrementable>
{
    T& operator-=(T const& other) { this->underlying().get() -= other.get(); return this->underlying(); }
    T& operator--() { --(this->underlying().get()); return this->underlying(); }
};

template <typename T>
struct Subtractable : crtp<T, Subtractable>
{
    T operator-(T const& other) const { return T(this->underlying().get() - other.get()); }
};
    
template <typename T>
struct Multiplicable : crtp<T, Multiplicable>
{
    T operator*(T const& other) const { return T(this->underlying().get() * other.get()); }
};
    
template <typename T>
struct Comparable : crtp<T, Comparable>
{
    bool operator<(T const& other) const  { return this->underlying().get() < other.get(); }
    bool operator>(T const& other) const  { return other.get() < this->underlying().get(); }
    bool operator<=(T const& other) const { return !(other.get() < this->underlying().get());}
    bool operator>=(T const& other) const { return !(*this < other); }
    bool operator==(T const& other) const { return !(*this < other) && !(other.get() < this->underlying().get()); }
    bool operator!=(T const& other) const { return !(*this == other); }
};

template <typename T>
struct Printable : crtp<T, Printable>
{
    template<typename Stream>
    void print(Stream& os) const { os << this->underlying().get(); }
};

template <typename Destination>
struct ImplicitlyConvertibleTo
{
    template <typename T>
    struct templ : crtp<T, templ>
    {
        operator Destination() const
        {
            return this->underlying().get();
        }
    };
};

template <typename T, typename Parameter, template<typename> class... Skills>
std::ostream& operator<<(std::ostream& os, NamedType<T, Parameter, Skills...> const& object)
{
    object.print(os);
    return os;
}

template<typename T>
struct Hashable
{
    static constexpr bool is_hashable = true;
};

template<typename NamedType_>
struct FunctionCallable;
    
template <typename T, typename Parameter, template<typename> class... Skills>
struct FunctionCallable<NamedType<T, Parameter, Skills...>> : crtp<NamedType<T, Parameter, Skills...>, FunctionCallable>
{
    operator T const&() const
    {
        return this->underlying().get();
    }
    operator T&()
    {
        return this->underlying().get();
    }
};
    
template<typename NamedType_>
struct MethodCallable;
    
template <typename T, typename Parameter, template<typename> class... Skills>
struct MethodCallable<NamedType<T, Parameter, Skills...>> : crtp<NamedType<T, Parameter, Skills...>, MethodCallable>
{
    T const* operator->() const { return std::addressof(this->underlying().get()); }
    T* operator->() { return std::addressof(this->underlying().get()); }
};

template<typename NamedType_>
struct Callable : FunctionCallable<NamedType_>, MethodCallable<NamedType_>{};
    
} // namespace fluent

namespace std
{
template <typename T, typename Parameter, typename Converter, template<typename> class... Skills>
struct hash<fluent::NamedTypeImpl<T, Parameter, Converter, Skills...>>
{
    using NamedType = fluent::NamedTypeImpl<T, Parameter, Converter, Skills...>;
    using checkIfHashable = typename std::enable_if<NamedType::is_hashable, void>::type;
    
    size_t operator()(fluent::NamedTypeImpl<T, Parameter, Converter, Skills...> const& x) const
    {
        return std::hash<T>()(x.get());
    }
};

}
    

#endif
