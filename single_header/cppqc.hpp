/*
 *  CppQuickCheck v1.0.0
 *  Generated: 2022-10-16 11:01:23.452308
 *  ----------------------------------------------------------
 *  This file has been merged from multiple headers. Please don't edit it directly
 *  Copyright (c) 2019, Gregory Rogers and Philipp Claßen. All rights reserved.
 *
 *  Distributed under the MIT Software License. (See accompanying
 *  file LICENSE or copy at https://opensource.org/licenses/MIT)
 */

#ifndef CPPQC_HPP
#define CPPQC_HPP

#include <limits>
#include <random>
#include <cstddef>
#include <iterator>
#include <memory>
#include <set>
#include <tuple>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <valarray>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <ostream>
#include <string>

namespace cppqc {

    
/*
 * Type trait for checking if a type is found in a parameter pack.
 */
template <class T, class... Us>
struct IsOneOf;

template <class T, class U, class... Us>
struct IsOneOf<T, U, Us...> {
  static constexpr bool value = IsOneOf<T, Us...>::value;
};

template <class T, class... Us>
struct IsOneOf<T, T, Us...> {
  static constexpr bool value = true;
};

template <class T>
struct IsOneOf<T> {
  static constexpr bool value = false;
};

/*
 * Type trait for checking if a type can be used as the template parameter of
 * std::uniform_int_distribution.
 */
template <class T>
struct IntDistributionSupported {
  static constexpr bool value = IsOneOf<T,
                                        short,
                                        int,
                                        long,
                                        long long,
                                        unsigned short,
                                        unsigned int,
                                        unsigned long,
                                        unsigned long long>::value;
};

// default generators

/*
 * Generates an integral number from a bounded domain defined by
 * the integer type.
 * The number will be chosen uniformly, that means big numbers
 * are equally likely as numbers near 0.
 */
template <class Integral>
Integral arbitraryBoundedIntegral(RngEngine& rng, std::size_t /*size*/) {
  using DistributionType =
      typename std::conditional<IntDistributionSupported<Integral>::value,
                                Integral, int>::type;
  std::uniform_int_distribution<DistributionType> dist{
      std::numeric_limits<Integral>::lowest(),
      std::numeric_limits<Integral>::max()};
  return dist(rng);
}

template <class Integral>
Integral arbitrarySizedIntegral(RngEngine& rng, std::size_t size) {
  if (size > std::numeric_limits<Integral>::max()) {
    return arbitraryBoundedIntegral<Integral>(rng, size);
  }

  std::uniform_int_distribution<Integral> dist(
      std::numeric_limits<Integral>::is_signed ? -Integral(size)
                                               : Integral(size),
      Integral(size));
  return dist(rng);
}

/*
 * Generates an integral number from a bounded domain. The number is
 * chosen from the entire range of the type, but numbers near 0 are
 * generated more often than big numbers.
 */
template <class Integral>
Integral arbitrarySizedBoundedIntegral(RngEngine& rng, std::size_t size) {
  const double limit =
      std::min(double(size), double(std::numeric_limits<Integral>::max()));

  // When dividing by 3, about 99% of the values should be
  // in the interval [-limit, limit]
  std::normal_distribution<> dist{0, 1.0 + limit / 3};
  for (;;) {
    double r = dist(rng);

    if (!std::numeric_limits<Integral>::is_signed) {
      r = std::abs(r);
    }

    if (r >= std::numeric_limits<Integral>::lowest() &&
        r <= std::numeric_limits<Integral>::max()) {
      return static_cast<Integral>(r);
    }
  }
}

template <class Real>
Real arbitrarySizedReal(RngEngine& rng, std::size_t size) {
  std::uniform_real_distribution<Real> dist{-Real(size + 1.0),
                                            Real(size + 1.0)};
  return dist(rng);
}

// default shrinkers

template <class T>
std::vector<T> shrinkNothing(const T&) {
  return std::vector<T>();
}

template <class Integral>
std::vector<Integral> shrinkIntegral(Integral x) {
  std::vector<Integral> ret;
  if (std::numeric_limits<Integral>::is_signed && x < 0) {
    if (x == std::numeric_limits<Integral>::min()) {
      ret.push_back(std::numeric_limits<Integral>::max());
    } else {
      assert(-x > 0);
      ret.push_back(-x);
    }
  }

  for (Integral n = x; n != 0; n /= 2)
    ret.push_back(x - n);
  return ret;
}

template <class Real>
std::vector<Real> shrinkReal(Real x) {
  std::vector<Real> ret;
  if (x == 0)
    return ret;
  if (x < 0)
    ret.push_back(-x);
  ret.push_back(Real(0));

  if (std::isnan(x) && std::abs(x) >= 2) {
    if (std::abs(x) < 1e100) {
      ret.push_back(x / Real(2));
    } else {
      // special case: reduce faster if the numbers are huge
      // (Note: Maybe there is a better heuristic. Looks quite crude.)
      ret.push_back(x / Real(1e20));
    }
  }
  return ret;
}

template <class T>
struct ArbitraryImpl;

// specialize Arbitrary and implement the members:
//     T unGen(RngEngine&, std::size_t);
//     std::vector<T> shrink(T);
//
// If they do not and they try to use Arbitrary<TheirClass>, a compile error
// will result.
template <class T>
struct Arbitrary {
  static T unGen(RngEngine& rng, std::size_t size) {
    return ArbitraryImpl<T>::unGen(rng, size);
  }

  template <typename U>
  static std::vector<T> shrink(U&& x) {
    return ArbitraryImpl<T>::shrink(std::forward<U>(x));
  }
};

template <>
struct ArbitraryImpl<bool> {
  static bool unGen(RngEngine& rng, std::size_t /*size*/) {
    return std::uniform_int_distribution<int>(0, 1)(rng) != 0;
  }

  static std::vector<bool> shrink(bool x) {
    std::vector<bool> ret;
    if (x)
      ret.push_back(false);
    return ret;
  }
};

namespace detail {
template <typename T>
struct ArbitrarySizedBoundedIntegral {
  static T unGen(RngEngine& rng, std::size_t size) {
    return arbitrarySizedBoundedIntegral<T>(rng, size);
  }
  static std::vector<T> shrink(T x) { return shrinkIntegral(x); }
};

template <typename T>
struct ArbitrarySizedReal {
  static T unGen(RngEngine& rng, std::size_t size) {
    return arbitrarySizedReal<T>(rng, size);
  }
  static std::vector<T> shrink(T x) { return shrinkReal(x); }
};

template <typename String>
struct ArbitraryString {
  static String unGen(RngEngine& rng, std::size_t size) {
    String ret;
    std::uniform_int_distribution<std::size_t> dist{0, size};
    std::size_t n = dist(rng);
    ret.reserve(n);
    while (n-- > 0)
      ret.push_back(Arbitrary<typename String::value_type>::unGen(rng, size));
    return ret;
  }
  static std::vector<String> shrink(const String& x) {
    std::vector<String> ret;
    ret.reserve(x.size());
    for (auto it = x.begin(); it != x.end(); ++it) {
      ret.push_back(String());
      ret.back().reserve(x.size() - 1);
      ret.back().insert(ret.back().end(), x.begin(), it);
      ret.back().insert(ret.back().end(), it + 1, x.end());
    }
    return ret;
  }
};

template <typename PairType>
struct ArbitraryPair {
  static PairType unGen(RngEngine& rng, std::size_t size) {
    return PairType(
        Arbitrary<typename PairType::first_type>::unGen(rng, size),
        Arbitrary<typename PairType::second_type>::unGen(rng, size));
  }
  static std::vector<PairType> shrink(const PairType& x) {
    std::vector<PairType> ret;
    using FirstType = typename PairType::first_type;
    using SecondType = typename PairType::second_type;
    std::vector<FirstType> shrinks1 = Arbitrary<FirstType>::shrink(x.first);
    std::vector<SecondType> shrinks2 = Arbitrary<SecondType>::shrink(x.second);
    ret.reserve(shrinks1.size() + shrinks2.size());
    for (auto it = shrinks1.begin(); it != shrinks1.end(); ++it) {
      ret.push_back(PairType(*it, x.second));
    }
    for (auto it = shrinks2.begin(); it != shrinks2.end(); ++it) {
      ret.push_back(PairType(x.first, *it));
    }
    return ret;
  }
};

}  // namespace detail

// builtin integer types
template <>
struct ArbitraryImpl<signed char>
    : detail::ArbitrarySizedBoundedIntegral<signed char> {};
template <>
struct ArbitraryImpl<unsigned char>
    : detail::ArbitrarySizedBoundedIntegral<unsigned char> {};
template <>
struct ArbitraryImpl<signed short>
    : detail::ArbitrarySizedBoundedIntegral<signed short> {};
template <>
struct ArbitraryImpl<unsigned short>
    : detail::ArbitrarySizedBoundedIntegral<unsigned short> {};
template <>
struct ArbitraryImpl<signed int>
    : detail::ArbitrarySizedBoundedIntegral<signed int> {};
template <>
struct ArbitraryImpl<unsigned int>
    : detail::ArbitrarySizedBoundedIntegral<unsigned int> {};
template <>
struct ArbitraryImpl<signed long>
    : detail::ArbitrarySizedBoundedIntegral<signed long> {};
template <>
struct ArbitraryImpl<unsigned long>
    : detail::ArbitrarySizedBoundedIntegral<unsigned long> {};
template <>
struct ArbitraryImpl<signed long long>
    : detail::ArbitrarySizedBoundedIntegral<signed long long> {};
template <>
struct ArbitraryImpl<unsigned long long>
    : detail::ArbitrarySizedBoundedIntegral<unsigned long long> {};

// builtin floating point types
template <>
struct ArbitraryImpl<float> : detail::ArbitrarySizedReal<float> {};
template <>
struct ArbitraryImpl<double> : detail::ArbitrarySizedReal<double> {};
template <>
struct ArbitraryImpl<long double> : detail::ArbitrarySizedReal<long double> {};

// builtin character types
template <>
struct ArbitraryImpl<char> {
  static char unGen(RngEngine& rng, std::size_t /*size*/) {
    std::uniform_int_distribution<int> dist{0x20, 0x7f};
    return static_cast<char>(dist(rng));
  }
  static std::vector<char> shrink(char c) {
    std::vector<char> ret;
    constexpr char possShrinks[] = {'a', 'b', 'c', 'A', 'B',  'C',
                                    '1', '2', '3', ' ', '\n', '\0'};
    for (auto possShrink : possShrinks) {
      if (possShrink < c)
        ret.push_back(possShrink);
    }
    if (isupper(c) &&
        std::find(possShrinks, possShrinks + sizeof(possShrinks), tolower(c)) ==
            possShrinks + sizeof(possShrinks)) {
      ret.push_back(tolower(c));
    }
    return ret;
  }
};

template <>
struct ArbitraryImpl<wchar_t> : detail::ArbitrarySizedBoundedIntegral<wchar_t> {
};

// builtin string types
template <class CharT, class Traits, class Alloc>
struct ArbitraryImpl<std::basic_string<CharT, Traits, Alloc>>
    : detail::ArbitraryString<std::basic_string<CharT, Traits, Alloc>> {};

// std::pair related types
template <class T1, class T2>
struct ArbitraryImpl<std::pair<T1, T2>>
    : detail::ArbitraryPair<std::pair<T1, T2>> {};

template <class T>
struct ArbitraryImpl<std::vector<T>> {
  static std::vector<T> unGen(RngEngine& rng, std::size_t size) {
    return listOf<T>().unGen(rng, size);
  }

