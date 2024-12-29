#pragma once

#include <cstddef>
#include <exception>
#include <utility>

template <class... Types>
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

template <class T, T v>
struct integral_constant {
  static constexpr T value = v;
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

namespace detail {

template <typename T, template <typename...> typename Class>
struct is_specialization_of {
  static constexpr bool value = false;
};

template <template <typename...> typename Class, typename... Args>
struct is_specialization_of<Class<Args...>, Class> {
  static constexpr bool value = true;
};

template <typename T, template <typename...> typename Class>
inline constexpr bool is_specialization_of_v = is_specialization_of<T, Class>::value;

template <typename T, template <std::size_t...> typename Class>
struct is_numeric_specialization_of {
  static constexpr bool value = false;
};

template <template <std::size_t...> typename Class, std::size_t... Args>
struct is_numeric_specialization_of<Class<Args...>, Class> {
  static constexpr bool value = true;
};

template <typename T, template <std::size_t...> typename Class>
inline constexpr bool is_numeric_specialization_of_v = is_numeric_specialization_of<T, Class>::value;

template <std::size_t N, class... Types>
struct nth;

template <std::size_t N, typename First, typename... Rest>
struct nth<N, First, Rest...> {
  using type = typename nth<N - 1, Rest...>::type;
};

template <typename First, typename... Rest>
struct nth<0, First, Rest...> {
  using type = First;
};

template <std::size_t N, class... Types>
using nth_t = typename nth<N, Types...>::type;

template <std::size_t I>
struct index {};

template <std::size_t... Indices>
struct index_sequence_t {};

template <std::size_t... Indices>
inline constexpr index_sequence_t<Indices...> index_sequence{};

template <std::size_t N, std::size_t... Indices>
struct nth_index_t;

template <std::size_t N, std::size_t... Indices>
inline constexpr std::size_t nth_index = nth_index_t<N, Indices...>::value;

template <std::size_t N, std::size_t first_index, std::size_t... rest_indices>
struct nth_index_t<N, first_index, rest_indices...> {
  static constexpr std::size_t value = nth_index<N - 1, rest_indices...>;
};

template <std::size_t first_index, std::size_t... rest_indices>
struct nth_index_t<0, first_index, rest_indices...> {
  static constexpr std::size_t value = first_index;
};

template <std::size_t N, std::size_t... Indices>
constexpr std::size_t get_index(index_sequence_t<Indices...>) {
  return nth_index<N, Indices...>;
}

template <typename Seq, std::size_t I>
struct concat {};

template <std::size_t... Indices, std::size_t I>
struct concat<index_sequence_t<Indices...>, I> {
  using type = index_sequence_t<Indices..., I>;
};

template <std::size_t N>
struct make_index_sequence;

template <std::size_t N>
using make_index_sequence_t = typename make_index_sequence<N>::type;

template <std::size_t N>
struct make_index_sequence {
  using type = typename concat<make_index_sequence_t<N - 1>, N - 1>::type;
};

template <>
struct make_index_sequence<0> {
  using type = index_sequence_t<>;
};

template <typename T>
struct wrapped_value {
  T value;

  template <typename... Args>
  constexpr explicit wrapped_value(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
    requires std::is_constructible_v<T, Args...>
      : value(std::forward<Args>(args)...) {}

  constexpr ~wrapped_value() = default;
};

template <typename... Types>
union variadic_union {
  constexpr variadic_union() = default;

  constexpr ~variadic_union() = default;
};

template <typename Head, typename... Rest>
union variadic_union<Head, Rest...> {
  constexpr variadic_union()
      : rest() {}

  constexpr ~variadic_union() noexcept
    requires (std::is_trivially_destructible_v<Head> && (std::is_trivially_destructible_v<Rest> && ...))
  = default;

  constexpr ~variadic_union() {}

  constexpr void destroy() {
    std::destroy_at(std::addressof(head));
  }

  template <typename... Args>
  constexpr Head& emplace(Args&&... args) {
    return *std::construct_at(std::addressof(head), std::forward<Args>(args)...);
  }

