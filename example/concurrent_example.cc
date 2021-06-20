/**
 *
 *  Copyright:  Copyright (c) 2021, ISSuh
 *
 */

#include <vector>
#include "../state_machine.hpp"

enum class MyState : uint32_t {
  CONNECT,
  WORK,
  CLOSE,

  START = CONNECT,
  DONE = CLOSE
};

enum class MyWork : uint32_t {
  WORK_1,
  WORK_2,
  WORK_DONE,

  START,
  DONE = WORK_DONE,
};

class MyWorker {
 public:
  sm::States<MyWork> Start(sm::Arguments& arg) {
    return {{MyWork::WORK_1}, {MyWork::WORK_2}};
  }

  sm::States<MyWork> Work1(sm::Arguments& arg) {
    std::cout << "----MyWork::WORK_1----\n";
    for (auto i = 0 ; i < 20 ; ++i) {
      std::cout << " WORK_1 = " << i << std::endl;
    }

    return {{MyWork::WORK_DONE}};
  }

  sm::States<MyWork> Work2(sm::Arguments& arg) {
    std::cout << "----MyWork::WORK_2----\n";
    for (auto i = 0 ; i < 20 ; ++i) {
      std::cout << " WORK_2 = " << i << std::endl;
    }

    return {{MyWork::WORK_DONE}};
  }

  sm::States<MyWork> WorkDone(sm::Arguments& arg) {
    std::cout << "----MyWork::WORK_DONE----\n";
    return {{MyWork::DONE}};
  }
};

sm::States<MyState> connect_func(sm::Arguments& arg) {
  std::cout << "----MyState::CONNECT----\n";
  return {{MyState::WORK}};
}

sm::States<MyState> close_func(sm::Arguments& arg) {
  std::cout << "----MyState::CLOSE----\n";
  return {{MyState::DONE}};
}

int main() {
  std::cout << "Simple State Machine example\n\n";

  sm::StateMachine<MyState> machine;
  machine.On(MyState::CONNECT, &connect_func);

  MyWorker worker;
  sm::StateMachine<MyWork> concurncy_machine;
  concurncy_machine.On(MyWork::START, &MyWorker::Start, &worker);
  concurncy_machine.On(MyWork::WORK_1, &MyWorker::Work1, &worker);
  concurncy_machine.On(MyWork::WORK_2, &MyWorker::Work2, &worker);
  concurncy_machine.On(MyWork::WORK_DONE, &MyWorker::WorkDone, &worker);

  machine.RegistSubState(MyState::WORK, &concurncy_machine, MyState::CLOSE);

  machine.On(MyState::CLOSE, &close_func);

  machine.Run();
}