  static std::vector<std::vector<T>> shrink(const std::vector<T>& v) {
    return listOf<T>().shrink(v);
  }
};

/// Note: N is the fixed size of the array.
/// It differs from the "size" param in the "unGen" function:
///
/// If "size" is increased, the output array will still contain
/// N elements, but each element will, in general, be more complex.
template <class T, std::size_t N>
struct ArbitraryImpl<std::array<T, N>> {
  static std::array<T, N> unGen(RngEngine& rng, std::size_t size) {
    return arrayOf<T, N>().unGen(rng, size);
  }

  static std::vector<std::array<T, N>> shrink(const std::array<T, N>& v) {
    return arrayOf<T, N>().shrink(v);
  }
};


template <typename Function>
struct CompactCheckFunction {
  using type = Function;
  Function m_Function;

  CompactCheckFunction(Function&& fnc) : m_Function(std::move(fnc)) {}

  CompactCheckFunction(CompactCheckFunction&&) = default;

  static const bool valid = true;

  template <typename ReturnType, typename... T>
  ReturnType apply(ReturnType, const T&... v) const {
    return m_Function(v...);
  }
};

template <>
struct CompactCheckFunction<void> {
  using type = void;
  static const bool valid = false;

  CompactCheckFunction() = default;
  CompactCheckFunction(CompactCheckFunction&&) = default;

  template <typename ReturnType, typename... T>
  ReturnType apply(ReturnType r, const T&... v) const {
    return r;
  }
};

template <typename CheckFunction,
          typename TrivialFunction,
          typename ClassifyFunction,
          typename... T>
class CompactCheck : public Property<T...> {
  std::string m_name;

  template <typename _CheckFunction,
            typename _TrivialFunction,
            typename _ClassifyFunction,
            typename... _T>
  friend class CompactCheck;

  using CheckFunctionT = CompactCheckFunction<CheckFunction>;
  CheckFunctionT m_checkFnc;

  using TrivialFunctionT = CompactCheckFunction<TrivialFunction>;
  TrivialFunctionT m_trivialFnc;

  using ClassifyFunctionT = CompactCheckFunction<ClassifyFunction>;
  ClassifyFunctionT m_classifyFnc;

  CompactCheck(std::string name,
               CheckFunctionT&& checkFunction,
               TrivialFunctionT&& trivialFunctions,
               ClassifyFunctionT&& classifyFunction)
      : m_name(std::move(name)),
        m_checkFnc(std::move(checkFunction)),
        m_trivialFnc(std::move(trivialFunctions)),
        m_classifyFnc(std::move(classifyFunction)) {}

  bool check(const T&... v) const override {
    return m_checkFnc.apply(true, v...);
  }

  bool trivial(const T&... v) const override {
    return m_trivialFnc.apply(false, v...);
  }

  std::string classify(const T&... v) const override {
    return m_classifyFnc.apply(std::string(), v...);
  }

  std::string name() const override {
    return m_name.empty() ? "no-name" : m_name;
  }

 public:
  CompactCheck() = default;

  CompactCheck(const Generator<T>&... g) : Property<T...>(g...) {}

  template <typename _CheckFunction>
  CompactCheck<_CheckFunction, TrivialFunction, ClassifyFunction, T...>
  property(const std::string& name, _CheckFunction&& checkFnc) {
    static_assert(CheckFunctionT::valid == false,
                  "Check function is already set");
    return CompactCheck<_CheckFunction, TrivialFunction, ClassifyFunction,
                        T...>(name, std::move(checkFnc),
                              std::move(m_trivialFnc),
                              std::move(m_classifyFnc));
  }

  template <typename _TrivialFunction>
  CompactCheck<CheckFunction, _TrivialFunction, ClassifyFunction, T...> trivial(
      _TrivialFunction&& trivialFnc) {
    static_assert(TrivialFunctionT::valid == false,
                  "Trivial function is already set");
    return CompactCheck<CheckFunction, _TrivialFunction, ClassifyFunction,
                        T...>(std::move(m_name), std::move(m_checkFnc),
                              std::move(trivialFnc), std::move(m_classifyFnc));
  }

  template <typename _ClassifyFunction>
  CompactCheck<CheckFunction, TrivialFunction, _ClassifyFunction, T...>
  classify(_ClassifyFunction&& classifyFnc) {
    static_assert(ClassifyFunctionT::valid == false,
                  "Classsify function is already set");
    return CompactCheck<CheckFunction, TrivialFunction, _ClassifyFunction,
                        T...>(std::move(m_name), std::move(m_checkFnc),
                              std::move(m_trivialFnc), std::move(classifyFnc));
  }

  Result test(std::size_t maxSuccess = 100,
              std::size_t maxDiscarded = 0,
              std::size_t maxSize = 0) {
    return quickCheck(*this, maxSuccess, maxDiscarded, maxSize);
  }

  Result testWithOutput(std::ostream& out = std::cout,
                        std::size_t maxSuccess = 100,
                        std::size_t maxDiscarded = 0,
                        std::size_t maxSize = 0) {
    return quickCheckOutput(*this, out, maxSuccess, maxDiscarded, maxSize);
  }
};

template <typename... T>
CompactCheck<void, void, void, T...> gen() {
  return CompactCheck<void, void, void, T...>();
}

template <typename... T>
CompactCheck<void, void, void, T...> gen(const Generator<T>&... g) {
  return CompactCheck<void, void, void, T...>(g...);
}


using RngEngine = std::mt19937;

template <class T>
struct Arbitrary;

/*
 * When creating user defined generators, they must model a
 * "GeneratorConcept<T>", that is, they must be copy-constructable and have
 * member functions compatible with the following signatures:
 *
 *      T unGen(RngEngine &, std::size_t);
 *      std::vector<T> shrink(const T &);
 *
 * The unGen function generates a new value, and the shrink function shrinks
 * the last previously generated value down - generating an std::vector<T> of
 * possible alternative shrinks. It possible to create a stateful generator
 * modelling GenConcept, which in the generator stores some state in unGen that
 * is looked up when calling shrink. Note however that the shrink function may
 * be called multiple times, the subsequent calls may be called with the
 * argument being one of the values returned by the previous call to shrink.
 * This limits the types of statefulness possible.
 *
 * GeneratorConcept<T>:
 *      T unGen(RngEngine &, std::size_t);
 *      std::vector<T> shrink(const T &);
 *
 * StatelessGeneratorConcept<T> (also models GeneratorConcept<T>):
 *      T unGen(RngEngine &, std::size_t) const;
 *      std::vector<T> shrink(const T &) const;
 */

namespace detail {

template <class T>
struct GenConcept {
  virtual ~GenConcept() = default;
  virtual T unGen(RngEngine&, std::size_t) = 0;
  virtual std::vector<T> shrink(const T&) = 0;
  virtual GenConcept* clone() const = 0;
};

template <class T>
struct StatelessGenConcept : GenConcept<T> {
  virtual ~StatelessGenConcept() = default;
  T unGen(RngEngine&, std::size_t) override = 0;
  std::vector<T> shrink(const T&) override = 0;
  StatelessGenConcept* clone() const override = 0;
};
}  // namespace detail

template <class T>
class Generator;

template <class T>
class StatelessGenerator {
  template <class StatelessGeneratorModel>
  class StatelessGenModel;

 public:
  template <class StatelessGeneratorModel>
  StatelessGenerator(const StatelessGeneratorModel& gm)
      : m_gen{new StatelessGenModel<StatelessGeneratorModel>(gm)} {}

  StatelessGenerator(const StatelessGenerator& g) : m_gen{g.m_gen->clone()} {}

  template <class StatelessGeneratorModel>
  StatelessGenerator& operator=(const StatelessGeneratorModel& gm) {
    m_gen.reset(new StatelessGenModel<StatelessGeneratorModel>(gm));
    return *this;
  }

  StatelessGenerator& operator=(const StatelessGenerator& g) {
    if (this == &g) {
      return *this;
    }
    m_gen.reset(g.m_gen.clone());
    return *this;
  }

  T unGen(RngEngine& rng, std::size_t size) const {
    return m_gen->unGen(rng, size);
  }

  std::vector<T> shrink(const T& x) const { return m_gen->shrink(x); }

 private:
  template <class StatelessGeneratorModel>
  class StatelessGenModel : public detail::StatelessGenConcept<T> {
   public:
    StatelessGenModel(StatelessGeneratorModel gm) : m_obj(std::move(gm)) {}

    T unGen(RngEngine& rng, std::size_t size) override {
      return m_obj.unGen(rng, size);
    }

    T unGen(RngEngine& rng, std::size_t size) const {
      return m_obj.unGen(rng, size);
    }

    std::vector<T> shrink(const T& x) override { return m_obj.shrink(x); }

    std::vector<T> shrink(const T& x) const { return m_obj.shrink(x); }

    detail::StatelessGenConcept<T>* clone() const override {
      return new StatelessGenModel(m_obj);
    }

   private:
    const StatelessGeneratorModel m_obj;
  };

  friend class Generator<T>;
  std::unique_ptr<detail::StatelessGenConcept<T>> m_gen;
};

template <class T>
class Generator {
  template <class GeneratorModel>
  class GenModel;

 public:
  template <class GeneratorModel>
  Generator(const GeneratorModel& gm)
      : m_gen{new GenModel<GeneratorModel>(gm)} {}

  Generator(const Generator& g) : m_gen(g.m_gen->clone()) {}

  Generator(const StatelessGenerator<T>& g) : m_gen(g.m_gen->clone()) {}

  template <class GeneratorModel>
  Generator& operator=(const GeneratorModel& gm) {
    m_gen.reset(new GenModel<GeneratorModel>(gm));
    return *this;
  }

  Generator& operator=(const Generator& g) {
    if (this == &g) {
      return *this;
    }
    m_gen.reset(g.m_gen->clone());
    return *this;
  }

  Generator& operator=(const StatelessGenerator<T>& g) {
    m_gen.reset(g.m_gen->clone());
    return *this;
  }

  T unGen(RngEngine& rng, std::size_t size) const {
    return m_gen->unGen(rng, size);
  }

  std::vector<T> shrink(const T& x) const { return m_gen->shrink(x); }

 private:
  template <class GeneratorModel>
  class GenModel : public detail::GenConcept<T> {
   public:
    GenModel(GeneratorModel gm) : m_obj(std::move(gm)) {}

    T unGen(RngEngine& rng, std::size_t size) override {
      return m_obj.unGen(rng, size);
    }

    std::vector<T> shrink(const T& x) override { return m_obj.shrink(x); }

    detail::GenConcept<T>* clone() const override {
      return new GenModel(m_obj);
    }

   private:
    GeneratorModel m_obj;
  };