  constexpr Head& get() {
    return head;
  }

  Head head;
  variadic_union<Rest...> rest;
};

template <std::size_t I, typename Union>
constexpr decltype(auto) union_i(Union& u) {
  if constexpr (I == 0) {
    return u;
  } else {
    return union_i<I - 1>(u.rest);
  }
}

template <std::size_t I, typename Union, typename... Args>
constexpr decltype(auto) emplace(Union& u, Args&&... args) {
  return union_i<I>(u).emplace(std::forward<Args>(args)...);
}

template <std::size_t I, typename Union>
constexpr void destroy(Union& u) {
  return union_i<I>(u).destroy();
}

template <std::size_t I, typename... Types>
constexpr nth_t<I, Types...>& get(variadic_union<Types...>& u) {
  return union_i<I>(u).head;
}

template <std::size_t I, typename Union>
constexpr decltype(auto) get(Union&& u) {
  return std::move(union_i<I>(u).head);
}

template <typename T, std::size_t... sizes>
struct multi_array {
  constexpr T& get() {
    return data;
  }

  constexpr const T& get() const {
    return data;
  }

  T data;
};

template <typename T, std::size_t first_size, std::size_t... rest_sizes>
struct multi_array<T, first_size, rest_sizes...> {
  using NextArrayType = multi_array<T, rest_sizes...>;

  NextArrayType array[first_size];

  template <typename... Args>
  constexpr T& get(std::size_t first_index, Args... rest_indices) {
    return array[first_index].get(rest_indices...);
  }

  template <typename... Args>
  constexpr const T& get(std::size_t first_index, Args... rest_indices) const {
    return array[first_index].get(rest_indices...);
  }

  constexpr NextArrayType& operator[](std::size_t index) {
    return array[index];
  }
};

template <bool transfer_indices, typename ArrayType, typename IndexSeq>
struct make_vtable_impl;

template <
    bool transfer_indices,
    typename RetType,
    typename Vis,
    typename... Variants,
    std::size_t... sizes,
    std::size_t... indices>
  requires (sizeof...(sizes) > 0)
struct make_vtable_impl<
    transfer_indices,
    multi_array<RetType (*)(Vis, Variants...), sizes...>,
    index_sequence_t<indices...>> {
  using T = RetType (*)(Vis, Variants...);
  using CurrentArrayType = multi_array<T, sizes...>;

  using Current = nth_t<sizeof...(indices), Variants...>;

  static constexpr CurrentArrayType make() {
    CurrentArrayType result{};
    make_all_alt(result, make_index_sequence_t<variant_size_v<Current>>{});
    return result;
  }

  template <std::size_t... current_indices_>
  static constexpr void make_all_alt(CurrentArrayType& array, index_sequence_t<current_indices_...>) {
    (make_alt(array[current_indices_], index<current_indices_>{}), ...);
  }

  template <typename NextArrayType, std::size_t index_>
  static constexpr void make_alt(NextArrayType& array, index<index_>) {
    array = make_vtable_impl<transfer_indices, NextArrayType, index_sequence_t<indices..., index_>>::make();
  }
};

template <bool transfer_indices, typename RetType, typename Vis, typename... Variants, std::size_t... indices>
struct make_vtable_impl<transfer_indices, multi_array<RetType (*)(Vis, Variants...)>, index_sequence_t<indices...>> {
  using T = RetType (*)(Vis, Variants...);
  using CurrentArrayType = multi_array<T>;

  static constexpr RetType invoke(Vis vis, Variants... variants) {
    if constexpr (transfer_indices) {
      return std::forward<Vis>(vis)(get<indices>(std::forward<Variants>(variants))..., index_sequence<indices...>);
    } else {
      return std::forward<Vis>(vis)(get<indices>(std::forward<Variants>(variants))...);
    }
  }

