#ifndef ZEN_INTO_HPP
#define ZEN_INTO_HPP

#include <memory>

namespace zen {

  template<typename T, typename Enabler = void>
  struct IntoType {
    template<typename R>
    static R apply(T value) {
      return value;
    }
  };

  template<typename T>
  struct IntoPtrType : IntoType<T, typename std::enable_if<std::is_pointer_v<T>>::type> { 
    template<typename R>
    static R apply(T value) {
      return std::static_pointer_cast<R>(value);
    }
  };

} // of namespace zen

#endif // #ifndef ZEN_INTO_HPP
