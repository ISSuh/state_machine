/**
 *
 *  Copyright:  Copyright (c) 2021, ISSuh
 *
 */

#ifndef STATE_MACHINE_HPP_
#define STATE_MACHINE_HPP_

#include <atomic>
#include <functional>
#include <future>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <utility>

namespace sm {

class ArgumentBase {
 public:
  virtual ~ArgumentBase() {}

  template <typename T>
  T* get();
};

template <typename T>
class Argument : public ArgumentBase {
 public:
  explicit Argument(T* value) : value_(value) {}
  virtual ~Argument() {}

  T* get() { return value_.get(); }

 private:
  std::unique_ptr<T> value_;
};

template <typename T>
T* ArgumentBase::get() {
  return dynamic_cast<Argument<T>&>(*this).get();
}

class Arguments {
 public:
  template <typename T>
  void allocate(const std::string& key, T* value) {
    if (args_.find(key) != args_.end()) {
#if defined(STATE_MACHINE_DEBUG)
      std::cout << "[" << key << "] already exist\n";
#endif
      return;
    }

    args_.insert({key, static_cast<ArgumentBase*>(new Argument<T>(value))});
  }

  template <typename T>
  T* at(const std::string& key) {
    return args_[key]->get<T>();
  }

  int32_t number_of_args() const { return args_.size(); }
  bool find(const std::string& key) { return args_.find(key) != args_.end(); }

 private:
  std::map<std::string, ArgumentBase*> args_;
};

template <typename S>
class State {
 public:
  explicit State(S state) : state_(state) {}

  void change(S state) { state_ = state; }
  const S now() const { return state_; }

  int32_t to_int() const { return static_cast<int32_t>(state_); }
  int32_t number_of_state() const { return static_cast<int32_t>(S::DONE); }

 private:
  S state_;
};

template <typename S>
class StateMachine {
 public:
  explicit StateMachine(Arguments args)
      : args_(args), state_(State<S>(S::START)), running_(false) {}

  template <typename F>
  void on(S state, F&& func) {
    if (worker_.find(state) != worker_.end()) {
#if defined(STATE_MACHINE_DEBUG)
      std::cout << "state already exist\n";
#endif
      return;
    }

    auto task = std::bind(std::forward<F>(func), std::ref(args_));
    worker_.insert({state, task});
  }

  template <typename F, typename T>
  void on(S state, F&& func, T&& class_obj) {
    if (worker_.find(state) != worker_.end()) {
      std::cout << "Already exist\n";
      return;
    }

    auto task = std::bind(std::forward<F>(func), std::forward<T>(class_obj),
                          std::ref(args_));
    worker_.insert({state, task});
  }

  void run() {
    running_.store(true);
    while (running_.load()) {
      if (state_.now() == S::DONE) {
        running_.store(false);
      }

      if (worker_.find(state_.now()) == worker_.end()) {
        running_.store(false);
      }

      S next_state = worker_[state_.now()]();
      state_.change(next_state);
    }
  }

 private:
  Arguments args_;
  State<S> state_;
  std::map<S, std::function<S()>> worker_;

  std::atomic_bool running_ = false;
};

}  // namespace sm

#endif  // STATE_MACHINE_HPP_