  static constexpr CurrentArrayType make() {
    return CurrentArrayType{&invoke};
  }
};

template <bool transfer_indices, typename RetType, typename Vis, typename... Variants>
struct make_vtable
    : make_vtable_impl<
          transfer_indices,
          multi_array<RetType (*)(Vis, Variants...), variant_size_v<Variants>...>,
          index_sequence_t<>> {};

template <typename Ti>
struct Arr_ {
  Ti arr[1];
};

template <size_t Ind, typename T, typename Ti, typename = void>
struct try_type {
  void try_it() = delete;
};

template <size_t Ind, typename T, typename Ti>
struct try_type<Ind, T, Ti, std::void_t<decltype(Arr_<Ti>{{std::declval<T>()}})>> {
  static integral_constant<size_t, Ind> try_it(Ti);
};

template <typename T, typename Variant, typename = make_index_sequence_t<variant_size_v<Variant>>>
struct try_types;

template <typename T, typename... Ti, size_t... Ind>
struct try_types<T, variant<Ti...>, index_sequence_t<Ind...>> : try_type<Ind, T, Ti>... {
  using try_type<Ind, T, Ti>::try_it...;
};

template <typename T, typename Variant>
using select_index = decltype(try_types<T, Variant>::try_it(std::declval<T>()));

template <typename T, typename Variant, typename = void>
inline constexpr size_t select_index_v = variant_npos;

template <typename T, typename Variant>
inline constexpr size_t select_index_v<T, Variant, std::void_t<select_index<T, Variant>>> =
    select_index<T, Variant>::value;

template <typename T, typename Variant>
using select_type_t = variant_alternative_t<select_index_v<T, Variant>, Variant>;

template <std::size_t I, typename T, typename... Types>
struct find_type_impl;

template <typename T, typename... Types>
using find_type = find_type_impl<0, T, Types...>;

template <typename T, typename... Types>
inline constexpr std::size_t find_type_v = find_type<T, Types...>::value;

template <std::size_t I, typename T, typename First, typename... Rest>
struct find_type_impl<I, T, First, Rest...> {
  static constexpr std::size_t value = find_type_impl<I + 1, T, Rest...>::value;
};

template <std::size_t I, typename T, typename... Rest>
struct find_type_impl<I, T, T, Rest...> {
  static constexpr std::size_t value = I;
};

template <std::size_t I, typename T>
struct find_type_impl<I, T> {
  static constexpr std::size_t value = variant_npos;
};

template <class R, bool transfer_indices = false, class Visitor, class... Variants>
constexpr R do_visit(Visitor&& vis, Variants&&... vars) {
  if ((vars.valueless_by_exception() || ...)) {
    throw bad_variant_access("variant is valueless");
  }

  constexpr auto table = make_vtable<transfer_indices, R, Visitor&&, Variants&&...>::make();

  return (*(table.get(vars.index()...)))(std::forward<Visitor>(vis), std::forward<Variants>(vars)...);
}

template <typename T, typename... Types>
struct exactly_one;

template <typename T, typename... Types>
inline constexpr bool exactly_one_v = exactly_one<T, Types...>::value;

template <typename T, typename... Rest>
struct exactly_one<T, T, Rest...> {
  static constexpr bool value = (find_type_v<T, Rest...> == variant_npos);
};

template <typename T, typename First, typename... Rest>
struct exactly_one<T, First, Rest...> {
  static constexpr bool value = exactly_one_v<T, Rest...>;
};

template <typename T>
struct exactly_one<T> {
  static constexpr bool value = false;
};

template <typename... Types>
inline constexpr bool is_trivially_copy_assignable_v =
    ((std::is_trivially_copy_constructible_v<Types> && std::is_trivially_copy_assignable_v<Types> &&
      std::is_trivially_destructible_v<Types>) &&
     ...);

template <typename... Types>
inline constexpr bool is_copy_assignable_v =
    ((std::is_copy_constructible_v<Types> && std::is_copy_assignable_v<Types>) && ...);

template <typename... Types>
inline constexpr bool is_move_assignable_v =
    ((std::is_move_constructible_v<Types> && std::is_move_assignable_v<Types>) && ...);

template <typename... Types>
inline constexpr bool is_trivially_move_assignable_v =
    ((std::is_trivially_move_constructible_v<Types> && std::is_trivially_move_assignable_v<Types> &&
      std::is_trivially_destructible_v<Types>) &&
     ...);

template <size_t I, typename Variant, typename T = variant_alternative_t<I, std::remove_reference_t<Variant>>>
using get_t = std::conditional_t<std::is_lvalue_reference_v<Variant>, T&, T&&>;

template <typename Visitor, typename... Variants>
using ret_type_visitor = decltype(std::declval<Visitor>()(std::declval<get_t<0, Variants>>()...));

} // namespace detail

template <class T, class... Types>
constexpr bool holds_alternative(const variant<Types...>& v) noexcept {
  return !v.valueless_by_exception() && detail::find_type_v<T, Types...> == v.index();
}

template <std::size_t I, class... Types>
constexpr variant_alternative_t<I, variant<Types...>>& get(variant<Types...>& v) {
  if (I != v.index()) {
    throw bad_variant_access();
  } else {
    return detail::get<I>(v.union_).value;
  }
}

template <std::size_t I, class... Types>
constexpr variant_alternative_t<I, variant<Types...>>&& get(variant<Types...>&& v) {
  if (I != v.index()) {
    throw bad_variant_access();
  } else {
    return detail::get<I>(std::move(v.union_)).value;
  }
}

template <std::size_t I, class... Types>
constexpr const variant_alternative_t<I, variant<Types...>>& get(const variant<Types...>& v) {
  return get<I>(const_cast<variant<Types...>&>(v));
}

template <std::size_t I, class... Types>
constexpr const variant_alternative_t<I, variant<Types...>>&& get(const variant<Types...>&& v) {
  return get<I>(const_cast<variant<Types...>&&>(v));
}

template <class T, class... Types>
constexpr T& get(variant<Types...>& v) {
  if constexpr (detail::find_type_v<T, Types...> == variant_npos) {
    throw bad_variant_access();
  } else {
    return get<detail::find_type_v<T, Types...>>(v);
  }
}

template <class T, class... Types>
constexpr T&& get(variant<Types...>&& v) {
  if constexpr (detail::find_type_v<T, Types...> == variant_npos) {
    throw bad_variant_access();
  } else {
    return get<detail::find_type_v<T, Types...>>(std::move(v));
  }
}

template <class T, class... Types>
constexpr const T& get(const variant<Types...>& v) {
  return get<T>(const_cast<variant<Types...>&>(v));
}

template <class T, class... Types>
constexpr const T&& get(const variant<Types...>&& v) {
  return get<T>(const_cast<variant<Types...>&&>(std::move(v)));
}

template <class Visitor, class... Variants, typename R = detail::ret_type_visitor<Visitor, Variants...>>
constexpr R visit(Visitor&& vis, Variants&&... vars) {
  return visit<R>(std::forward<Visitor>(vis), std::forward<Variants>(vars)...);
}

template <class R, class Visitor, class... Variants>
constexpr R visit(Visitor&& vis, Variants&&... vars) {
  return detail::do_visit<R>(std::forward<Visitor>(vis), std::forward<Variants>(vars)...);
}

template <class... Types>
class variant {
public:
  constexpr variant() noexcept(std::is_nothrow_default_constructible_v<detail::nth_t<0, Types...>>)
    requires (std::is_default_constructible_v<detail::nth_t<0, Types...>>)
      : index_(0) {
    union_.emplace();
  }

