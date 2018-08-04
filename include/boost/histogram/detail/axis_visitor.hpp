// Copyright 2015-2017 Hans Demsizeki
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef _BOOST_HISTOGARM_AXIS_VISITOR_HPP_
#define _BOOST_HISTOGARM_AXIS_VISITOR_HPP_

#include <boost/histogram/axis/any.hpp>
#include <boost/histogram/detail/meta.hpp>
#include <boost/mp11.hpp>
#include <tuple>
#include <type_traits>
#include <vector>

namespace boost {
namespace histogram {
namespace detail {

template <typename... Ts>
using dynamic_axes = std::vector<boost::histogram::axis::any<Ts...>>;

template <typename... Ts>
using static_axes = std::tuple<Ts...>;

namespace {

template <typename StaticAxes, typename DynamicAxes>
struct axes_equal_static_dynamic_impl {
  bool& equal;
  const StaticAxes& t;
  const DynamicAxes& v;
  axes_equal_static_dynamic_impl(bool& eq, const StaticAxes& tt,
                                 const DynamicAxes& vv)
      : equal(eq), t(tt), v(vv) {}
  template <typename Int>
  void operator()(Int) const {
    using T = mp11::mp_at<StaticAxes, Int>;
    auto tp = boost::get<T>(&v[Int::value]);
    equal &= (tp && *tp == std::get<Int::value>(t));
  }
};

template <typename... Ts>
bool axes_equal_static_static_impl(mp11::mp_true, const static_axes<Ts...>& t,
                                   const static_axes<Ts...>& u) {
  return t == u;
}

template <typename... Ts, typename... Us>
bool axes_equal_static_static_impl(mp11::mp_false, const static_axes<Ts...>&,
                                   const static_axes<Us...>&) {
  return false;
}

template <typename StaticAxes, typename DynamicAxes>
struct axes_assign_static_dynamic_impl {
  StaticAxes& t;
  const DynamicAxes& v;
  axes_assign_static_dynamic_impl(StaticAxes& tt, const DynamicAxes& vv)
      : t(tt), v(vv) {}
  template <typename Int>
  void operator()(Int) const {
    using T = mp11::mp_at<StaticAxes, Int>;
    std::get<Int::value>(t) = boost::get<T>(v[Int::value]);
  }
};

template <typename DynamicAxes, typename StaticAxes>
struct axes_assign_dynamic_static_impl {
  DynamicAxes& v;
  const StaticAxes& t;
  axes_assign_dynamic_static_impl(DynamicAxes& vv, const StaticAxes& tt)
      : v(vv), t(tt) {}
  template <typename Int>
  void operator()(Int) const {
    v[Int::value] = std::get<Int::value>(t);
  }
};
} // namespace

template <typename... Ts, typename... Us>
bool axes_equal(const static_axes<Ts...>& t, const static_axes<Us...>& u) {
  return axes_equal_static_static_impl(
      mp11::mp_same<mp11::mp_list<Ts...>, mp11::mp_list<Us...>>(), t, u);
}

template <typename... Ts, typename... Us>
void axes_assign(static_axes<Ts...>& t, const static_axes<Us...>& u) {
  static_assert(
      std::is_same<mp11::mp_list<Ts...>, mp11::mp_list<Us...>>::value,
      "cannot assign incompatible axes");
  t = u;
}

template <typename... Ts, typename... Us>
bool axes_equal(const static_axes<Ts...>& t, const dynamic_axes<Us...>& u) {
  if (sizeof...(Ts) != u.size()) return false;
  bool equal = true;
  auto fn =
      axes_equal_static_dynamic_impl<static_axes<Ts...>, dynamic_axes<Us...>>(
          equal, t, u);
  mp11::mp_for_each<mp11::mp_iota_c<sizeof...(Ts)>>(fn);
  return equal;
}

template <typename... Ts, typename... Us>
void axes_assign(static_axes<Ts...>& t, const dynamic_axes<Us...>& u) {
  auto fn = axes_assign_static_dynamic_impl<static_axes<Ts...>,
                                            dynamic_axes<Us...>>(t, u);
  mp11::mp_for_each<mp11::mp_iota_c<sizeof...(Ts)>>(fn);
}

template <typename... Ts, typename... Us>
bool axes_equal(const dynamic_axes<Ts...>& t, const static_axes<Us...>& u) {
  return axes_equal(u, t);
}

template <typename... Ts, typename... Us>
void axes_assign(dynamic_axes<Ts...>& t, const static_axes<Us...>& u) {
  t.resize(sizeof...(Us));
  auto fn = axes_assign_dynamic_static_impl<dynamic_axes<Ts...>,
                                            static_axes<Us...>>(t, u);
  mp11::mp_for_each<mp11::mp_iota_c<sizeof...(Us)>>(fn);
}

template <typename... Ts, typename... Us>
bool axes_equal(const dynamic_axes<Ts...>& t, const dynamic_axes<Us...>& u) {
  if (t.size() != u.size()) return false;
  for (std::size_t i = 0; i < t.size(); ++i) {
    if (t[i] != u[i]) return false;
  }
  return true;
}

template <typename... Ts, typename... Us>
void axes_assign(dynamic_axes<Ts...>& t, const dynamic_axes<Us...>& u) {
  for (std::size_t i = 0; i < t.size(); ++i) { t[i] = u[i]; }
}

struct shape_collector {
  std::vector<unsigned>::iterator iter;
  shape_collector(std::vector<unsigned>::iterator i) : iter(i) {}
  template <typename T>
  void operator()(const T& a) {
    *iter++ = a.shape();
  }
};

struct field_counter {
  std::size_t value = 1;
  template <typename T>
  void operator()(const T& t) {
    value *= t.shape();
  }
};

template <typename Unary>
struct unary_adaptor_visitor : public static_visitor<void> {
  Unary&& unary;
  unary_adaptor_visitor(Unary&& u) : unary(std::forward<Unary>(u)) {}
  template <typename T>
  void operator()(const T& a) const {
    unary(a);
  }
};

} // namespace detail
} // namespace histogram
} // namespace boost

#endif