  std::unique_ptr<detail::GenConcept<T>> m_gen;
};

/// Generates some example values and returns them.
template <class T>
std::vector<T> sample(const Generator<T>& g,
                      std::size_t num = 0,
                      std::size_t seed = 0) {
  if (num == 0)
    num = 20;
  if (seed == 0)
    seed = std::time(nullptr);
  RngEngine rng(seed);
  std::vector<T> ret;
  ret.reserve(num);
  try {
    for (std::size_t i = 0; i < num; ++i)
      ret.push_back(g.unGen(rng, i));
  } catch (...) {
  }
  return ret;
}

/// Generates some example values and prints them.
template <class T>
void sampleOutput(const Generator<T>& g,
                  std::ostream& out,
                  std::size_t num = 0,
                  std::size_t seed = 0) {
  if (num == 0)
    num = 20;
  if (seed == 0)
    seed = std::time(nullptr);
  RngEngine rng(seed);
  try {
    for (std::size_t i = 0; i < num; ++i) {
      if (i != 0)
        out << ' ';
      out << g.unGen(rng, i);
    }
  } catch (...) {
  }
  out << std::endl;
}

/// Generates some example values, then generates a list of possible shrinks
/// for each value and returns both the value and the shrinks.
template <class T>
std::vector<std::pair<T, std::vector<T>>> sampleShrink(const Generator<T>& g,
                                                       std::size_t num = 0,
                                                       std::size_t seed = 0) {
  if (num == 0)
    num = 20;
  if (seed == 0)
    seed = std::time(nullptr);
  RngEngine rng(seed);
  std::vector<std::pair<T, std::vector<T>>> ret;
  ret.reserve(num);
  try {
    for (std::size_t i = 0; i < num; ++i) {
      T x = g.unGen(rng, i);
      ret.push_back(std::make_pair(x, g.shrink(x)));
    }
  } catch (...) {
  }
  return ret;
}

/// Outputs some example values from a generator. For each value, outputs
/// several possible shrinks of that value.
template <class T>
void sampleShrinkOutput(const Generator<T>& g,
                        std::ostream& out,
                        std::size_t num = 0,
                        bool randomized = false,
                        std::size_t seed = 0) {
  if (num == 0)
    num = 20;
  if (seed == 0)
    seed = std::time(nullptr);
  RngEngine rng(seed);
  try {
    for (std::size_t i = 0; i < num; ++i) {
      T x = g.unGen(rng, i);
      std::vector<T> shr = g.shrink(x);
      if (randomized)
        std::shuffle(shr.begin(), shr.end(), rng);
      out << x << " ->";
      for (std::size_t j = 0; j < num && j < shr.size(); ++j)
        out << ' ' << shr[j];
      out << '\n';
    }
  } catch (...) {
  }
  out << std::flush;
}

// generator combinators

namespace detail {
template <class T>
class NoShrinkGenerator {
 public:
  NoShrinkGenerator(const Generator<T>& g) : m_gen(g) {}

  T unGen(RngEngine& rng, std::size_t size) { return m_gen.unGen(rng, size); }

  std::vector<T> shrink(const T&) { return std::vector<T>(); }

 private:
  Generator<T> m_gen;
};

template <class T>
class NoShrinkStatelessGenerator {
 public:
  NoShrinkStatelessGenerator(const Generator<T>& g) : m_gen(g) {}

  T unGen(RngEngine& rng, std::size_t size) { return m_gen.unGen(rng, size); }

  std::vector<T> shrink(const T&) { return std::vector<T>(); }

 private:
  StatelessGenerator<T> m_gen;
};
}  // namespace detail

// Generates a value the same way as the input generator, but doesn't shrink.
template <class T>
Generator<T> noShrink(const Generator<T>& g) {
  return detail::NoShrinkGenerator<T>(g);
}
template <class T>
StatelessGenerator<T> noShrink(const StatelessGenerator<T>& g) {
  return detail::NoShrinkStatelessGenerator<T>(g);
}

namespace detail {
template <class T>
class SizedGenerator {
 public:
  SizedGenerator(std::function<Generator<T>(std::size_t)> f)
      : m_genfun(f), m_lastgen(f(0)) {}

  T unGen(RngEngine& rng, std::size_t size) {
    m_lastgen = m_genfun(size);
    return m_lastgen.unGen(rng, size);
  }

  std::vector<T> shrink(const T& x) { return m_lastgen.shrink(x); }

 private:
  const std::function<Generator<T>(std::size_t)> m_genfun;
  Generator<T> m_lastgen;
};
}  // namespace detail

/// Used to create generators that depend on the size parameter.
template <class T>
Generator<T> sized(std::function<Generator<T>(std::size_t)> f) {
  return detail::SizedGenerator<T>(f);
}

namespace detail {
template <class T>
class ResizeGenerator {
 public:
  ResizeGenerator(std::size_t size, const Generator<T>& g)
      : m_size(size), m_gen(g) {}

  T unGen(RngEngine& rng, std::size_t) { return m_gen.unGen(rng, m_size); }

  std::vector<T> shrink(const T& x) { return m_gen.shrink(x); }

 private:
  const std::size_t m_size;
  const Generator<T> m_gen;
};

template <class T>
class ResizeStatelessGenerator {
 public:
  ResizeStatelessGenerator(std::size_t size, const StatelessGenerator<T>& g)
      : m_size(size), m_gen(g) {}

  T unGen(RngEngine& rng, std::size_t) const {
    return m_gen.unGen(rng, m_size);
  }

  std::vector<T> shrink(const T& x) const { return m_gen.shrink(x); }

 private:
  const std::size_t m_size;
  const StatelessGenerator<T> m_gen;
};
}  // namespace detail

/// Overrides the size parameter. Returns a generator which uses the given size
/// instead of the runtime size parameter.
template <class T>
Generator<T> resize(std::size_t size, const Generator<T>& g) {
  return detail::ResizeGenerator<T>(size, g);
}
template <class T>
StatelessGenerator<T> resize(std::size_t size, const StatelessGenerator<T>& g) {
  return detail::ResizeStatelessGenerator<T>(size, g);
}

namespace detail {
template <class Integer>
class ChooseStatelessGenerator {
 public:
  ChooseStatelessGenerator(Integer min, Integer max) : m_min(min), m_max(max) {
    assert(min <= max);
  }

  Integer unGen(RngEngine& rng, std::size_t) const {
    std::uniform_int_distribution<Integer> dist{m_min, m_max};
    return dist(rng);
  }

  std::vector<Integer> shrink(Integer x) const {
    std::vector<Integer> ret;
    ret.reserve(x - m_min);
    if (abs(m_min) <= abs(m_max)) {
      for (Integer i = m_min; i != x; ++i)
        ret.push_back(i);
    } else {
      for (Integer i = m_max; i != x; --i)
        ret.push_back(i);
    }
    return ret;
  }

 private:
  const Integer m_min;
  const Integer m_max;
};
}  // namespace detail

/// Generates a random integer in the range min..max inclusive, requires that
/// min <= max. Shrinks towards smaller absolute values.
template <class Integer>
StatelessGenerator<Integer> choose(Integer min, Integer max) {
  return detail::ChooseStatelessGenerator<Integer>(min, max);
}

namespace detail {
template <class T>
class SuchThatGenerator {
 public:
  SuchThatGenerator(const Generator<T>& g, std::function<bool(T)> pred)
      : m_gen(g), m_pred(pred) {}

  T unGen(RngEngine& rng, std::size_t size) {
    T ret = m_gen.unGen(rng, size);
    if (!m_pred(ret))
      throw std::runtime_error(
          "suchThat: generated value did not satisfy pred");
    return ret;
  }

  std::vector<T> shrink(const T& x) {
    std::vector<T> ret = m_gen.shrink(x);
    ret.erase(std::remove_if(ret.begin(), ret.end(), std::not1(m_pred)),
              ret.end());
    return ret;
  }

 private:
  const Generator<T> m_gen;
  const std::function<bool(T)> m_pred;
};

template <class T>
class SuchThatStatelessGenerator {
 public:
  SuchThatStatelessGenerator(const StatelessGenerator<T>& g,
                             std::function<bool(T)> pred)
      : m_gen(g), m_pred(pred) {}

  T unGen(RngEngine& rng, std::size_t size) const {
    T ret = m_gen.unGen(rng, size);
    if (!m_pred(ret))
      throw std::runtime_error(
          "suchThat: generated value did not satisfy pred");
    return ret;
  }

  std::vector<T> shrink(const T& x) const {
    std::vector<T> ret = m_gen.shrink(x);
    ret.erase(std::remove_if(ret.begin(), ret.end(), std::not1(m_pred)),
              ret.end());
    return ret;
  }

 private:
  const StatelessGenerator<T> m_gen;
  const std::function<bool(T)> m_pred;
};
}  // namespace detail

/// Generates a value that satisfies a predicate.
template <class T, class Pred>
Generator<T> suchThat(const Generator<T>& g, Pred pred) {
  return detail::SuchThatGenerator<T>(g, pred);
}
template <class T, class Pred>
StatelessGenerator<T> suchThat(const StatelessGenerator<T>& g, Pred pred) {
  return detail::SuchThatStatelessGenerator<T>(g, pred);
}

namespace detail {
template <class T>
class OneOfGenerator {
 public:
  OneOfGenerator& operator()(const Generator<T>& g) {
    m_gens.push_back(g);
    return *this;
  }

  T unGen(RngEngine& rng, std::size_t size) {
    std::uniform_int_distribution<std::size_t> dist{0, m_gens.size() - 1};
    m_last_index = dist(rng);
    return m_gens[m_last_index].unGen(rng, size);
  }

  std::vector<T> shrink(const T& x) { return m_gens[m_last_index].shrink(x); }

 private:
  std::vector<Generator<T>> m_gens;
  std::size_t m_last_index;
};
}  // namespace detail

/// Randomly uses one of the given generators.
template <class T>
detail::OneOfGenerator<T> oneof(const Generator<T>& g) {
  detail::OneOfGenerator<T> ret;
  return ret(g);
}

namespace detail {
template <class T>
class FrequencyGenerator {
 public:
  FrequencyGenerator& operator()(std::size_t weight, const Generator<T>& g) {
    if (weight != 0) {
      m_tot += weight;
      m_gens.insert(std::make_pair(m_tot, g));
    }
    return *this;
  }

  T unGen(RngEngine& rng, std::size_t size) {
    std::uniform_int_distribution<std::size_t> dist{1, m_tot};
    std::size_t weight = dist(rng);
    auto it = m_gens.lower_bound(weight);
    if (it == m_gens.end()) {
      throw std::logic_error("frequency: all generators have weight 0");
    } else {
      m_last_index = it->first;
      return it->second.unGen(rng, size);
    }
  }

  std::vector<T> shrink(const T& x) {
    if (m_gens.empty()) {
      return {};
    }
    auto it = m_gens.find(m_last_index);
    assert(it != m_gens.end());
    return it->second.shrink(x);
  }

 private:
  std::map<std::size_t, Generator<T>> m_gens;
  std::size_t m_tot{0};
  std::size_t m_last_index{0};
};
}  // namespace detail

/// Chooses one of the given generators, with a weighted random distribution.
/// Any generator with weight "0" will not be chosen.
template <class T>
detail::FrequencyGenerator<T> frequency(std::size_t f, const Generator<T>& g) {
  detail::FrequencyGenerator<T> ret;
  return ret(f, g);
}

namespace detail {
template <class T>
class ElementsGenerator {
 public:
  ElementsGenerator& operator()(const std::initializer_list<T> x) {
    for (auto& i : x)
      m_elems.push_back(i);
    return *this;
  }