  constexpr variant(const variant&) noexcept
    requires (std::is_trivially_copy_constructible_v<Types> && ...)
  = default;

  constexpr variant(const variant& other)
    requires ((std::is_copy_constructible_v<Types> && ...) && (!std::is_trivially_copy_constructible_v<Types> || ...))
      : index_(other.index_) {
    detail::do_visit<void, true>(
        [this](auto&& r_data, auto index_seq) {
          constexpr std::size_t index = detail::get_index<0>(index_seq);
          detail::emplace<index>(this->union_, std::forward<decltype(r_data)>(r_data));
        },
        other
    );
  }

  constexpr variant(variant&& other) noexcept
    requires (std::is_trivially_move_constructible_v<Types> && ...)
  = default;

  constexpr variant(variant&& other) noexcept((std::is_nothrow_move_constructible_v<Types> && ...))
    requires ((std::is_move_constructible_v<Types> && ...) && (!std::is_trivially_move_constructible_v<Types> || ...))
      : index_(other.index_) {
    detail::do_visit<void, true>(
        [this](auto&& r_data, auto index_seq) {
          constexpr std::size_t j = detail::get_index<0>(index_seq);
          this->template emplace<j>(std::forward<decltype(r_data)>(r_data));
        },
        std::forward<variant>(other)
    );
  }

