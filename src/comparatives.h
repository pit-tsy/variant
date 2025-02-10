#pragma once

#include "visit.h"

#include <compare>
#include <cstddef>

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
        constexpr std::size_t l_index = utils::get_index<0>(index_seq);
        constexpr std::size_t r_index = utils::get_index<1>(index_seq);
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
        constexpr std::size_t l_index = utils::get_index<0>(index_seq);
        constexpr std::size_t r_index = utils::get_index<1>(index_seq);
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
        constexpr std::size_t l_index = utils::get_index<0>(index_seq);
        constexpr std::size_t r_index = utils::get_index<1>(index_seq);
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
        constexpr std::size_t l_index = utils::get_index<0>(index_seq);
        constexpr std::size_t r_index = utils::get_index<1>(index_seq);
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
        constexpr std::size_t l_index = utils::get_index<0>(index_seq);
        constexpr std::size_t r_index = utils::get_index<1>(index_seq);
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
        constexpr std::size_t l_index = utils::get_index<0>(index_seq);
        constexpr std::size_t r_index = utils::get_index<1>(index_seq);
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
        constexpr std::size_t l_index = utils::get_index<0>(index_seq);
        constexpr std::size_t r_index = utils::get_index<1>(index_seq);
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