  ElementsGenerator& operator()(const T& x) {
    m_elems.push_back(x);
    return *this;
  }

  T unGen(RngEngine& rng, std::size_t /*size*/) {
    std::uniform_int_distribution<std::size_t> dist{0, m_elems.size() - 1};
    m_last_index = dist(rng);
    return m_elems[m_last_index];
  }

  std::vector<T> shrink(const T&) { return {}; }

 private:
  std::vector<T> m_elems;
  std::size_t m_last_index;
};
}  // namespace detail

/// Generates one of the given values.
template <class T>
detail::ElementsGenerator<T> elements(const std::initializer_list<T> x) {
  detail::ElementsGenerator<T> ret;
  return ret(x);
}

template <class T>
detail::ElementsGenerator<T> elements(const T& x) {
  detail::ElementsGenerator<T> ret;
  return ret(x);
}

namespace detail {
template <class T>
class FixedGenerator {
 public:
  FixedGenerator() : m_last_index_plus_one(0) {}

  FixedGenerator& operator()(const T& x) {
    m_fixed.push_back(x);
    return *this;
  }

  T unGen(RngEngine&, std::size_t) {
    ++m_last_index_plus_one;
    if (m_last_index_plus_one >= m_fixed.size()) {
      throw std::runtime_error("fixed: exhausted fixed test cases");
    } else {
      return m_fixed[m_last_index_plus_one];
    }
  }

  std::vector<T> shrink(const T&) { return std::vector<T>(); }

 private:
  std::size_t m_last_index_plus_one;
  std::vector<T> m_fixed;
};
}  // namespace detail

/// Generates each fixed test case until all have been generated.  Does not
/// shrink. Especially useful with "chain".
template <class T>
detail::FixedGenerator<T> fixed(const T& x) {
  detail::FixedGenerator<T> ret;
  return ret(x);
}

namespace detail {
template <class T>
class ChainGenerator {
 public:
  ChainGenerator() : m_last_index(0) {}

  ChainGenerator& operator()(const Generator<T>& x) {
    m_gens.push_back(x);
    return *this;
  }

  T unGen(RngEngine& rng, std::size_t size) {
    for (m_last_index = 0; m_last_index < m_gens.size(); ++m_last_index) {
      try {
        return m_gens[m_last_index].unGen(rng, size);
      } catch (...) {
      }
    }

    throw std::runtime_error("chain: exhausted all possible generators");
  }

  std::vector<T> shrink(const T& x) { return m_gens[m_last_index].shrink(x); }

 private:
  std::size_t m_last_index;
  std::vector<Generator<T>> m_gens;
};
}  // namespace detail

/// Attempts to generate from each generator, if a generator fails to generate
/// input, the next one is tried.
template <class T>
detail::ChainGenerator<T> chain(const Generator<T>& g) {
  detail::ChainGenerator<T> ret;
  return ret(g);
}

namespace detail {
template <class T, class U>
class ConvertGenerator {
 public:
  ConvertGenerator(std::function<T(U)> f, const Generator<U>& g)
      : m_convert(f), m_gen(g) {}

  T unGen(RngEngine& rng, std::size_t size) {
    m_lastgen_u.clear();
    m_lastgen_t.clear();
    m_lastgen_u.push_back(m_gen.unGen(rng, size));
    m_lastgen_t.push_back(m_convert(m_lastgen_u.back()));
    return m_lastgen_t.back();
  }

  std::vector<T> shrink(const T& x) {
    auto it = std::find(m_lastgen_t.begin(), m_lastgen_t.end(), x);
    assert(it != m_lastgen_t.end());
    U last_u = m_lastgen_u[it - m_lastgen_t.begin()];
    m_lastgen_t.clear();
    m_lastgen_u = m_gen.shrink(last_u);
    m_lastgen_t.reserve(m_lastgen_u.size());
    std::transform(m_lastgen_u.begin(), m_lastgen_u.end(),
                   std::back_inserter(m_lastgen_t), m_convert);
    return m_lastgen_t;
  }

 private:
  std::function<T(U)> m_convert;
  const Generator<U> m_gen;
  std::vector<T> m_lastgen_t;
  std::vector<U> m_lastgen_u;
};
}  // namespace detail

/// Converts a generator of U's into a generator of T's by passing the results
/// through a function which converts U's into T's.
template <class T, class U>
Generator<T> convert(std::function<T(U)> f,
                     const Generator<U>& g = Arbitrary<U>()) {
  return detail::ConvertGenerator<T, U>(f, g);
}

/// Convenience function to aid generic programming, equivalent to convert.
template <class T, class U1>
Generator<T> combine(std::function<T(U1)> f,
                     const Generator<U1>& g1 = Arbitrary<U1>()) {
  return detail::ConvertGenerator<T, U1>(f, g1);
}

namespace detail {
template <class T, class U1, class U2>
class CombineGenerator {
 public:
  CombineGenerator(std::function<T(U1, U2)> f,
                   const Generator<U1>& g1,
                   const Generator<U2>& g2)
      : m_combine(f), m_gen1(g1), m_gen2(g2) {}

  T unGen(RngEngine& rng, std::size_t size) {
    m_lastgen_u1.clear();
    m_lastgen_u2.clear();
    m_lastgen_t.clear();
    m_lastgen_u1.push_back(m_gen1.unGen(rng, size));
    m_lastgen_u2.push_back(m_gen2.unGen(rng, size));
    m_lastgen_t.push_back(m_combine(m_lastgen_u1.back(), m_lastgen_u2.back()));
    return m_lastgen_t.back();
  }

  std::vector<T> shrink(const T& x) {
    auto it = std::find(m_lastgen_t.begin(), m_lastgen_t.end(), x);
    assert(it != m_lastgen_t.end());
    std::size_t dist = it - m_lastgen_t.begin();
    U1 last_u1 = m_lastgen_u1[dist];
    U2 last_u2 = m_lastgen_u2[dist];
    m_lastgen_t.clear();
    std::vector<U1> shrink1 = m_gen1.shrink(last_u1);
    std::vector<U2> shrink2 = m_gen2.shrink(last_u2);
    m_lastgen_u1.assign(shrink1.begin(), shrink1.end());
    m_lastgen_u1.insert(m_lastgen_u1.end(), shrink2.size(), last_u1);
    m_lastgen_u2.assign(shrink1.size(), last_u2);
    m_lastgen_u2.insert(m_lastgen_u2.end(), shrink2.begin(), shrink2.end());

    m_lastgen_t.reserve(m_lastgen_u1.size());
    std::transform(m_lastgen_u1.begin(), m_lastgen_u1.end(),
                   m_lastgen_u2.begin(), std::back_inserter(m_lastgen_t),
                   m_combine);
    return m_lastgen_t;
  }

 private:
  std::function<T(U1, U2)> m_combine;
  const Generator<U1> m_gen1;
  const Generator<U2> m_gen2;
  std::vector<T> m_lastgen_t;
  std::vector<U1> m_lastgen_u1;
  std::vector<U2> m_lastgen_u2;
};
}  // namespace detail

/// Combines generators of U1's and U2's into a generator of T's by passing the
/// results through a function which converts U1's and U2's into T's. Ie. the
/// same as convert for functions taking more than one argument.
template <class T, class U1, class U2>
Generator<T> combine(std::function<T(U1, U2)> f,
                     const Generator<U1>& g1 = Arbitrary<U1>(),
                     const Generator<U2>& g2 = Arbitrary<U2>()) {
  return detail::CombineGenerator<T, U1, U2>(f, g1, g2);
}

namespace detail {
template <class T>
class ListOfStatelessGenerator {
 public:
  ListOfStatelessGenerator(const StatelessGenerator<T>& g) : m_gen(g) {}

  std::vector<T> unGen(RngEngine& rng, std::size_t size) const {
    std::uniform_int_distribution<std::size_t> dist{0, size};
    std::size_t n = dist(rng);
    std::vector<T> ret;
    ret.reserve(n);
    while (n-- > 0)
      ret.push_back(m_gen.unGen(rng, size));
    return ret;
  }

  std::vector<std::vector<T>> shrink(const std::vector<T>& v) const {
    std::vector<std::vector<T>> result;

    // 1) leave one element out (reduces size of new arrays by one)
    for (auto it = v.begin(); it != v.end(); ++it) {
      typename std::vector<T> shortendV;
      shortendV.reserve(v.size() - 1);
      shortendV.insert(shortendV.end(), v.begin(), it);
      shortendV.insert(shortendV.end(), it + 1, v.end());
      assert(shortendV.size() == v.size() - 1);
      result.push_back(std::move(shortendV));
    }

    // 2) shrink each element
    //    (array size stays the same but inner elements shrink)
    for (int i = 0; i < static_cast<int>(v.size()); ++i) {
      typename std::vector<T> shrinkedTypes = Arbitrary<T>::shrink(v[i]);
      for (T& shrinked : Arbitrary<T>::shrink(v[i])) {
        auto copy = v;
        copy[i] = std::move(shrinked);
        result.push_back(std::move(copy));
      }
    }
    return result;
  }

 private:
  const StatelessGenerator<T> m_gen;
};
}  // namespace detail

/// Generates an std::vector of random length. The maximum length depends on the
/// generation size parameter. Shrinks by removing elements from the vector.
template <class T>
StatelessGenerator<std::vector<T>> listOf(
    const StatelessGenerator<T>& g = Arbitrary<T>()) {
  return detail::ListOfStatelessGenerator<T>(g);
}

namespace detail {
template <class T>
class ListOfNonEmptyStatelessGenerator {
 public:
  ListOfNonEmptyStatelessGenerator(const StatelessGenerator<T>& g)
      : m_gen(g), m_vectorGen(listOf<T>(g)) {}

  std::vector<T> unGen(RngEngine& rng, std::size_t size) const {
    std::vector<T> result = m_vectorGen.unGen(rng, size);
    if (result.empty())
      result.emplace_back(m_gen.unGen(rng, size));
    return result;
  }

  std::vector<std::vector<T>> shrink(const std::vector<T>& x) const {
    assert(!x.empty());

    // TODO: should work for now, but uses (too much?!) internal
    //       knowledge of the "ListOfStatelessGenerator"
    //       shrink function implementation
    if (x.size() > 1)
      return m_vectorGen.shrink(x);
    else
      return {};
  }

 private:
  const StatelessGenerator<T> m_gen;
  const StatelessGenerator<std::vector<T>> m_vectorGen;
};
}  // namespace detail

/// Generates a non-empty std::vector of random length. The maximum length
/// depends on the generation size parameter. Shrinks by removing elements from
/// the vector.
template <class T>
StatelessGenerator<std::vector<T>> listOfNonEmpty(
    const StatelessGenerator<T>& g = Arbitrary<T>()) {
  return detail::ListOfNonEmptyStatelessGenerator<T>(g);
}

namespace detail {
template <class T, std::size_t N>
class ArrayOfStatelessGenerator {
 public:
  ArrayOfStatelessGenerator(const StatelessGenerator<T>& g) : m_gen(g) {}

