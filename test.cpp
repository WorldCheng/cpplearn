#include <ostream>
#include <print> // std::println

template <typename T> void out(T a) { std::println("{}", a); }

int main() {
  out(1);
  out("昆明你好！！");
}