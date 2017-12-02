A **strong type** is a type used in place of another type to carry specific **meaning** through its **name**.

This project experiments with strong types in C++. All components are in the namespace `fluent`. You can find a collection of blog posts explaining the rationale of the library and usages for strong types on [Fluent C++](https://www.fluentcpp.com/2016/12/08/strong-types-for-strong-interfaces/).

## Basic usage

It central piece is the templated class NamedType, which can be used to declare a strong type with a typedef-like syntax:

```cpp
using Width = NamedType<double, struct WidthTag>;
using Height = NamedType<double, struct HeightTag>;
```

which can be used to make interfaces more expressive and more robust.
Note how the below constructor shows in which order it expects its parameters:

```cpp
class Rectangle
{
public:
    Rectangle(Width width, Height height) : width_(width.get()), height_(height.get()) {}
    double getWidth() const { return width_; }
    double getHeight() const { return height_; }

private:
    double width_;
    double height_;
};
```

**Strong types are about better expressing your intentions, both to the compiler and to other human developers.**

## Strong typing over generic types

This implementation of strong types can be used to add strong typing over generic or unknown types such as lambdas:

```cpp
template<typename Function>
using Comparator = NamedType<Function, struct ComparatorTag>;

template <typename Function>
void performAction(Comparator<Function> comp)
{
    comp.get()();
}

performAction(make_named<Comparator>([](){ std::cout << "compare\n"; }));
```

## Strong typing over references

The NamedType class is designed so that the following usage:

```cpp
using FamilyNameRef = NamedType<std:string&, struct FamilyNameRefTag>;
```

behaves like a reference on an std::string, strongly typed.

## Inheriting the underlying type functionalities

You can declare which functionalities should be inherited from the underlying type. So far, only basic operators are taken into account.
For instance, to inherit from operator+ and operator<<, you can declare the strong type:

```cpp
using Meter = NamedType<double, MeterTag, Addable, Printable>
```

There is one special skill, `FunctionCallable`, that lets the strong type be converted in the underlying type. This has the effect of removing the need to call .get() to get the underlying value. And `MethodCallable` enables `operator->` on the strong type to invoke methods on the underlying type.

The skill `Callable` is the union of `FunctionCallable` and `MethodCallable`.


## Strong type conversions

Especially relevant for units, you can declare a strong type being a multiple of another one, with any std::ratio:

```cpp
using Meter = NamedType<double, MeterTag, Addable, Printable>
using KiloMeter = MultipleOf<Meter, std::kilo>
```

Or with a non-linear conversion:

```cpp
using Watt = NamedType<double, struct WattTag>;

struct ConvertDBFromAndToWatt
{
    static double convertFrom(double watt) { return 10 * log(watt) / log(10); }
    static double convertTo(double db) { return pow(10, db / 10); }
};

using dB = ConvertibleTo<Watt, ConvertDBFromAndToWatt>;
```

You can have a look at main.cpp for usage examples.