  std::array<T, N> unGen(RngEngine& rng, std::size_t size) const {
    std::array<T, N> result;
    for (size_t i = 0; i < N; i++)
      result[i] = m_gen.unGen(rng, size);
    return result;
  }

  std::vector<std::array<T, N>> shrink(const std::array<T, N>& arr) const {
    std::vector<std::array<T, N>> result;

    // shrink each element
    // (array size stays the same but inner elements shrink)
    for (size_t i = 0; i < N; i++) {
      for (const auto& shrinkedInnerElem : m_gen.shrink(arr[i])) {
        auto copy = arr;
        copy[i] = shrinkedInnerElem;
        result.push_back(std::move(copy));
      }
    }

    return result;
  }

 private:
  const StatelessGenerator<T> m_gen;
};
}  // namespace detail

/// Generates an std::array of fixed length N.
/// As the array's length is fixed, it cannot be shrunken.
/// However, its inner elements can be.
template <class T, std::size_t N>
StatelessGenerator<std::array<T, N>> arrayOf(
    const StatelessGenerator<T>& g = Arbitrary<T>()) {
  return detail::ArrayOfStatelessGenerator<T, N>(g);
}

namespace detail {
template <class T>
class VectorOfStatelessGenerator {
 public:
  VectorOfStatelessGenerator(std::size_t size, const StatelessGenerator<T>& g)
      : m_size(size), m_gen(g) {}

  std::vector<T> unGen(RngEngine& rng, std::size_t size) const {
    std::vector<T> ret;
    std::size_t n = m_size;
    ret.reserve(n);
    while (n-- > 0)
      ret.push_back(m_gen.unGen(rng, size));
    return ret;
  }

  std::vector<std::vector<T>> shrink(const std::vector<T>&) const {
    return std::vector<std::vector<T>>();
  }

 private:
  std::size_t m_size;
  const StatelessGenerator<T> m_gen;
};
}  // namespace detail

/// Generates an std::vector of the given length. Does not shrink. Prefer using
/// listOf or listOfNonEmpty to vectorOf because those can shrink.
template <class T>
StatelessGenerator<std::vector<T>> vectorOf(
    std::size_t size,
    const StatelessGenerator<T>& g = Arbitrary<T>()) {
  return detail::VectorOfStatelessGenerator<T>(size, g);
}

namespace detail {

template <int offset, typename T, typename... Ts>
struct TupleGeneratorHelper_unGen {
  template <typename Generators>
  static std::tuple<T, Ts...> unGen(RngEngine& rng,
                                    std::size_t size,
                                    const Generators& generators) {
    return std::tuple_cat(
        std::make_tuple(std::get<offset>(generators).unGen(rng, size)),
        TupleGeneratorHelper_unGen<offset + 1, Ts...>::unGen(rng, size,
                                                             generators));
  }
};

template <int offset, typename T>
struct TupleGeneratorHelper_unGen<offset, T> {
  template <typename Generators>
  static std::tuple<T> unGen(RngEngine& rng,
                             std::size_t size,
                             const Generators& generators) {
    return std::tuple<T>(std::get<offset>(generators).unGen(rng, size));
  }
};

template <int offset, typename... T>
struct TupleGeneratorHelper_shrink {
  template <typename Generators>
  static void appendShrinks(const std::tuple<T...>& shrinkInput,
                            std::vector<std::tuple<T...>>& shrinkOutput,
                            const Generators& generators) {
    // Compute all shrink combinations for the tuple
    // element at the position "offset".
    // All other elements will not be changed.
    static_assert(offset >= 0 && offset < sizeof...(T), "offset out of bounds");
    auto allLocalShrinks =
        std::get<offset>(generators).shrink(std::get<offset>(shrinkInput));
    for (auto&& localShrink : allLocalShrinks) {
      auto copiedShrinkInput = shrinkInput;
      std::get<offset>(copiedShrinkInput) = std::move(localShrink);
      shrinkOutput.push_back(std::move(copiedShrinkInput));
    }

    // continue recursion (from right to left)
    TupleGeneratorHelper_shrink<offset - 1, T...>::appendShrinks(
        shrinkInput, shrinkOutput, generators);
  }
};

template <typename... T>
struct TupleGeneratorHelper_shrink<-1, T...> {
  template <typename Generators>
  static void appendShrinks(const std::tuple<T...>& /*shrinkInput*/,
                            std::vector<std::tuple<T...>>& /*shrinkOutput*/,
                            const Generators& /*generators*/) {
    // nothing to do (end of recursion)
  }
};

template <typename... T>
struct TupleGenerator {
  TupleGenerator(const Generator<T>&... g) : m_gen(g...) {}

  std::tuple<T...> unGen(RngEngine& rng, std::size_t size) const {
    return TupleGeneratorHelper_unGen<0, T...>::unGen(rng, size, m_gen);
  }

  std::vector<std::tuple<T...>> shrink(
      const std::tuple<T...>& shrinkInput) const {
    std::vector<std::tuple<T...>> shrinkOutput;
    constexpr auto last_index = sizeof...(T) - 1;
    TupleGeneratorHelper_shrink<last_index, T...>::appendShrinks(
        shrinkInput, shrinkOutput, m_gen);
    return shrinkOutput;
  }

 private:
  std::tuple<Generator<T>...> m_gen;
};

}  // namespace detail

template <typename... T>
Generator<std::tuple<T...>> tupleOf(const Generator<T>&... g) {
  return detail::TupleGenerator<T...>(g...);
}

template <typename... T>
Generator<std::tuple<T...>> tupleOf() {
  return detail::TupleGenerator<T...>(Arbitrary<T>()...);
}

namespace detail {
struct null_type {};
}  // namespace detail

class PropertyBase {
 public:
  // By default, it is expected that every check passes.
  virtual bool expect() const { return true; }

  // Should be overwriten by subclasses, as the default
  // implementation is compiler dependent.
  // (However, in practice, gcc and clang produce useful defaults.)
  virtual std::string name() const { return typeid(*this).name(); }

  virtual ~PropertyBase() = default;
};

template <class T0,
          class T1 = detail::null_type,
          class T2 = detail::null_type,
          class T3 = detail::null_type,
          class T4 = detail::null_type>
class Property : public PropertyBase {
 public:
  using Input = std::tuple<T0, T1, T2, T3, T4>;

  Property(const Generator<T0>& g0 = Arbitrary<T0>(),
           const Generator<T1>& g1 = Arbitrary<T1>(),
           const Generator<T2>& g2 = Arbitrary<T2>(),
           const Generator<T3>& g3 = Arbitrary<T3>(),
           const Generator<T4>& g4 = Arbitrary<T4>())
      : m_gen(tupleOf(g0, g1, g2, g3, g4)) {}

  Input generateInput(RngEngine& rng, std::size_t size) const {
    return m_gen.unGen(rng, size);
  }
  std::vector<Input> shrinkInput(const Input& in) const {
    return m_gen.shrink(in);
  }
  bool trivialInput(const Input& in) const {
    return trivial(std::get<0>(in), std::get<1>(in), std::get<2>(in),
                   std::get<3>(in), std::get<4>(in));
  }
  std::string classifyInput(const Input& in) const {
    return classify(std::get<0>(in), std::get<1>(in), std::get<2>(in),
                    std::get<3>(in), std::get<4>(in));
  }
  bool checkInput(const Input& in) const {
    return check(std::get<0>(in), std::get<1>(in), std::get<2>(in),
                 std::get<3>(in), std::get<4>(in));
  }

 private:
  virtual bool trivial(const T0&,
                       const T1&,
                       const T2&,
                       const T3&,
                       const T4&) const {
    return false;
  }

  virtual std::string classify(const T0&,
                               const T1&,
                               const T2&,
                               const T3&,
                               const T4&) const {
    return "";
  }

  virtual bool check(const T0&,
                     const T1&,
                     const T2&,
                     const T3&,
                     const T4&) const = 0;

 private:
  const Generator<Input> m_gen;
};

template <class T0>
class Property<T0,
               detail::null_type,
               detail::null_type,
               detail::null_type,
               detail::null_type> : public PropertyBase {
 public:
  using Input = std::tuple<T0>;

  Property(const Generator<T0>& g0 = Arbitrary<T0>()) : m_gen(tupleOf(g0)) {}

  Input generateInput(RngEngine& rng, std::size_t size) const {
    return m_gen.unGen(rng, size);
  }
  std::vector<Input> shrinkInput(const Input& in) const {
    return m_gen.shrink(in);
  }
  bool trivialInput(const Input& in) const { return trivial(std::get<0>(in)); }
  std::string classifyInput(const Input& in) const {
    return classify(std::get<0>(in));
  }
  bool checkInput(const Input& in) const { return check(std::get<0>(in)); }

 private:
  virtual bool trivial(const T0&) const { return false; }

  virtual std::string classify(const T0&) const { return ""; }

  virtual bool check(const T0&) const = 0;

 private:
  const Generator<Input> m_gen;
};

template <class T0, class T1>
class Property<T0, T1, detail::null_type, detail::null_type, detail::null_type>
    : public PropertyBase {
 public:
  using Input = std::tuple<T0, T1>;

  Property(const Generator<T0>& g0 = Arbitrary<T0>(),
           const Generator<T1>& g1 = Arbitrary<T1>())
      : m_gen(tupleOf(g0, g1)) {}

  Input generateInput(RngEngine& rng, std::size_t size) const {
    return m_gen.unGen(rng, size);
  }
  std::vector<Input> shrinkInput(const Input& in) const {
    return m_gen.shrink(in);
  }
  bool trivialInput(const Input& in) const {
    return trivial(std::get<0>(in), std::get<1>(in));
  }
  std::string classifyInput(const Input& in) const {
    return classify(std::get<0>(in), std::get<1>(in));
  }
  bool checkInput(const Input& in) const {
    return check(std::get<0>(in), std::get<1>(in));
  }

 private:
  virtual bool trivial(const T0&, const T1&) const { return false; }

  virtual std::string classify(const T0&, const T1&) const { return ""; }

  virtual bool check(const T0&, const T1&) const = 0;

 private:
  const Generator<Input> m_gen;
};

template <class T0, class T1, class T2>
class Property<T0, T1, T2, detail::null_type, detail::null_type>
    : public PropertyBase {
 public:
  using Input = std::tuple<T0, T1, T2>;

  Property(const Generator<T0>& g0 = Arbitrary<T0>(),
           const Generator<T1>& g1 = Arbitrary<T1>(),
           const Generator<T2>& g2 = Arbitrary<T2>())
      : m_gen(tupleOf(g0, g1, g2)) {}

  Input generateInput(RngEngine& rng, std::size_t size) const {
    return m_gen.unGen(rng, size);
  }
  std::vector<Input> shrinkInput(const Input& in) const {
    return m_gen.shrink(in);
  }
  bool trivialInput(const Input& in) const {
    return trivial(std::get<0>(in), std::get<1>(in), std::get<2>(in));
  }
  std::string classifyInput(const Input& in) const {
    return classify(std::get<0>(in), std::get<1>(in), std::get<2>(in));
  }
  bool checkInput(const Input& in) const {
    return check(std::get<0>(in), std::get<1>(in), std::get<2>(in));
  }

 private:
  virtual bool trivial(const T0&, const T1&, const T2&) const { return false; }

  virtual std::string classify(const T0&, const T1&, const T2&) const {
    return "";
  }

  virtual bool check(const T0&, const T1&, const T2&) const = 0;

 private:
  const Generator<Input> m_gen;
};

template <class T0, class T1, class T2, class T3>
class Property<T0, T1, T2, T3, detail::null_type> : public PropertyBase {
 public:
  using Input = std::tuple<T0, T1, T2, T3>;

