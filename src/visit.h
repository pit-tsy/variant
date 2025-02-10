#pragma once

#include "helpers.h"
#include "utils.h"

#include <cstddef>

namespace detail {
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
    utils::index_sequence_t<indices...>> {
  using T = RetType (*)(Vis, Variants...);
  using CurrentArrayType = multi_array<T, sizes...>;

  using Current = utils::nth_t<sizeof...(indices), Variants...>;

  static constexpr CurrentArrayType make() {
    CurrentArrayType result{};
    make_all_alt(result, utils::make_index_sequence_t<variant_size_v<Current>>{});
    return result;
  }

  template <std::size_t... current_indices_>
  static constexpr void make_all_alt(CurrentArrayType& array, utils::index_sequence_t<current_indices_...>) {
    (make_alt(array[current_indices_], utils::index<current_indices_>{}), ...);
  }

  template <typename NextArrayType, std::size_t index_>
  static constexpr void make_alt(NextArrayType& array, utils::index<index_>) {
    array = make_vtable_impl<transfer_indices, NextArrayType, utils::index_sequence_t<indices..., index_>>::make();
  }
};

template <bool transfer_indices, typename RetType, typename Vis, typename... Variants, std::size_t... indices>
struct make_vtable_impl<
    transfer_indices,
    multi_array<RetType (*)(Vis, Variants...)>,
    utils::index_sequence_t<indices...>> {
  using T = RetType (*)(Vis, Variants...);
  using CurrentArrayType = multi_array<T>;

  static constexpr RetType invoke(Vis vis, Variants... variants) {
    if constexpr (transfer_indices) {
      return std::forward<
          Vis>(vis)(get<indices>(std::forward<Variants>(variants))..., utils::index_sequence<indices...>);
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
          utils::index_sequence_t<>> {};

template <class R, bool transfer_indices = false, class Visitor, class... Variants>
constexpr R do_visit(Visitor&& vis, Variants&&... vars) {
  if ((vars.valueless_by_exception() || ...)) {
    throw bad_variant_access("variant is valueless");
  }

  constexpr auto table = make_vtable<transfer_indices, R, Visitor&&, Variants&&...>::make();

  return (*(table.get(vars.index()...)))(std::forward<Visitor>(vis), std::forward<Variants>(vars)...);
}
} // namespace detail
