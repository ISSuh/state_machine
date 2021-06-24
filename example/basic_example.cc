/**
 *
 *  Copyright:  Copyright (c) 2021, ISSuh
 *
 */

#include "../state_machine.hpp"

enum class Color {
  // 'START' is first state on state machine
  // it is must defined on user state
  START,

  RED = START,
  BLUE,
  GREEN,

  // 'LAST' is first state on state machine
  // it is must defined on user state
  DONE
};

std::string ColorStateToString(Color state) {
  switch (state) {
  case Color::RED:
    return "Color::RED";
  case Color::BLUE:
    return "Color::BLUE";
  case Color::GREEN:
    return "Color::GREEN";
  default:
    return "";
  }
}

enum class SubColor {
  START,

  WHITE = START,
  BLACK,

  DONE
};

std::string SubColorStateToString(SubColor state) {
  switch (state) {
  case SubColor::WHITE:
    return "Color::RED";
  case SubColor::BLACK:
    return "Color::BLUE";
  default:
    return "";
  }
}

class MyArgument {
 public:
  void hello() { std::cout << "hello " << val++ << std::endl; }

 private:
  int val = 0;
};

sm::States<Color> red_color(sm::Arguments& arg) {
  std::cout << "----- Color::RED State -----\n";

  // get varialble pointer using key string
  int* count = arg.at<int>("count");
  double* array = arg.at<double>("array");
  std::vector<int>* container = arg.at<std::vector<int>>("container");
  MyArgument* my_arg = arg.at<MyArgument>("obj");

  return {{Color::BLUE}};
}

class MySubState {
 public:
  sm::States<SubColor> white_color(sm::Arguments& arg) {
    std::cout << "----- SubColor::WHITE State -----\n";
    MyArgument* my_arg = arg.at<MyArgument>("obj");
    my_arg->hello();
    return {{SubColor::BLACK}};
  }

  sm::States<SubColor> black_color(sm::Arguments& arg) {
    std::cout << "----- SubColor::BLACK State -----\n";
    MyArgument* my_arg = arg.at<MyArgument>("obj");
    my_arg->hello();
    return {{SubColor::DONE}};
  }
};

int main() {
  std::cout << "Simple State Machine example\n\n";

  sm::Arguments args;

  // regist varialbe though passing key string and address
  int count = 0;
  args.insert("count", &count);

  // regist array
  double array[5] = {0.0, };
  args.insert("array", array);

  // regist STL container
  std::vector<int> container;
  args.insert("container", &container);

  // regist object
  MyArgument my_arg;
  args.insert("obj", &my_arg);

  // if key is already exist, it will be ignored
  int count_dup = 1;
  args.insert("count", &count_dup);

  // create state machine and regist arguments
  sm::StateMachine<Color> machine(&args);

  // function mapping with user define State
  machine.On(Color::RED, &red_color);

  MySubState my_state;
  sm::StateMachine<SubColor> sub_machine(&args);

  sub_machine.On(SubColor::WHITE, &MySubState::white_color, &my_state);
  sub_machine.On(SubColor::BLACK, &MySubState::black_color, &my_state);

  machine.RegistSubState(Color::BLUE, &sub_machine, Color::GREEN);

  machine.On(Color::GREEN, [](sm::Arguments& arg) -> sm::States<Color> {
    std::cout << "----- Color::GREEN State -----\n";
    MyArgument* my_arg = arg.at<MyArgument>("obj");
    my_arg->hello();
    return {{Color::DONE}};
  });

  machine.Excute();
}