  Property(const Generator<T0>& g0 = Arbitrary<T0>(),
           const Generator<T1>& g1 = Arbitrary<T1>(),
           const Generator<T2>& g2 = Arbitrary<T2>(),
           const Generator<T3>& g3 = Arbitrary<T3>())
      : m_gen(tupleOf(g0, g1, g2, g3)) {}

  Input generateInput(RngEngine& rng, std::size_t size) const {
    return m_gen.unGen(rng, size);
  }
  std::vector<Input> shrinkInput(const Input& in) const {
    return m_gen.shrink(in);
  }
  bool trivialInput(const Input& in) const {
    return trivial(std::get<0>(in), std::get<1>(in), std::get<2>(in),
                   std::get<3>(in));
  }
  std::string classifyInput(const Input& in) const {
    return classify(std::get<0>(in), std::get<1>(in), std::get<2>(in),
                    std::get<3>(in));
  }
  bool checkInput(const Input& in) const {
    return check(std::get<0>(in), std::get<1>(in), std::get<2>(in),
                 std::get<3>(in));
  }

 private:
  virtual bool trivial(const T0&, const T1&, const T2&, const T3&) const {
    return false;
  }

  virtual std::string classify(const T0&,
                               const T1&,
                               const T2&,
                               const T3&) const {
    return "";
  }

  virtual bool check(const T0&, const T1&, const T2&, const T3&) const = 0;

 private:
  const Generator<Input> m_gen;
};

// named
// expectFailure
// classifyWith
// trivializeWith

template <class T0>
std::ostream& printInput(std::ostream& out, const std::tuple<T0>& in) {
  return out << "  0: " << prettyPrint(std::get<0>(in)) << '\n' << std::flush;
}

template <class T0, class T1>
std::ostream& printInput(std::ostream& out, const std::tuple<T0, T1>& in) {
  return out << "  0: " << prettyPrint(std::get<0>(in)) << '\n'
             << "  1: " << prettyPrint(std::get<1>(in)) << '\n'
             << std::flush;
}

template <class T0, class T1, class T2>
std::ostream& printInput(std::ostream& out, const std::tuple<T0, T1, T2>& in) {
  return out << "  0: " << prettyPrint(std::get<0>(in)) << '\n'
             << "  1: " << prettyPrint(std::get<1>(in)) << '\n'
             << "  2: " << prettyPrint(std::get<2>(in)) << '\n'
             << std::flush;
}

template <class T0, class T1, class T2, class T3>
std::ostream& printInput(std::ostream& out,
                         const std::tuple<T0, T1, T2, T3>& in) {
  return out << "  0: " << prettyPrint(std::get<0>(in)) << '\n'
             << "  1: " << prettyPrint(std::get<1>(in)) << '\n'
             << "  2: " << prettyPrint(std::get<2>(in)) << '\n'
             << "  3: " << prettyPrint(std::get<3>(in)) << '\n'
             << std::flush;
}

template <class T0, class T1, class T2, class T3, class T4>
std::ostream& printInput(std::ostream& out,
                         const std::tuple<T0, T1, T2, T3, T4>& in) {
  return out << "  0: " << prettyPrint(std::get<0>(in)) << '\n'
             << "  1: " << prettyPrint(std::get<1>(in)) << '\n'
             << "  2: " << prettyPrint(std::get<2>(in)) << '\n'
             << "  3: " << prettyPrint(std::get<3>(in)) << '\n'
             << "  4: " << prettyPrint(std::get<4>(in)) << '\n'
             << std::flush;
}

template <class T0, class T1, class T2, class T3, class T4, class T5>
std::ostream& printInput(std::ostream& out,
                         const std::tuple<T0, T1, T2, T3, T4, T5>& in) {
  return out << "  0: " << prettyPrint(std::get<0>(in)) << '\n'
             << "  1: " << prettyPrint(std::get<1>(in)) << '\n'
             << "  2: " << prettyPrint(std::get<2>(in)) << '\n'
             << "  3: " << prettyPrint(std::get<3>(in)) << '\n'
             << "  4: " << prettyPrint(std::get<4>(in)) << '\n'
             << "  5: " << prettyPrint(std::get<5>(in)) << '\n'
             << std::flush;
}

template <class T0, class T1, class T2, class T3, class T4, class T5, class T6>
std::ostream& printInput(std::ostream& out,
                         const std::tuple<T0, T1, T2, T3, T4, T5, T6>& in) {
  return out << "  0: " << prettyPrint(std::get<0>(in)) << '\n'
             << "  1: " << prettyPrint(std::get<1>(in)) << '\n'
             << "  2: " << prettyPrint(std::get<2>(in)) << '\n'
             << "  3: " << prettyPrint(std::get<3>(in)) << '\n'
             << "  4: " << prettyPrint(std::get<4>(in)) << '\n'
             << "  5: " << prettyPrint(std::get<5>(in)) << '\n'
             << "  6: " << prettyPrint(std::get<6>(in)) << '\n'
             << std::flush;
}

template <class T0,
          class T1,
          class T2,
          class T3,
          class T4,
          class T5,
          class T6,
          class T7>
std::ostream& printInput(std::ostream& out,
                         const std::tuple<T0, T1, T2, T3, T4, T5, T6, T7>& in) {
  return out << "  0: " << prettyPrint(std::get<0>(in)) << '\n'
             << "  1: " << prettyPrint(std::get<1>(in)) << '\n'
             << "  2: " << prettyPrint(std::get<2>(in)) << '\n'
             << "  3: " << prettyPrint(std::get<3>(in)) << '\n'
             << "  4: " << prettyPrint(std::get<4>(in)) << '\n'
             << "  5: " << prettyPrint(std::get<5>(in)) << '\n'
             << "  6: " << prettyPrint(std::get<6>(in)) << '\n'
             << "  7: " << prettyPrint(std::get<7>(in)) << '\n'
             << std::flush;
}

template <class T0,
          class T1,
          class T2,
          class T3,
          class T4,
          class T5,
          class T6,
          class T7,
          class T8>
std::ostream& printInput(
    std::ostream& out,
    const std::tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8>& in) {
  return out << "  0: " << prettyPrint(std::get<0>(in)) << '\n'
             << "  1: " << prettyPrint(std::get<1>(in)) << '\n'
             << "  2: " << prettyPrint(std::get<2>(in)) << '\n'
             << "  3: " << prettyPrint(std::get<3>(in)) << '\n'
             << "  4: " << prettyPrint(std::get<4>(in)) << '\n'
             << "  5: " << prettyPrint(std::get<5>(in)) << '\n'
             << "  6: " << prettyPrint(std::get<6>(in)) << '\n'
             << "  7: " << prettyPrint(std::get<7>(in)) << '\n'
             << "  8: " << prettyPrint(std::get<8>(in)) << '\n'
             << std::flush;
}

template <class T0,
          class T1,
          class T2,
          class T3,
          class T4,
          class T5,
          class T6,
          class T7,
          class T8,
          class T9>
std::ostream& printInput(
    std::ostream& out,
    const std::tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9>& in) {
  return out << "  0: " << prettyPrint(std::get<0>(in)) << '\n'
             << "  1: " << prettyPrint(std::get<1>(in)) << '\n'
             << "  2: " << prettyPrint(std::get<2>(in)) << '\n'
             << "  3: " << prettyPrint(std::get<3>(in)) << '\n'
             << "  4: " << prettyPrint(std::get<4>(in)) << '\n'
             << "  5: " << prettyPrint(std::get<5>(in)) << '\n'
             << "  6: " << prettyPrint(std::get<6>(in)) << '\n'
             << "  7: " << prettyPrint(std::get<7>(in)) << '\n'
             << "  8: " << prettyPrint(std::get<8>(in)) << '\n'
             << "  9: " << prettyPrint(std::get<9>(in)) << '\n'
             << std::flush;
}


using SeedType = uint32_t;

// By default, a random seed will be used.
// To overwrite the default behaviour, set the environment variable
// "CPPQUICKCHECK_SEED".
constexpr SeedType USE_DEFAULT_SEED = std::numeric_limits<uint32_t>::max();

// If this environment variable is set, its value will overwrite the
// default seed. In other words, no random seed will be generated but
// if a seed has been explicitly set, it will will be used.
constexpr const char* CPPQUICKCHECK_SEED_ENV = "CPPQUICKCHECK_SEED";

// If this environment variable is set, failing tests will be
// looped forever. Useful for attaching a debugger.
constexpr const char* CPPQUICKCHECK_LOOP_FAILING_TESTS_ENV =
    "CPPQUICKCHECK_LOOP_FAILING_TESTS";

enum ResultType {
  QC_SUCCESS,             // All tests succeeded
  QC_GAVE_UP,             // Failed to generate data
  QC_FAILURE,             // A test failed
  QC_NO_EXPECTED_FAILURE  // The property was expected to fail but didn't
};

struct Result {
  ResultType result;
  std::size_t numTests;
  std::multimap<std::size_t, std::string> labels;
  SeedType seed;

