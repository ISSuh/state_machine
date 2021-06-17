/**
 * 
 *  Copyright:  Copyright (c) 2021, ISSuh
 * 
 */

#ifndef STATE_MACHINE_HPP_
#define STATE_MACHINE_HPP_

#include <iostream>
#include <string>
#include <map>
#include <functional>
#include <future>
#include <atomic>
#include <utility>
#include <memory>

namespace sm {

class ArgumentBase {
 public:
  virtual ~ArgumentBase() {}

  template<typename T>
  T get();

  template<typename T>
  void set(T value);
};

template<typename T>
class Argument : public ArgumentBase {
 public:
  Argument() {}
  virtual ~Argument() {}

  T get() {
    return value_;
  }

  void set(T value) {
    value_ = value;
  }

 private:
  T value_;
};

template<typename T>
T ArgumentBase::get() {
  return dynamic_cast<Argument<T>&>(*this).get();
}

template<typename T>
void ArgumentBase::set(T value) {
  return dynamic_cast<Argument<T>&>(*this).set(value);
}

class Arguments {
 public:
  Arguments() = default;
  ~Arguments() = default;

  template<typename T>
  void allocate(const std::string& key, T value) {
    if (args_.find(key) != args_.end()) {
      return;
    }

    Argument<T> *arg = new Argument<T>();
    arg->set(value);

    args_.insert({key, arg});
  }

  template<typename T>
  T at(const std::string& key) {
    return args_[key]->get<T>();
  }

 private:
  std::map<std::string, ArgumentBase*> args_;
};

template<typename S>
class State {
 public:
  explicit State(S state) : state_(state) {}
  ~State() = default;

  void change(S state) { state_ = state; }
  const S now() const { return state_; }

  int32_t to_int() const { return static_cast<int32_t>(state_); }
  int32_t number_of_state() const { return static_cast<int32_t>(S::DONE); }

 private:
  S state_;
};

template<typename S>
class StateMachine {
 public:
  explicit StateMachine(Arguments args)
    : args_(args), state_(State<S>(S::START)), running_(false) {}

  template<typename F>
  void on(S state, F&& func) {
    if (worker_.find(state) != worker_.end()) {
      std::cout << "Already exist\n";
      return;
    }

    auto task = std::bind(std::forward<F>(func), std::ref(args_));
    worker_.insert({state, task});
  }

  template<typename F, typename T>
  void on(S state, F&& func, T&& class_obj) {
    if (worker_.find(state) != worker_.end()) {
      std::cout << "Already exist\n";
      return;
    }

    auto task = std::bind(std::forward<F>(func), std::forward<T>(class_obj), std::ref(args_));
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
