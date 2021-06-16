/**
 * 
 *  Copyright:  Copyright (c) 2021, ISSuh
 * 
 */

#ifndef SM_HPP_
#define SM_HPP_

#include <iostream>
#include <string>
#include <map>
#include <functional>
#include <future>
#include <atomic>
#include <utility>
#include <memory>

namespace sm {

class Arguments {
 public:
 private:
};

template<typename S>
class State {
 public:
  void set(S state) {
    state_ = state;
  }

  S now() {
    return state_;
  }

 public:
  // class Itertor {
  //   explicit Itertor(int value) : value_(value) {}

  //   S operator*(void) const {
  //     return static_cast<S>(value_);
  //   }

  //   void operator++(void) {
  //     ++value_;
  //   }

  //   bool operator==(Itertor rhs) {
  //     return value_ == rhs.value_;
  //   }

  //   bool operator!=(Itertor rhs) {
  //     return value_ != rhs.value_;
  //   }

  //  private:
  //   int32_t value_;
  // };

 private:
  S state_;
};

// template<typename S>
// typename State<S>::Iterator begin(State<S>) {
//   return typename State<S>::Iterator(static_cast<int32_t>(S::START));
// }

// template<typename S>
// typename State<S>::Iterator end(State<S>) {
//   return typename State<S>::Iterator(static_cast<int32_t>(S::DONE) + 1);
// }

template<typename S>
class StateMachine {
 public:
  explicit StateMachine(Arguments args)
    : args_(args), running_(false) {}

  // template<class F, class ...Args>
  // void on(S state, F&& func,  Args&&... args) {
  //   if (worker_.find(state) != worker_.end()) {
  //     std::cout << "Already exist\n";
  //     return;
  //   }

  //   auto task = std::bind(std::forward<F>(func), std::forward<Args>(args)...);
  //   worker_.insert({state, task});
  // }

  template<class F>
  void on(S state, F& func, Arguments args) {
    if (worker_.find(state) != worker_.end()) {
      std::cout << "Already exist\n";
      return;
    }

    auto task = std::bind(std::forward<F>(func), std::ref(args));
    worker_.insert({state, task});
  }

  void run() {
    running_.store(true);
    while (running_.load()) {
      if (state_.now() == S::DONE) {
        running_.store(true);
      }

      S next_state = worker_[state_.now()]();
      state_.set(next_state);
    }
  }

 private:
  Arguments args_;
  State<S> state_;
  std::map<S, std::function<S()>> worker_;

  std::atomic_bool running_ = false;
};

}  // namespace sm

#endif  // SM_HPP_
