#ifndef NAMED_TYPE_FXD_HPP
#define NAMED_TYPE_FXD_HPP

#include <ratio>

namespace fluent
{
    
template<typename T, typename Ratio>
struct ConvertWithRatio
{
    static T convertFrom(T t) { return t * Ratio::den / Ratio::num; }
    static T convertTo(T t) { return t * Ratio::num / Ratio::den; }
};

template <typename T, typename Parameter, typename Converter, template<typename> class... Skills>
class NamedTypeImpl;

template <typename T, typename Parameter, template<typename> class... Skills>
using NamedType = NamedTypeImpl<T, Parameter, ConvertWithRatio<T, std::ratio<1>>, Skills...>;

} // namespace fluent

#endif