  template <class T>
    requires (
        sizeof...(Types) > 0 && !std::is_same_v<std::remove_cvref_t<T>, variant> &&
        !detail::is_specialization_of_v<std::remove_cvref_t<T>, in_place_type_t> &&
        !detail::is_numeric_specialization_of_v<std::remove_cvref_t<T>, in_place_index_t> &&
        (detail::select_index_v<T, variant> != variant_npos)
    )
  constexpr variant(T&& t) noexcept(std::is_nothrow_constructible_v<detail::select_type_t<T, variant>, T>)
      : variant(in_place_index_t<detail::select_index_v<T, variant>>(), std::forward<T>(t)) {}

  template <class T, class... Args>
    requires (std::is_constructible_v<T, Args...> && detail::exactly_one_v<T, Types...>)
  constexpr explicit variant(in_place_type_t<T>, Args&&... args)
      : variant(in_place_index<detail::select_index_v<T, variant>>, std::forward<Args>(args)...) {}

  template <std::size_t I, class... Args>
    requires (I < sizeof...(Types)) && std::is_constructible_v<variant_alternative_t<I, variant>, Args...>
  constexpr explicit variant(in_place_index_t<I>, Args&&... args)
      : index_(I) {
    detail::emplace<I>(union_, std::forward<Args>(args)...);
  }

  constexpr variant& operator=(const variant& rhs)
    requires (!detail::is_copy_assignable_v<Types...>)
  = delete;

  constexpr variant& operator=(const variant& rhs) noexcept
    requires (detail::is_trivially_copy_assignable_v<Types...>)
  = default;

  constexpr variant& operator=(const variant& rhs)
    requires (!detail::is_trivially_copy_assignable_v<Types...>) && (detail::is_copy_assignable_v<Types...>)
  {
    detail::do_visit<void, true>(
        [this, &rhs](auto&& l_data, auto& r_data, auto index_seq) {
          constexpr std::size_t l_index = detail::get_index<0>(index_seq);
          constexpr std::size_t r_index = detail::get_index<1>(index_seq);
          if constexpr (l_index == r_index) {
            l_data = r_data;
          } else if constexpr (std::is_nothrow_copy_constructible_v<variant_alternative_t<r_index, variant>> ||
                               !std::is_nothrow_move_constructible_v<variant_alternative_t<r_index, variant>>) {
            this->template emplace<r_index>(r_data);
          } else {
            // this->template emplace<r_index>(r_data);
            this->operator=(variant(rhs));
          }
        },
        std::move(*this),
        rhs
    );
    return *this;
  }

  constexpr variant& operator=(variant&& rhs)
    requires (!detail::is_move_assignable_v<Types...>)
  = delete;

  constexpr variant& operator=(variant&& rhs) noexcept
    requires (detail::is_trivially_move_assignable_v<Types...>)
  = default;

