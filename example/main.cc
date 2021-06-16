/**
 *
 *  Copyright:  Copyright (c) 2021, ISSuh
 *
 */

#include "../include/sm.hpp"

// class Type {
//  public:
//   virtual ~Type() {}
//   virtual void* allocate() const = 0;
//   virtual void* cast(void* obj) const = 0;
// };

// template <typename T>
// class TypeImpl : public Type {
//  public:
//   virtual void* allocate() const { return new T; }
//   virtual void* cast(void* obj) const { return static_cast<T*>(obj); }
// };

enum class Color {
  RED,
  BLUE,
  PINK,
  GREEN,

  START = RED,
  DONE = GREEN
};

// Color red_color(sm::Arguments arg) {
//   std::cout << "Color RED State\n";
//   return Color::BLUE;
// }

// class Test {
//  public:
//   Color blue_color(sm::Arguments arg) {
//     std::cout << "Color BLUE State\n";
//     return Color::PINK;
//   }
// };

// Color green_color(sm::Arguments arg) {
//   std::cout << "Color GREEN State\n";
//   return Color::DONE;
// }


int main() {
  std::cout << "Simple FSM Example\n";

  sm::Arguments args;
  sm::StateMachine<Color> machine(args);

  // machine.on(Color::RED, &red_color, args);

  // Test test;
  // machine.on(Color::BLUE, &Test::blue_color, &test, args);
  // machine.on(Color::GREEN, &green_color, args);

  machine.on(Color::GREEN, [](sm::Arguments arg) {
    std::cout << "Color PINK State\n";
    return Color::GREEN;
  }, args);

  machine.run();
}