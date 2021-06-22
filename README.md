# State Machine

## Overview

Implement header only simple state machine using c++ 11.

## Feature
- Sequential state
- Sub state
- Concurrent state

## Build
This project include *\<thread\>*.\
So if you use it on POSIX platform, you must link pthraed(-lpthread).

[example_make_file](./example/Makefile)
```Makefile
# Makefile example
CC = g++
CXXFLAGS = -Wall -g -std=c++11

all: clean sequence_example

sequence_example : sequence_example.cc
	$(CC) $(CXXFLAGS) $^ -o $@ $(DEFINE) -lpthread

.PHONY: clean
clean:
	rm -f sequence_example
```

## Example
## User define state
You can declare state using *enum class*.

```cpp
// user define state
enum class Color {
  // 'START' element is first state on state machine.
  // it is must defined at first element on user state.
  START,

  RED,
  BLUE,
  GREEN

  // 'DONE' element is last state on state machine
  // it is must defined at last element on user state.
  DONE
};
```

### Basic state machine
*sm::StateMachine* represent a sinlge state machine and it is implemented template.\
So you can use state machine using *__sm::StateMachine\<UserDefineState\>__*.

The *sm::StateMachine* can regist normal funcion, member function, lamdba function.\
Basic funcion prototype is *__sm::States\<UserDefineState\>(const sm::Arguments&)__*

Basically, the state machine is runed on __single thread__.

```cpp
#include "state_machine.hpp"

// user define state
enum class Color {
  START,

  RED = START,
  BLUE,
  GREEN

  DONE = GREEN
};

sm::States<Color> red_color(const sm::Arguments& arg) {
  std::cout << "----- Color::RED State -----\n";

  // return next state
  return {{Color::BLUE}};
}

class MyBlueState {
 public:
  sm::States<Color> blue_color(const sm::Arguments& arg) {
    std::cout << "----- Color::BLUE State -----\n";
    return {{SubColor::GREEN}};
  }
};

int main() {
  sm::StateMachine<Color> machine; 
  
  // regist function on Color::RED state
  machine.On(Color::RED, &red_color);

  // regist member function on Color::blue state
  MyBlueState my_blud_state;
  machine.On(Color::BLUE, &blue_color, &my_blud_state);

  // regist lambda function on Color::GREEN state
  machine.On(Color::GREEN, [](sm::Arguments& arg) -> sm::States<Color> {
    std::cout << "----- Color::GREEN State -----\n";

    // the state machine finished when returned "UserDefineState::DONE"
    return {{Color::DONE}};
  });
  
  machine.Excute();
}
```

### Assign arguments
Can regist arguments on state machine using *sm::Arguments* class.

```cpp
class MyArgyment {
 public:
  void hello() { std::cout << "hello " << val++ << std::endl; }

 private:
  int val = 0;
};

sm::States<MyState> Work(const sm::Arguments& arg) {
  // get varialble pointer using key string
  int* count = arg.At<int>("count");
  double* array = arg.At<double>("array");
  std::vector<int>* container = arg.At<std::vector<int>>("container");
  MyArgyment* my_arg = arg.At<MyArgyment>("obj");

  my_arg->hello();

  return {{MyState::DONE}};
}

int main() {
  sm::Arguments args;
  
  // regist varialbe though passing key string and address 
  int count = 0;
  args.Allocate("obj", &count);

  // regist array
  double array[5] = {0.0, };
  args.Allocate("array", array);

  // regist STL container
  std::vector<int> container;
  args.Allocate("container", &container);

  // regist object
  MyArgyment my_arg;
  args.Allocate("obj", &my_arg);

  // regist arguments on state machine
  sm::StateMachine<MyState> machine(args);

  ...
}
```

### Regist sub state machine on parent
Can regist sub state machine on other state machine.

```cpp
#include "state_machine.hpp"

enum class Color {
  START,

  RED = START,
  BLUE,
  GREEN

  DONE = GREEN
};

enum class SubColor {
  START,

  WHITE = START,
  BLACK,

  DONE = BLACK,
};

sm::States<Color> red_color(const sm::Arguments& arg) {
  std::cout << "----- Color::RED State -----\n";
  return {{Color::BLUE}};
}

sm::States<Color> green_color(const sm::Arguments& arg) {
  std::cout << "----- Color::GREEN State -----\n";
  return {{Color::DONE}};
}

sm::States<SubColor> white_color(const sm::Arguments& arg) {
  std::cout << "----- SubColor::WHITE State -----\n";
  return {{SubColor::BLACK}};
}

sm::States<SubColor> black_color(const sm::Arguments& arg) {
  std::cout << "----- SubColor::BLACK State -----\n";
  return {{SubColor::DONE}};
}

int main() {

  sm::Arguments args;
  sm::StateMachine<Color> machine(args); 

  machine.On(Color::RED, &red_color);

  // declare sub state machine
  sm::StateMachine<SubColor> sub_machine(args);
  sub_machine.On(SubColor::WHITE, &white_color);
  sub_machine.On(SubColor::BLACK, &black_color);

  // regist sub state machine on main state machine
  machine.RegistSubState(
    Color::BLUE,  // the state in which the sub state machine
                  // will be registered.

    &sub_machine, // address of sub state machine.

    Color::GREEN  // the next state of the parent state machine
                  // after sub state machine completion.
  );


  machine.On(Color::GREEN, &green_color);
  
  machine.Excute();
}
```

## TODO
 - Visualize state machine using graphviz
 - Redesign cunrrency state