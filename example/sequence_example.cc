/**
 *
 *  Copyright:  Copyright (c) 2021, ISSuh
 *
 */

#include <vector>
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

enum class SubColor {
  WHITE,
  BLACK,

  START = WHITE,
  DONE = BLACK,
};

class MyArgyment {
 public:
  void hello() { std::cout << "hello " << val++ << std::endl; }

 private:
  int val = 0;
};

sm::States<Color> red_color(const sm::Arguments& arg) {
  std::cout << "----- Color::RED State -----\n";
  MyArgyment* my_arg = arg.At<MyArgyment>("obj");
  my_arg->hello();
  return {{Color::BLUE}};
}

class MySubState {
 public:
  sm::States<SubColor> white_color(const sm::Arguments& arg) {
    std::cout << "----- SubColor::WHITE State -----\n";
    MyArgyment* my_arg = arg.At<MyArgyment>("obj");
    my_arg->hello();
    return {{SubColor::BLACK}};
  }

  sm::States<SubColor> black_color(const sm::Arguments& arg) {
    std::cout << "----- SubColor::BLACK State -----\n";
    MyArgyment* my_arg = arg.At<MyArgyment>("obj");
    my_arg->hello();
    return {{SubColor::DONE}};
  }
};

int main() {
  std::cout << "Simple FSM Example\n\n";

  // create arguments and regist
  MyArgyment my_arg;

  sm::Arguments args;
  args.Allocate("obj", &my_arg);

  // create state machine and regist arguments
  sm::StateMachine<Color> machine(args);

  // function mapping with user define State
  machine.On(Color::RED, &red_color);

  MySubState my_state;
  sm::StateMachine<SubColor> sub_machine(args);

  sub_machine.On(SubColor::WHITE, &MySubState::white_color, &my_state);
  sub_machine.On(SubColor::BLACK, &MySubState::black_color, &my_state);

  machine.RegistSubState(Color::BLUE, &sub_machine, Color::GREEN);

  machine.On(Color::GREEN, [](sm::Arguments& arg) -> sm::States<Color> {
    std::cout << "----- Color::GREEN State -----\n";
    MyArgyment* my_arg = arg.At<MyArgyment>("obj");
    my_arg->hello();
    return {{Color::DONE}};
  });

  machine.Excute();
}
