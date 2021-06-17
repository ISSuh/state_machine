/**
 *
 *  Copyright:  Copyright (c) 2021, ISSuh
 *
 */

#include "../state_machine.hpp"

enum class Color {
  RED,
  BLUE,
  GREEN,

  // 'START' is first state on state machine
  // it is must defined on user state
  START = RED,

  // 'LAST' is first state on state machine
  // it is must defined on user state
  DONE = GREEN
};

Color red_color(sm::Arguments& arg) {
  std::cout << "Color RED State\n";
  return Color::BLUE;
}

class Test {
 public:
  Color blue_color(sm::Arguments& arg) {
    std::cout << "Color BLUE State\n";
    return Color::GREEN;
  }
};

Color green_color(sm::Arguments& arg) {
  std::cout << "Color GREEN State\n";
  return Color::DONE;
}


int main() {
  std::cout << "Simple FSM Example\n\n";

  sm::Arguments args;
  sm::StateMachine<Color> machine(args);

  // function mapping with user define State
  machine.on(Color::RED, &red_color);

  Test test;
  machine.on(Color::BLUE, &Test::blue_color, &test);

  machine.on(Color::GREEN, [](sm::Arguments& arg) {
    std::cout << "Color PINK State\n";
    return Color::DONE;
  });

  machine.run();
}