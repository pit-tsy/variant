#pragma once

#include "comparatives.h"
#include "helpers.h"
#include "utils.h"
#include "visit.h"

#include <cstddef>
#include <exception>
#include <utility>

template <class... Types>
class variant;

namespace detail {

template <typename T>
struct wrapped_value {
  T value{};

  constexpr wrapped_value()
    requires (std::is_default_constructible_v<T>)
  = default;

  template <typename... Args>
  constexpr wrapped_value(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
    requires (sizeof...(Args) > 0) && std::is_constructible_v<T, Args...>
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
    std::construct_at(std::addressof(rest));
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

template <std::size_t I, typename... Types>
constexpr utils::nth_t<I, Types...>& get(variadic_union<Types...>& u) {
  return union_i<I>(u).head;
}

template <std::size_t I, typename... Types>
constexpr utils::nth_t<I, Types...>&& get(variadic_union<Types...>&& u) {
  return std::move(union_i<I>(u).head);
}

template <std::size_t I, typename Union, typename... Args>
constexpr decltype(auto) emplace(Union& u, Args&&... args) {
  return union_i<I>(u).emplace(std::forward<Args>(args)...);
}

template <std::size_t I, typename Union>
constexpr void destroy(Union& u) {
  union_i<I>(u).destroy();
}

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
  static std::integral_constant<size_t, Ind> try_it(Ti);
};

template <typename T, typename Variant, typename = utils::make_index_sequence_t<variant_size_v<Variant>>>
struct try_types;

template <typename T, typename... Ti, size_t... Ind>
struct try_types<T, variant<Ti...>, utils::index_sequence_t<Ind...>> : try_type<Ind, T, Ti>... {
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

template <typename T, typename... Types>
struct exactly_one;

template <typename T, typename... Types>
inline constexpr bool exactly_one_v = exactly_one<T, Types...>::value;

template <typename T, typename... Rest>
struct exactly_one<T, T, Rest...> {
  static constexpr bool value = (utils::find_type_v<T, Rest...> == variant_npos);
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
  return !v.valueless_by_exception() && utils::find_type_v<T, Types...> == v.index();
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
  if constexpr (utils::find_type_v<T, Types...> == variant_npos) {
    throw bad_variant_access();
  } else {
    return get<utils::find_type_v<T, Types...>>(v);
  }
}

template <class T, class... Types>
constexpr T&& get(variant<Types...>&& v) {
  if constexpr (utils::find_type_v<T, Types...> == variant_npos) {
    throw bad_variant_access();
  } else {
    return get<utils::find_type_v<T, Types...>>(std::move(v));
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
class variant {
public:
  constexpr variant() noexcept(std::is_nothrow_default_constructible_v<utils::nth_t<0, Types...>>)
    requires (std::is_default_constructible_v<utils::nth_t<0, Types...>>)
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
          constexpr std::size_t index = utils::get_index<0>(index_seq);
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
          constexpr std::size_t j = utils::get_index<0>(index_seq);
          this->template emplace<j>(std::forward<decltype(r_data)>(r_data));
        },
        std::forward<variant>(other)
    );
  }

  template <class T>
    requires (
        sizeof...(Types) > 0 && !std::is_same_v<std::remove_cvref_t<T>, variant> &&
        !utils::is_specialization_of_v<std::remove_cvref_t<T>, in_place_type_t> &&
        !utils::is_numeric_specialization_of_v<std::remove_cvref_t<T>, in_place_index_t> &&
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
          constexpr std::size_t l_index = utils::get_index<0>(index_seq);
          constexpr std::size_t r_index = utils::get_index<1>(index_seq);
          if constexpr (l_index == r_index) {
            l_data = r_data;
          } else if constexpr (std::is_nothrow_copy_constructible_v<variant_alternative_t<r_index, variant>> ||
                               !std::is_nothrow_move_constructible_v<variant_alternative_t<r_index, variant>>) {
            this->template emplace<r_index>(r_data);
          } else {
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
          constexpr std::size_t l_index = utils::get_index<0>(index_seq);
          constexpr std::size_t r_index = utils::get_index<1>(index_seq);
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
        !utils::is_specialization_of_v<std::remove_cvref_t<T>, in_place_type_t> &&
        !utils::is_numeric_specialization_of_v<std::remove_cvref_t<T>, in_place_index_t> &&
        (detail::select_index_v<T, variant> != variant_npos)
    )
  constexpr variant& operator=(T&& t) noexcept(std::is_nothrow_constructible_v<selected_type, T>) {
    if (valueless_by_exception()) {
      emplace<selected_type>(std::forward<T>(t));
    } else {
      detail::do_visit<void, true>(
          [this, &t](auto&& data, auto index_seq) {
            const std::size_t index = utils::get_index<0>(index_seq);
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
            constexpr std::size_t index = utils::get_index<0>(index_seq);
            rhs.template emplace<index>(std::forward<decltype(l_data)>(l_data));
          },
          std::move(*this)
      );
      clear();
      return;
    }

    detail::do_visit<void, true>(
        [this, &rhs](auto&& l_data, auto&& r_data, auto index_seq) {
          constexpr std::size_t l_index = utils::get_index<0>(index_seq);
          constexpr std::size_t r_index = utils::get_index<1>(index_seq);
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
    return emplace<utils::find_type_v<T, Types...>>(std::forward<Args>(args)...);
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
          constexpr std::size_t index = utils::get_index<0>(index_seq);
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
