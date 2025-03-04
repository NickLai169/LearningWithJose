#include <iostream>
#include <type_traits>

/*
Notes:
- requirements:
    - log(...) method that takes variable number of arguments (Args&&) and prints them
        - if the argument is integral print with "[int]" prefix
        - if the argument is a floating-point, print with "[float]" prefix
        - otherwise, just print
    - the log(...) method should be templated, or separate funcrtion for integral vs float with SFINAE, std::enable_if, or concepts
*/


class MetaLoggerSFINAE {
    template<typename T>
    std::enable_if_t<std::is_integral<T>::value, void>
    print(T&& val) { std::cout << "[int] " << val; }

    template<typename T>
    std::enable_if_t<std::is_floating_point_v<T>, void>
    print(T&& val) { std::cout << "[float] " << val; }

    template<typename T, std::enable_if_t<!std::is_arithmetic_v<T>, int> = 0>
    void print(T&& val) { std::cout << val; };
public:
    template<typename... Args>
    void log(Args&&... args) {
        (print(std::forward<Args>(args)), ...);
    }
};

template<typename T>
concept IsFloat = std::is_floating_point_v<T>;

template<typename T>
concept IsInt = std::is_integral<T>::value;

template<typename T>
concept IsOther = !IsInt<T> && !IsFloat<T>;

class MetaLoggerConcept {
    template<IsFloat T>
    void print(T&& val) { std::cout << "[int] " << val; }

    template<IsInt T>
    void print(T&& val) { std::cout << "[float] " << val; }

    template<IsOther T>
    void print(T&& val) { std::cout << val; };
public:
    template<typename... Args>
    void log(Args&&... args) {
        (print(std::forward<Args>(args)), ...);
    }
};



int main() {
    MetaLoggerConcept logger;
    logger.log(1, 2.0, "hello");

    std::cout << "end run" << std::endl;
    return 0;
}