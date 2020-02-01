/// \file zen/either.hpp
/// \brief Encapsulation for computations that may fail.
///
/// ### Working With Computations That May Fail
///
/// Often, you find yourself interfacing with external systems, such as a network
/// service or the file system. Doing operations on these objects can result in
/// failures, e.g. an `ENOENT` returned from a call to `stat()`.
///
/// In C, it is very common to store the actual result in one of the function's
/// parameters and return an error code, like so:
///
/// ```c
/// int read_some(const char* filename, char* output) {
///   int fd, error;
///   fd = open(in, O_RDONLY);
///   if (fd < 0) {
///     return -1;
///   }
///   error = read(fd, output, 4);
///   if (error < 0) {
///     return -1;
///   }
///   return 0;
/// }
/// ```
///
/// In C++ another common idiom is returning a `nullptr` whenever a heap-allocated
/// object could not be created. These approaches have obvious drawbacks. In the
/// case of returning an error code instead of the result, we have to make sure our
/// variable can be kept as a reference, leading to more code.
///
/// The generic solution to this problem is to introduce a new type, called
/// `Either`, that can hold both a result and an error code, without wasting
/// precious memory. This is exactly what `zen::Either<L, R>` was made for.
///
/// ```cpp
/// Either<int, std::string> writeSome(std::string filename) {
///   int fd = open(in, O_RDONLY);
///   if (fd < 0) {
///     return zen::left(-1)
///   }
///   char buf[4];
///   read(fd, buf, 4);
///   return zen::right(std::string(output, 4));
/// }
/// ```
///
/// We can further improve upon our code snippet by declaring an `enum` that lists
/// all possible errors that might occur. The errors might even be full classes
/// using virtual inheritance; something which we'll see later on.
///
/// ```cpp
/// enum class Error {
///   OpenFailed,
///   ReadFailed,
/// }
///
/// Either<int, std::string> writeSome(std::string filename) {
///   int fd = open(in, O_RDONLY);
///   if (fd < 0) {
///     return zen::left(Error::OpenFailed)
///   }
///   char buf[4];
///   if (read(fd, buf, 4) < 0) {
///     return zen::left(Error::ReadFailed)
///   }
///   return zen::right(std::string(output, 4));
/// }
/// ```
///
/// Finally, we encapsulate our error type in a custom `Result`-type that will be
/// used thoughout our application:
///
/// ```cpp
/// template<typename T>
/// using Result = Either<Error, T>;
/// ```
///
/// That's it! You've learned how to write simple C++ code the Zen way!

#ifndef ZEN_EITHER_HPP
#define ZEN_EITHER_HPP

#include "zen/macros.h"

namespace zen {

  template<typename L>
  struct left_t {
    L value;
    inline left_t(L value): value(std::move(value)) {};
  };

  template<typename R>
  struct right_t {
    R value;
    inline right_t(R&& value): value(std::move(value)) {};
    inline right_t(const R& value): value(value) {};
  };

  template<>
  struct right_t<void> {};

  template<typename L, typename R>
  class either {

    template<typename L2, typename R2>
    friend
    class either;

    struct dummy {
    };

    union {

      L left_value;
      R right_value;

    };

    bool has_right_v;

  public:

    template<typename L2>
    inline either(left_t<L2>&& value): left_value(std::move(value.value)), has_right_v(false) {};

    template<typename R2>
    inline either(right_t<R2>&& value): right_value(std::move(value.value)), has_right_v(true) {};

    either(either &&other) : has_right_v(std::move(other.has_right_v)) {
      if (has_right_v) {
        new(&right_value)R(std::move(other.right_value));
      } else {
        new(&left_value)L(std::move(other.left_value));
      }
    }

//    either(const either &other) : has_right_v(other.has_right_v) {
//      if (has_right_v) {
//        new(&right_value)R(other.right_value);
//      } else {
//        new(&left_value)L(other.left_value);
//      }
//    }

    template<typename L2, typename R2>
    either(either<L2, R2>&& other): has_right_v(std::move(other.has_right_v)) {
      if (has_right_v) {
        new(&right_value)R(std::move(other.right_value));
      } else {
        new(&left_value)L(std::move(other.left_value));
      }
    }

    template<typename L2, typename R2>
    either(const either<L2, R2> &other): has_right_v(other.has_right_v) {
      if (has_right_v) {
        new(&right_value)R(other.right_value);
      } else {
        new(&left_value)L(other.left_value);
      }
    }

    R* operator->() {
      ZEN_ASSERT(has_right_v);
      return &right_value;
    }

    R &operator*() {
      ZEN_ASSERT(has_right_v);
      return right_value;
    }

    bool is_left() { return !has_right_v; }

    bool is_right() { return has_right_v; }

    L &left() {
      ZEN_ASSERT(!has_right_v);
      return left_value;
    }

    R &right() {
      ZEN_ASSERT(has_right_v);
      return right_value;
    }

    ~either() {
      if (has_right_v) {
        right_value.~R();
      } else {
        left_value.~L();
      }
    }

  };

  template<typename L>
  class either<L, void> {

    struct dummy {};

    union data_t {
      L left;
      dummy right;
    };

    data_t data;

    bool has_left;

  public:

    inline either(left_t<L> data): data(data.value),  has_left(true) {};
    inline either(right_t<void>): has_left(false) {};

    either(either&& other): has_left(other.has_left) {
      if (other.has_left) {
        data.left = std::move(other.data.left);
      }
    }

    either(const either& other): has_left(other.has_left) {
      if (other.has_left) {
        data.left = other.data.left;
      }
    }

    bool is_left() { return has_left; }
    bool is_right() { return !has_left; }

    L& left() const {
      ZEN_ASSERT(has_left);
      return data.left;
    }

    ~either() {
      if (has_left) {
        data.left.~L();
      }
    }

  };

  template<typename L>
  left_t<L> left(const L& value) {
    return left_t<L> { value };
  }

  template<typename L>
  left_t<L> left(L&& value) {
    return left_t<L> { std::move(value) };
  }

  template<typename R>
  right_t<R> right(R& value) {
    return right_t<R> { value };
  }

  template<typename R>
  right_t<R> right(R&& value) {
    return right_t<R> { std::move(value) };
  }

#define ZEN_TRY(value) \
  { \
    if (value.is_left()) { \
      return ::zen::left(std::move(value.left())); \
    } \
  }

#define ZEN_TRY2(expr) \
  { \
    auto zen__either__result = (expr); \
    if (zen__either__result.is_left()) { \
      return ::zen::left(std::move(zen__either__result.left())); \
    } \
  }

#define ZEN_UNWRAP(value) \
  if (value.is_left()) { \
    ZEN_PANIC("unwrapping a left-valued object"); \
  }

} // of namespace zen

#endif // ZEN_EITHER_HPP
