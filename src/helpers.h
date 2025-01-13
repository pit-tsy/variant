#pragma once

#include <cstddef>
#include <exception>
#include <type_traits>

template <typename...>
class variant;

inline constexpr std::size_t variant_npos = static_cast<std::size_t>(-1);

class bad_variant_access : public std::exception {
public:
  bad_variant_access() noexcept
      : msg_("bad variant access") {}

  bad_variant_access(const bad_variant_access& other) noexcept
      : msg_(other.msg_) {}

  bad_variant_access(const char* msg) noexcept
      : msg_(msg) {}

  const char* what() const noexcept override {
    return msg_;
  }

private:
  const char* msg_;
};

template <class T>
struct in_place_type_t {
  explicit in_place_type_t() = default;
};

template <class T>
constexpr in_place_type_t<T> in_place_type{};

template <std::size_t I>
struct in_place_index_t {
  explicit in_place_index_t() = default;
};

template <std::size_t I>
inline constexpr in_place_index_t<I> in_place_index{};

template <std::size_t I, class T>
struct variant_alternative; /* undefined */

template <size_t I, class T>
using variant_alternative_t = typename variant_alternative<I, T>::type;

template <std::size_t I, class Head, class... Rest>
struct variant_alternative<I, variant<Head, Rest...>> {
  using type = variant_alternative_t<I - 1, variant<Rest...>>;
};

template <class Head, class... Rest>
struct variant_alternative<0, variant<Head, Rest...>> {
  using type = Head;
};

template <std::size_t I>
struct variant_alternative<I, variant<>> {
  using type = void;
};

template <std::size_t I, class T>
struct variant_alternative<I, const T> {
  using type = std::add_const_t<variant_alternative_t<I, T>>;
};

template <class T>
struct variant_size {};

template <class T>
constexpr std::size_t variant_size_v = variant_size<std::remove_reference_t<T>>::value;

template <class T>
struct variant_size<const T> : std::integral_constant<std::size_t, variant_size_v<T>> {};

template <class... Types>
struct variant_size<variant<Types...>> : std::integral_constant<std::size_t, sizeof...(Types)> {};
