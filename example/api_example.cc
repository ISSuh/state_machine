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

void PrintArguments(const sm::Arguments& arg) {
  int32_t* count = arg.At<int32_t>("count");
  double* array = arg.At<double>("array");
  std::vector<int32_t>* vec = arg.At<std::vector<int32_t>>("vec");
  MyArgyment* my_arg = arg.At<MyArgyment>("obj");

  std::cout << "count : " << *count++ << std::endl;
  std::cout << "array : ";
  for (int32_t i = 0; i < 10; ++i) {
    std::cout << array[i]++ << " ";
  }
  std::cout << std::endl;

  std::cout << "vec : ";
  for (int32_t& i : *vec) {
    std::cout << i++ << " ";
  }
  std::cout << std::endl;

  my_arg->hello();
}

Color red_color(sm::Arguments& arg) {
  std::cout << "----- Color RED State -----\n";
  PrintArguments(arg);
  return Color::BLUE;
}

class MyState {
 public:
  SubColor white_color(sm::Arguments& arg) {
    std::cout << "----- Sub Color WHITE State -----\n";
    PrintArguments(arg);
    return SubColor::BLACK;
  }

  SubColor black_color(sm::Arguments& arg) {
    std::cout << "----- Sub Color BLACK State -----\n";
    PrintArguments(arg);
    return SubColor::DONE;
  }

  Color convert() {
    return Color::GREEN;
  }
};

int main() {
  std::cout << "Simple FSM Example\n\n";

  // create arguments and regist
  sm::Arguments args;

  int32_t count = 0;
  double array[10] = {0.0, };
  std::vector<int32_t> vec = {1, 2, 3, 4, 5};
  MyArgyment my_arg;

  args.Allocate("count", &count);
  args.Allocate("array", array);
  args.Allocate("vec", &vec);
  args.Allocate("obj", &my_arg);

  // create state machine and regist arguments
  sm::StateMachine<Color> machine(args);

  // function mapping with user define State
  machine.On(Color::RED, &red_color);

  MyState my_state;
  sm::StateMachine<SubColor> sub_machine(args);

  sub_machine.On(SubColor::WHITE, &MyState::white_color, &my_state);
  sub_machine.On(SubColor::BLACK, &MyState::black_color, &my_state);

  machine.RegistSubState(Color::BLUE, &sub_machine, Color::GREEN);

  machine.On(Color::GREEN, [](sm::Arguments& arg) {
    std::cout << "----- Color GREEN State -----\n";
    PrintArguments(arg);
    return Color::DONE;
  });

  machine.Run();
}