  // only used if result is QC_FAILURE
  std::size_t numShrinks;
  std::size_t usedSize;
};

namespace detail {
struct NullOstream : std::ostream {
  NullOstream() : std::ios(nullptr), std::ostream(nullptr) {}
};
}  // namespace detail

template <class T0, class T1, class T2, class T3, class T4>
Result quickCheck(const Property<T0, T1, T2, T3, T4>& prop,
                  std::size_t maxSuccess = 100,
                  std::size_t maxDiscarded = 0,
                  std::size_t maxSize = 0) {
  detail::NullOstream out;
  return quickCheckOutput(prop, out, maxSuccess, maxDiscarded, maxSize);
}

namespace detail {
inline std::multimap<std::size_t, std::string> convertLabels(
    const std::map<std::string, std::size_t>& labelsCollected) {
  std::multimap<std::size_t, std::string> labels;
  for (const auto& elem : labelsCollected) {
    labels.insert(std::make_pair(elem.second, elem.first));
  }
  return labels;
}

inline void outputLabels(
    std::ostream& out,
    std::size_t numSuccess,
    const std::multimap<std::size_t, std::string>& labels) {
  std::size_t cnt = 20;
  for (auto it = labels.rbegin(); it != labels.rend(); --cnt, ++it) {
    if (cnt == 0) {
      out << "  ..." << std::endl;
      break;
    }

    if (it->second != "") {
      out << std::setw(3) << (100 * it->first / numSuccess) << "% "
          << it->second << std::endl;
    }
  }
}

template <class T0, class T1, class T2, class T3, class T4>
std::pair<std::size_t, typename Property<T0, T1, T2, T3, T4>::Input> doShrink(
    const Property<T0, T1, T2, T3, T4>& prop,
    const typename Property<T0, T1, T2, T3, T4>::Input& in,
    std::chrono::duration<double> timeout,
    std::ostream& out = std::cout) {
  using Input = typename Property<T0, T1, T2, T3, T4>::Input;

  std::size_t numShrinks = 0;
  Input shrunk = in;
  const auto start = std::chrono::steady_clock::now();

  auto isTimeoutReached = [start, timeout]() {
    const std::chrono::duration<double> elapsed =
        std::chrono::steady_clock::now() - start;
    return elapsed >= timeout;
  };

  try {
  continueShrinking:
    std::vector<Input> shrinks = prop.shrinkInput(shrunk);
    for (Input input : shrinks) {
      if (isTimeoutReached()) {
        out << "Shrinking timed out...\n";
        break;
      }

      if (!prop.checkInput(input)) {
        shrunk = std::move(input);
        numShrinks++;
        goto continueShrinking;
      }
    }
  } catch (...) {
  }
  return std::make_pair(numShrinks, shrunk);
}
}  // namespace detail

namespace detail {
inline SeedType resolveSeed(SeedType originalSeed = USE_DEFAULT_SEED) {
  if (originalSeed == USE_DEFAULT_SEED) {
    const char* seed_from_env = std::getenv(CPPQUICKCHECK_SEED_ENV);
    if (seed_from_env != nullptr) {
      try {
        return static_cast<uint32_t>(std::stoi(seed_from_env));
      } catch (std::invalid_argument&) {
        std::ostringstream err;
        err << "Failed to parse seed in environment variable "
            << CPPQUICKCHECK_SEED_ENV << ": Got <" << seed_from_env
            << ">, but expected an integer between "
            << "0 and " << (USE_DEFAULT_SEED - 1)
            << ". To use a random seed instead, "
               "unset the environment variable.";
        throw std::invalid_argument{err.str()};
      }
    } else {
      return static_cast<SeedType>(std::time(0));
    }
  } else {
    return originalSeed;
  }
}
}  // namespace detail

constexpr auto DEFAULT_SHRINK_TIMEOUT = std::chrono::seconds(30);
constexpr auto DISABLE_SHRINK_TIMEOUT = std::chrono::seconds::max();

template <class T0, class T1, class T2, class T3, class T4>
Result quickCheckOutput(
    const Property<T0, T1, T2, T3, T4>& prop,
    std::ostream& out = std::cout,
    std::size_t maxSuccess = 100,
    std::size_t maxDiscarded = 0,
    std::size_t maxSize = 0,
    std::chrono::duration<double> shrinkTimeout = DEFAULT_SHRINK_TIMEOUT,
    SeedType seed = USE_DEFAULT_SEED) {
  using Input = typename Property<T0, T1, T2, T3, T4>::Input;

  out << "* Checking property \"" << prop.name() << "\" ..." << std::endl;

  if (maxDiscarded == 0)
    maxDiscarded = maxSuccess * 5;
  if (maxSize == 0)
    maxSize = 100;

  std::map<std::string, std::size_t> labelsCollected;
  std::size_t numSuccess = 0, numDiscarded = 0, numTrivial = 0;

  seed = detail::resolveSeed(seed);
  RngEngine rng{seed};
  while (numSuccess < maxSuccess) {
    std::size_t size = (numSuccess * maxSize + numDiscarded) / maxSuccess;
    Input in = prop.generateInput(rng, size);
    try {
      bool success = false;
      try {
        success = prop.checkInput(in);
      } catch (...) {
        out << "Caught exception checking property...\n";
      }

      if (prop.trivialInput(in))
        ++numTrivial;
      ++labelsCollected[prop.classifyInput(in)];

      if (success) {
        ++numSuccess;
      } else {
        if (prop.expect()) {
          out << "*** Failed! ";
        } else {
          out << "+++ OK, failed as expected. ";
        }

        std::size_t numShrinks = 0;
        try {
          out << "Starting shrinking..." << std::flush;
          std::pair<std::size_t, Input> shrinkRes =
              detail::doShrink(prop, in, shrinkTimeout, out);
          numShrinks = shrinkRes.first;
          in = shrinkRes.second;
          out << "done\n";
        } catch (...) {
          out << "WARN: Aborted shrinking because an exception was thrown\n";
        }
        out << "Falsifiable after " << numSuccess + 1
            << (numSuccess == 0 ? " test" : " tests");
        if (numShrinks > 0) {
          out << " and " << numShrinks
              << (numShrinks == 1 ? " shrink" : " shrinks");
        }
        out << " for input:\n";
        printInput(out, in);
        out << "(To reproduce the test, use " << CPPQUICKCHECK_SEED_ENV << '='
            << seed << ")\n";

        if (prop.expect()) {
          while (getenv(CPPQUICKCHECK_LOOP_FAILING_TESTS_ENV) != nullptr) {
            out << "Looping failing test...\n";
            prop.checkInput(in);
          }
          Result ret;
          ret.result = QC_FAILURE;
          ret.numTests = numSuccess + 1;
          ret.labels = detail::convertLabels(labelsCollected);
          ret.seed = seed;
          ret.numShrinks = numShrinks;
          ret.usedSize = size;
          return ret;
        } else {
          Result ret;
          ret.result = QC_SUCCESS;
          ret.numTests = numSuccess + 1;
          ret.labels = detail::convertLabels(labelsCollected);
          ret.seed = seed;
          return ret;
        }
      }
    } catch (...) {
      if (++numDiscarded >= maxDiscarded) {
        out << "*** Gave up! Passed only " << numSuccess << " tests."
            << std::endl;

        Result ret;
        ret.result = QC_GAVE_UP;
        ret.numTests = numSuccess;
        ret.labels = detail::convertLabels(labelsCollected);
        ret.seed = seed;
        return ret;
      }
    }
  }

  std::multimap<std::size_t, std::string> labels =
      detail::convertLabels(labelsCollected);

  if (prop.expect()) {
    out << "+++ OK, passed " << numSuccess << " tests";
    if (numTrivial != 0)
      out << " (" << (100 * numTrivial / numSuccess) << "% trivial)";
    out << '.' << std::endl;
    detail::outputLabels(out, numSuccess, labels);

    Result ret;
    ret.result = QC_SUCCESS;
    ret.numTests = numSuccess;
    ret.labels = labels;
    ret.seed = seed;
    return ret;
  } else {
    out << "*** Failed! Expected failure but passed " << numSuccess << " tests";
    if (numTrivial != 0)
      out << " (" << (100 * numTrivial / numSuccess) << "% trivial)";
    out << '.' << std::endl;
    detail::outputLabels(out, numSuccess, labels);

    Result ret;
    ret.result = QC_NO_EXPECTED_FAILURE;
    ret.numTests = numSuccess;
    ret.labels = labels;
    ret.seed = seed;
    return ret;
  }
}
}; // namespace cppqc


namespace pretty_print
{
    namespace detail
    {
        // SFINAE type trait to detect whether T::const_iterator exists.

        struct sfinae_base
        {
            using yes = char;
            using no  = yes[2];
        };

        template <typename T>
        struct has_const_iterator : private sfinae_base
        {
        private:
            template <typename C> static yes & test(typename C::const_iterator*);
            template <typename C> static no  & test(...);
        public:
            static const bool value = sizeof(test<T>(nullptr)) == sizeof(yes);
            using type =  T;
        };

        template <typename T>
        struct has_begin_end : private sfinae_base
        {
        private:
            template <typename C>
            static yes & f(typename std::enable_if<
                std::is_same<decltype(static_cast<typename C::const_iterator(C::*)() const>(&C::begin)),
                             typename C::const_iterator(C::*)() const>::value>::type *);

            template <typename C> static no & f(...);

            template <typename C>
            static yes & g(typename std::enable_if<
                std::is_same<decltype(static_cast<typename C::const_iterator(C::*)() const>(&C::end)),
                             typename C::const_iterator(C::*)() const>::value, void>::type*);

            template <typename C> static no & g(...);

        public:
            static bool const beg_value = sizeof(f<T>(nullptr)) == sizeof(yes);
            static bool const end_value = sizeof(g<T>(nullptr)) == sizeof(yes);
        };

    }  // namespace detail


    // Holds the delimiter values for a specific character type

    template <typename TChar>
    struct delimiters_values
    {
        using char_type = TChar;
        const char_type * prefix;
        const char_type * delimiter;
        const char_type * postfix;
    };


    // Defines the delimiter values for a specific container and character type

    template <typename T, typename TChar>
    struct delimiters
    {
        using type = delimiters_values<TChar>;
        static const type values; 
    };

    template <typename T>
    struct quoting_handler
    {
        static std::string toString(const T& x)
        {
            std::ostringstream out;
            out << x;
            return out.str();
        }
    };

    template <>
    struct quoting_handler<std::string>
    {
        static std::string toString(const std::string& x)
        {
            return '\"' + x + '\"';
        }
    };

    template <class T>
    std::string quote_if_necessary(const T& x)
    {
        return quoting_handler<T>::toString(x);
    }

    // Functor to print containers. You can use this directly if you want
    // to specificy a non-default delimiters type. The printing logic can
    // be customized by specializing the nested template.

    template <typename T,
              typename TChar = char,
              typename TCharTraits = ::std::char_traits<TChar>,
              typename TDelimiters = delimiters<T, TChar>>
    struct print_container_helper
    {
        using delimiters_type = TDelimiters;
        using ostream_type = std::basic_ostream<TChar, TCharTraits>;

        template <typename U>
        struct printer
        {
            static void print_body(const U & c, ostream_type & stream)
            {
                using std::begin;
                using std::end;

                auto it = begin(c);
                const auto the_end = end(c);

                if (it != the_end)
                {
                    for ( ; ; )
                    {
		        stream << quote_if_necessary(*it);

                        if (++it == the_end)
                            break;

                        if (delimiters_type::values.delimiter != NULL)
                            stream << delimiters_type::values.delimiter;
                    }
                }
            }
        };

        print_container_helper(const T & container)
        : container_(container)
        { }

        inline void operator()(ostream_type & stream) const
        {
            if (delimiters_type::values.prefix != NULL)
                stream << delimiters_type::values.prefix;

            printer<T>::print_body(container_, stream);

            if (delimiters_type::values.postfix != NULL)
                stream << delimiters_type::values.postfix;
        }

    private:
        const T & container_;
    };

    // Specialization for pairs

    template <typename T, typename TChar, typename TCharTraits, typename TDelimiters>
    template <typename T1, typename T2>
    struct print_container_helper<T, TChar, TCharTraits, TDelimiters>::printer<std::pair<T1, T2>>
    {
        using ostream_type = print_container_helper<T, TChar, TCharTraits, TDelimiters>::ostream_type;

        static void print_body(const std::pair<T1, T2> & c, ostream_type & stream)
        {
            stream << quote_if_necessary(c.first);
            if (print_container_helper<T, TChar, TCharTraits, TDelimiters>::delimiters_type::values.delimiter != NULL)
                stream << print_container_helper<T, TChar, TCharTraits, TDelimiters>::delimiters_type::values.delimiter;
            stream << quote_if_necessary(c.second);
        }
    };

