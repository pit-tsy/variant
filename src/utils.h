#pragma once

namespace utils {
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
  static constexpr std::size_t value = static_cast<std::size_t>(-1);
};

} // namespace utils