  constexpr variant& operator=(variant&& rhs
  ) noexcept(((std::is_nothrow_move_constructible_v<Types> && std::is_nothrow_move_assignable_v<Types>) && ...))
    requires (!detail::is_trivially_move_assignable_v<Types...> && detail::is_move_assignable_v<Types...>)
  {
    detail::do_visit<void, true>(
        [this](auto&& l_data, auto&& r_data, auto index_seq) {
          constexpr std::size_t l_index = detail::get_index<0>(index_seq);
          constexpr std::size_t r_index = detail::get_index<1>(index_seq);
          if constexpr (l_index == r_index) {
            l_data = std::forward<decltype(r_data)>(r_data);
          } else {
            this->template emplace<r_index>(std::forward<decltype(r_data)>(r_data));
          }
        },
        std::move(*this),
        std::forward<variant>(rhs)
    );
    return *this;
  }

  template <class T, typename selected_type = detail::select_type_t<T, variant>>
    requires (
        sizeof...(Types) > 0 && !std::is_same_v<std::remove_cvref_t<T>, variant> &&
        !detail::is_specialization_of_v<std::remove_cvref_t<T>, in_place_type_t> &&
        !detail::is_numeric_specialization_of_v<std::remove_cvref_t<T>, in_place_index_t> &&
        (detail::select_index_v<T, variant> != variant_npos)
    )
  constexpr variant& operator=(T&& t) noexcept(std::is_nothrow_constructible_v<selected_type, T>) {
    if (valueless_by_exception()) {
      emplace<selected_type>(std::forward<T>(t));
    } else {
      detail::do_visit<void, true>(
          [this, &t](auto&& data, auto index_seq) {
            const std::size_t index = detail::get_index<0>(index_seq);
            if constexpr (std::is_same_v<selected_type, variant_alternative_t<index, variant>>) {
              data = std::forward<T>(t);
            } else if constexpr (std::is_nothrow_constructible_v<selected_type, T> ||
                                 !std::is_nothrow_move_constructible_v<selected_type>) {
              this->template emplace<selected_type>(std::forward<T>(t));
            } else {
              selected_type tmp(std::forward<T>(t));
              this->template emplace<selected_type>(std::forward<selected_type>(tmp));
            }
          },
          std::move(*this)
      );
    }
    return *this;
  }

  constexpr ~variant() noexcept
    requires (std::is_trivially_destructible_v<Types> && ...)
  = default;

  constexpr ~variant() noexcept
    requires (!std::is_trivially_destructible_v<Types> || ...)
  {
    clear();
  }

  constexpr void swap(variant& rhs
  ) noexcept(((std::is_nothrow_move_constructible_v<Types> && std::is_nothrow_swappable_v<Types>) && ...)) {
    using std::swap;
    if (valueless_by_exception() && rhs.valueless_by_exception()) {
      return;
    }
    if (valueless_by_exception()) {
      rhs.swap(*this);
      return;
    }
    if (rhs.valueless_by_exception()) {
      detail::do_visit<void, true>(
          [&rhs](auto&& l_data, auto index_seq) {
            constexpr std::size_t index = detail::get_index<0>(index_seq);
            rhs.template emplace<index>(std::forward<decltype(l_data)>(l_data));
          },
          std::move(*this)
      );
      clear();
      return;
    }

    detail::do_visit<void, true>(
        [this, &rhs](auto&& l_data, auto&& r_data, auto index_seq) {
          constexpr std::size_t l_index = detail::get_index<0>(index_seq);
          constexpr std::size_t r_index = detail::get_index<1>(index_seq);
          if constexpr (l_index == r_index) {
            swap(l_data, r_data);
          } else {
            auto temp_l_data = std::forward<decltype(l_data)>(l_data);
            this->template emplace<r_index>(std::forward<decltype(r_data)>(r_data));
            rhs.template emplace<l_index>(std::forward<decltype(l_data)>(temp_l_data));
          }
        },
        std::move(*this),
        std::move(rhs)
    );
  }

  constexpr std::size_t index() const noexcept {
    return index_;
  }

  constexpr bool valueless_by_exception() const noexcept {
    return index_ == variant_npos;
  }