    // Specialization for tuples

    template <typename T, typename TChar, typename TCharTraits, typename TDelimiters>
    template <typename ...Args>
    struct print_container_helper<T, TChar, TCharTraits, TDelimiters>::printer<std::tuple<Args...>>
    {
        using ostream_type = print_container_helper<T, TChar, TCharTraits, TDelimiters>::ostream_type;
        using element_type = std::tuple<Args...>;

        template <std::size_t I> struct Int { };

        static void print_body(const element_type & c, ostream_type & stream)
        {
            tuple_print(c, stream, Int<0>());
        }

        static void tuple_print(const element_type &, ostream_type &, Int<sizeof...(Args)>)
        {
        }

        static void tuple_print(const element_type & c, ostream_type & stream,
                                typename std::conditional<sizeof...(Args) != 0, Int<0>, std::nullptr_t>::type)
        {
            stream << quote_if_necessary(std::get<0>(c));
            tuple_print(c, stream, Int<1>());
        }

        template <std::size_t N>
        static void tuple_print(const element_type & c, ostream_type & stream, Int<N>)
        {
            if (print_container_helper<T, TChar, TCharTraits, TDelimiters>::delimiters_type::values.delimiter != NULL)
                stream << print_container_helper<T, TChar, TCharTraits, TDelimiters>::delimiters_type::values.delimiter;

            stream << quote_if_necessary(std::get<N>(c));

            tuple_print(c, stream, Int<N + 1>());
        }
    };

    // Prints a print_container_helper to the specified stream.

    template<typename T, typename TChar, typename TCharTraits, typename TDelimiters>
    inline std::basic_ostream<TChar, TCharTraits> & operator<<(
        std::basic_ostream<TChar, TCharTraits> & stream,
        const print_container_helper<T, TChar, TCharTraits, TDelimiters> & helper)
    {
        helper(stream);
        return stream;
    }


    // Basic is_container template; specialize to derive from std::true_type for all desired container types

    template <typename T>
    struct is_container : public std::integral_constant<bool,
                                                        detail::has_const_iterator<T>::value &&
                                                        detail::has_begin_end<T>::beg_value  &&
                                                        detail::has_begin_end<T>::end_value> { };

    template <typename T, std::size_t N>
    struct is_container<T[N]> : std::true_type { };

    template <std::size_t N>
    struct is_container<char[N]> : std::false_type { };

    template <typename T>
    struct is_container<std::valarray<T>> : std::true_type { };

    template <typename T1, typename T2>
    struct is_container<std::pair<T1, T2>> : std::true_type { };

    template <typename ...Args>
    struct is_container<std::tuple<Args...>> : std::true_type { };


    // Default delimiters

    template <typename T> struct delimiters<T, char> { static const delimiters_values<char> values; };
    template <typename T> const delimiters_values<char> delimiters<T, char>::values = { "[", ", ", "]" };
    template <typename T> struct delimiters<T, wchar_t> { static const delimiters_values<wchar_t> values; };
    template <typename T> const delimiters_values<wchar_t> delimiters<T, wchar_t>::values = { L"[", L", ", L"]" };


    // Delimiters for (multi)set and unordered_(multi)set

    template <typename T, typename TComp, typename TAllocator>
    struct delimiters< ::std::set<T, TComp, TAllocator>, char> { static const delimiters_values<char> values; };

    template <typename T, typename TComp, typename TAllocator>
    const delimiters_values<char> delimiters< ::std::set<T, TComp, TAllocator>, char>::values = { "{", ", ", "}" };

    template <typename T, typename TComp, typename TAllocator>
    struct delimiters< ::std::set<T, TComp, TAllocator>, wchar_t> { static const delimiters_values<wchar_t> values; };

    template <typename T, typename TComp, typename TAllocator>
    const delimiters_values<wchar_t> delimiters< ::std::set<T, TComp, TAllocator>, wchar_t>::values = { L"{", L", ", L"}" };

    template <typename T, typename TComp, typename TAllocator>
    struct delimiters< ::std::multiset<T, TComp, TAllocator>, char> { static const delimiters_values<char> values; };

    template <typename T, typename TComp, typename TAllocator>
    const delimiters_values<char> delimiters< ::std::multiset<T, TComp, TAllocator>, char>::values = { "{", ", ", "}" };

    template <typename T, typename TComp, typename TAllocator>
    struct delimiters< ::std::multiset<T, TComp, TAllocator>, wchar_t> { static const delimiters_values<wchar_t> values; };

    template <typename T, typename TComp, typename TAllocator>
    const delimiters_values<wchar_t> delimiters< ::std::multiset<T, TComp, TAllocator>, wchar_t>::values = { L"{", L", ", L"}" };

    template <typename T, typename THash, typename TEqual, typename TAllocator>
    struct delimiters< ::std::unordered_set<T, THash, TEqual, TAllocator>, char> { static const delimiters_values<char> values; };

    template <typename T, typename THash, typename TEqual, typename TAllocator>
    const delimiters_values<char> delimiters< ::std::unordered_set<T, THash, TEqual, TAllocator>, char>::values = { "{", ", ", "}" };

    template <typename T, typename THash, typename TEqual, typename TAllocator>
    struct delimiters< ::std::unordered_set<T, THash, TEqual, TAllocator>, wchar_t> { static const delimiters_values<wchar_t> values; };

    template <typename T, typename THash, typename TEqual, typename TAllocator>
    const delimiters_values<wchar_t> delimiters< ::std::unordered_set<T, THash, TEqual, TAllocator>, wchar_t>::values = { L"{", L", ", L"}" };

    template <typename T, typename THash, typename TEqual, typename TAllocator>
    struct delimiters< ::std::unordered_multiset<T, THash, TEqual, TAllocator>, char> { static const delimiters_values<char> values; };

    template <typename T, typename THash, typename TEqual, typename TAllocator>
    const delimiters_values<char> delimiters< ::std::unordered_multiset<T, THash, TEqual, TAllocator>, char>::values = { "{", ", ", "}" };

    template <typename T, typename THash, typename TEqual, typename TAllocator>
    struct delimiters< ::std::unordered_multiset<T, THash, TEqual, TAllocator>, wchar_t> { static const delimiters_values<wchar_t> values; };

    template <typename T, typename THash, typename TEqual, typename TAllocator>
    const delimiters_values<wchar_t> delimiters< ::std::unordered_multiset<T, THash, TEqual, TAllocator>, wchar_t>::values = { L"{", L", ", L"}" };


    // Delimiters for pair and tuple

    template <typename T1, typename T2> struct delimiters<std::pair<T1, T2>, char> { static const delimiters_values<char> values; };
    template <typename T1, typename T2> const delimiters_values<char> delimiters<std::pair<T1, T2>, char>::values = { "(", ", ", ")" };
    template <typename T1, typename T2> struct delimiters< ::std::pair<T1, T2>, wchar_t> { static const delimiters_values<wchar_t> values; };
    template <typename T1, typename T2> const delimiters_values<wchar_t> delimiters< ::std::pair<T1, T2>, wchar_t>::values = { L"(", L", ", L")" };

    template <typename ...Args> struct delimiters<std::tuple<Args...>, char> { static const delimiters_values<char> values; };
    template <typename ...Args> const delimiters_values<char> delimiters<std::tuple<Args...>, char>::values = { "(", ", ", ")" };
    template <typename ...Args> struct delimiters< ::std::tuple<Args...>, wchar_t> { static const delimiters_values<wchar_t> values; };
    template <typename ...Args> const delimiters_values<wchar_t> delimiters< ::std::tuple<Args...>, wchar_t>::values = { L"(", L", ", L")" };


    // Type-erasing helper class for easy use of custom delimiters.
    // Requires TCharTraits = std::char_traits<TChar> and TChar = char or wchar_t, and MyDelims needs to be defined for TChar.
    // Usage: "cout << pretty_print::custom_delims<MyDelims>(x)".

    struct custom_delims_base
    {
	virtual ~custom_delims_base() = default;
        virtual std::ostream & stream(::std::ostream &) = 0;
        virtual std::wostream & stream(::std::wostream &) = 0;
    };

    template <typename T, typename Delims>
    struct custom_delims_wrapper : custom_delims_base
    {
        custom_delims_wrapper(const T & t_) : t(t_) { }

	std::ostream & stream(std::ostream & s) override
        {
            return s << print_container_helper<T, char, std::char_traits<char>, Delims>(t);
        }

	std::wostream & stream(std::wostream & s) override
        {
            return s << print_container_helper<T, wchar_t, std::char_traits<wchar_t>, Delims>(t);
        }

    private:
        const T & t;
    };

    template <typename Delims>
    struct custom_delims
    {
        template <typename Container>
        custom_delims(const Container & c) : base(new custom_delims_wrapper<Container, Delims>(c)) { }

        std::unique_ptr<custom_delims_base> base;
    };

    template <typename TChar, typename TCharTraits, typename Delims>
    inline std::basic_ostream<TChar, TCharTraits> & operator<<(std::basic_ostream<TChar, TCharTraits> & s, const custom_delims<Delims> & p)
    {
        return p.base->stream(s);
    }


    // A wrapper for a C-style array given as pointer-plus-size.
    // Usage: std::cout << pretty_print_array(arr, n) << std::endl;

    template<typename T>
    struct array_wrapper_n
    {
	using const_iterator = const T *;
	using value_type = T;

        array_wrapper_n(const T * const a, size_t n) : _array(a), _n(n) { }
        inline const_iterator begin() const { return _array; }
        inline const_iterator end() const { return _array + _n; }

    private:
        const T * const _array;
        size_t _n;
    };


    // A wrapper for hash-table based containers that offer local iterators to each bucket.
    // Usage: std::cout << bucket_print(m, 4) << std::endl;  (Prints bucket 5 of container m.)

    template <typename T>
    struct bucket_print_wrapper
    {
	using const_iterator = typename T::const_local_iterator;
	using size_type = typename T::size_type;

        const_iterator begin() const
        {
            return m_map.cbegin(n);
        }

        const_iterator end() const
        {
            return m_map.cend(n);
        }

        bucket_print_wrapper(const T & m, size_type bucket) : m_map(m), n(bucket) { }

    private:
        const T & m_map;
        const size_type n;
    };

}   // namespace pretty_print


// Global accessor functions for the convenience wrappers

template<typename T>
inline pretty_print::array_wrapper_n<T> pretty_print_array(const T * const a, size_t n)
{
    return pretty_print::array_wrapper_n<T>(a, n);
}

template <typename T> pretty_print::bucket_print_wrapper<T>
bucket_print(const T & m, typename T::size_type n)
{
    return pretty_print::bucket_print_wrapper<T>(m, n);
}


// Main magic entry point: An overload snuck into namespace std.
// Can we do better?

namespace std
{
    // Prints a container to the stream using default delimiters

    template<typename T, typename TChar, typename TCharTraits>
    inline typename enable_if< ::pretty_print::is_container<T>::value,
                              basic_ostream<TChar, TCharTraits> &>::type
    operator<<(basic_ostream<TChar, TCharTraits> & stream, const T & container)
    {
        return stream << ::pretty_print::print_container_helper<T, TChar, TCharTraits>(container);
    }
}

#endif // CPPQC_HPP