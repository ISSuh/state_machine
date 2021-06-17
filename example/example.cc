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

class MyArgyment {
 public:
  void hello() { std::cout << "hello " << val++ << std::endl; }
 private:
  int val = 0;
};

Color red_color(sm::Arguments& arg) {
  int32_t* count = arg.at<int32_t>("count");
  double* array = arg.at<double>("array");
  std::vector<int32_t>* vec = arg.at<std::vector<int32_t>>("vec");
  MyArgyment* my_arg = arg.at<MyArgyment>("obj");

  std::cout << "----- Color RED State -----\n";
  std::cout << "count : " << *count++ << std::endl;
  std::cout << "array : ";
  for (int32_t i  = 0 ; i < 10 ; ++i) {
    std::cout << array[i]++ << " ";
  }
  std::cout << std::endl;

  std::cout << "vec : ";
  for (int32_t& i : *vec) {
    std::cout << i++ << " ";
  }
  std::cout << std::endl;

  my_arg->hello();

  return Color::BLUE;
}

class MyState {
 public:
  Color blue_color(sm::Arguments& arg) {
    int32_t* count = arg.at<int32_t>("count");
    double* array = arg.at<double>("array");
    std::vector<int32_t>* vec = arg.at<std::vector<int32_t>>("vec");
    MyArgyment* my_arg = arg.at<MyArgyment>("obj");

    std::cout << "----- Color BLUE State -----\n";
    std::cout << "count : " << *count++ << std::endl;
    std::cout << "array : ";
    for (int32_t i  = 0 ; i < 10 ; ++i) {
      std::cout << array[i]++ << " ";
    }
    std::cout << std::endl;

    std::cout << "vec : ";
    for (int32_t& i : *vec) {
      std::cout << i++ << " ";
    }
    std::cout << std::endl;

    my_arg->hello();

    return Color::GREEN;
  }
};

int main() {
  std::cout << "Simple FSM Example\n\n";

  // create arguments and regist
  sm::Arguments args;

  int32_t count = 0;
  args.allocate("count", &count);

  double array[10] = {0.0, };
  args.allocate("array", array);

  std::vector<int32_t> vec = {1, 2, 3, 4, 5};
  args.allocate("vec", &vec);

  MyArgyment my_arg;
  args.allocate("obj", &my_arg);

  // create state machine and regist arguments
  sm::StateMachine<Color> machine(args);

  // function mapping with user define State
  machine.on(Color::RED, &red_color);

  MyState my_state;
  machine.on(Color::BLUE, &MyState::blue_color, &my_state);

  machine.on(Color::GREEN, [](sm::Arguments& arg) {
    int32_t* count = arg.at<int32_t>("count");
    double* array = arg.at<double>("array");
    std::vector<int32_t>* vec = arg.at<std::vector<int32_t>>("vec");
    MyArgyment* my_arg = arg.at<MyArgyment>("obj");

    std::cout << "----- Color GREEN State -----\n";
    std::cout << "count : " << *count++ << std::endl;
    std::cout << "array : ";
    for (int32_t i  = 0 ; i < 10 ; ++i) {
      std::cout << array[i]++ << " ";
    }
    std::cout << std::endl;

    std::cout << "vec : ";
    for (int32_t& i : *vec) {
      std::cout << i++ << " ";
    }
    std::cout << std::endl;

    my_arg->hello();

    return Color::DONE;
  });

  machine.run();
}