  template <class T, class... Args>
  constexpr T& emplace(Args&&... args) {
    // select_index_v
    return emplace<detail::find_type_v<T, Types...>>(std::forward<Args>(args)...);
  }

  template <std::size_t I, class... Args>
  constexpr variant_alternative_t<I, variant>& emplace(Args&&... args) {
    clear();
    detail::emplace<I>(union_, std::forward<Args>(args)...);
    index_ = I;
    return get<I>(*this);
  }

  constexpr void clear() {
    if (valueless_by_exception()) {
      return;
    }

    detail::do_visit<void, true>(
        [this](auto&&, auto index_seq) {
          constexpr std::size_t index = detail::get_index<0>(index_seq);
          detail::destroy<index>(union_);
        },
        std::move(*this)
    );
    index_ = variant_npos;
  }

private:
  template <std::size_t I, typename... Ts>
  friend constexpr variant_alternative_t<I, variant<Ts...>>& get(variant<Ts...>& v);

  template <std::size_t I, class... Ts>
  friend constexpr variant_alternative_t<I, variant<Ts...>>&& get(variant<Ts...>&& v);

private:
  detail::variadic_union<detail::wrapped_value<Types>...> union_;
  std::size_t index_;
};

template <typename... Types>
void swap(variant<Types...>& lhs, variant<Types...>& rhs) noexcept(
    ((std::is_nothrow_move_constructible_v<Types> && std::is_nothrow_swappable_v<Types>) && ...)
) {
  lhs.swap(rhs);
}

template <std::size_t I, class... Types>
constexpr std::add_pointer_t<variant_alternative_t<I, variant<Types...>>> get_if(variant<Types...>* pv) noexcept {
  if (pv != nullptr && I == pv->index()) {
    return std::addressof(get<I>(*pv));
  }
  return nullptr;
}

template <std::size_t I, class... Types>
constexpr std::add_pointer_t<const variant_alternative_t<I, variant<Types...>>> get_if(const variant<Types...>* pv
) noexcept {
  if (pv != nullptr && I == pv->index()) {
    return std::addressof(get<I>(*pv));
  }
  return nullptr;
}

template <class T, class... Types>
constexpr std::add_pointer_t<T> get_if(variant<Types...>* pv) noexcept {
  if (pv != nullptr && holds_alternative<T>(*pv)) {
    return std::addressof(get<T>(*pv));
  }
  return nullptr;
}

template <class T, class... Types>
constexpr std::add_pointer_t<const T> get_if(const variant<Types...>* pv) noexcept {
  if (pv != nullptr && holds_alternative<T>(*pv)) {
    return std::addressof(get<T>(*pv));
  }
  return nullptr;
}

template <class... Types>
constexpr bool operator==(const variant<Types...>& v, const variant<Types...>& w) {
  if (v.index() != w.index()) {
    return false;
  }
  if (v.valueless_by_exception()) {
    return true;
  }
  return detail::do_visit<bool, true>(
      [](auto& l_data, auto& r_data, auto index_seq) {
        constexpr std::size_t l_index = detail::get_index<0>(index_seq);
        constexpr std::size_t r_index = detail::get_index<1>(index_seq);
        if constexpr (l_index == r_index) {
          return l_data == r_data;
        } else {
          return false;
        }
      },
      v,
      w
  );
}

template <class... Types>
constexpr bool operator!=(const variant<Types...>& v, const variant<Types...>& w) {
  if (v.index() != w.index()) {
    return true;
  }
  if (v.valueless_by_exception()) {
    return false;
  }
  return detail::do_visit<bool, true>(
      [](auto& l_data, auto& r_data, auto index_seq) {
        constexpr std::size_t l_index = detail::get_index<0>(index_seq);
        constexpr std::size_t r_index = detail::get_index<1>(index_seq);
        if constexpr (l_index == r_index) {
          return l_data != r_data;
        } else {
          return false;
        }
      },
      v,
      w
  );
}

template <class... Types>
constexpr bool operator<(const variant<Types...>& v, const variant<Types...>& w) {
  if (w.valueless_by_exception()) {
    return false;
  }
  if (v.valueless_by_exception()) {
    return true;
  }
  if (v.index() < w.index()) {
    return true;
  }
  if (v.index() > w.index()) {
    return false;
  }
  return detail::do_visit<bool, true>(
      [](auto& l_data, auto& r_data, auto index_seq) {
        constexpr std::size_t l_index = detail::get_index<0>(index_seq);
        constexpr std::size_t r_index = detail::get_index<1>(index_seq);
        if constexpr (l_index == r_index) {
          return l_data < r_data;
        } else {
          return false;
        }
      },
      v,
      w
  );
}

template <class... Types>
constexpr bool operator>(const variant<Types...>& v, const variant<Types...>& w) {
  if (v.valueless_by_exception()) {
    return false;
  }
  if (w.valueless_by_exception()) {
    return true;
  }
  if (v.index() > w.index()) {
    return true;
  }
  if (v.index() < w.index()) {
    return false;
  }
  return detail::do_visit<bool, true>(
      [](auto& l_data, auto& r_data, auto index_seq) {
        constexpr std::size_t l_index = detail::get_index<0>(index_seq);
        constexpr std::size_t r_index = detail::get_index<1>(index_seq);
        if constexpr (l_index == r_index) {
          return l_data > r_data;
        } else {
          return false;
        }
      },
      v,
      w
  );
}

template <class... Types>
constexpr bool operator<=(const variant<Types...>& v, const variant<Types...>& w) {
  if (v.valueless_by_exception()) {
    return true;
  }
  if (w.valueless_by_exception()) {
    return false;
  }
  if (v.index() < w.index()) {
    return true;
  }
  if (v.index() > w.index()) {
    return false;
  }
  return detail::do_visit<bool, true>(
      [](auto& l_data, auto& r_data, auto index_seq) {
        constexpr std::size_t l_index = detail::get_index<0>(index_seq);
        constexpr std::size_t r_index = detail::get_index<1>(index_seq);
        if constexpr (l_index == r_index) {
          return l_data <= r_data;
        } else {
          return false;
        }
      },
      v,
      w
  );
}

template <class... Types>
constexpr bool operator>=(const variant<Types...>& v, const variant<Types...>& w) {
  if (w.valueless_by_exception()) {
    return true;
  }
  if (v.valueless_by_exception()) {
    return false;
  }
  if (v.index() > w.index()) {
    return true;
  }
  if (v.index() < w.index()) {
    return false;
  }
  return detail::do_visit<bool, true>(
      [](auto& l_data, auto& r_data, auto index_seq) {
        constexpr std::size_t l_index = detail::get_index<0>(index_seq);
        constexpr std::size_t r_index = detail::get_index<1>(index_seq);
        if constexpr (l_index == r_index) {
          return l_data >= r_data;
        } else {
          return false;
        }
      },
      v,
      w
  );
}

template <class... Types>
constexpr std::common_comparison_category_t<std::compare_three_way_result_t<Types>...>
operator<=>(const variant<Types...>& v, const variant<Types...>& w) {
  if (v.valueless_by_exception() && w.valueless_by_exception()) {
    return std::strong_ordering::equal;
  }
  if (v.valueless_by_exception()) {
    return std::strong_ordering::less;
  }
  if (w.valueless_by_exception()) {
    return std::strong_ordering::greater;
  }
  if (v.index() != w.index()) {
    return v.index() <=> w.index();
  }
  return detail::do_visit<std::common_comparison_category_t<std::compare_three_way_result_t<Types>...>, true>(
      [](auto& l_data, auto& r_data, auto index_seq) {
        constexpr std::size_t l_index = detail::get_index<0>(index_seq);
        constexpr std::size_t r_index = detail::get_index<1>(index_seq);
        if constexpr (l_index == r_index) {
          return l_data <=> r_data;
        } else {
          return l_index <=> r_index;
        }
      },
      v,
      w
  );
}
