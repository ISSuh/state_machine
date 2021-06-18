/**
 *
 *  Copyright:  Copyright (c) 2021, ISSuh
 *
 */

#ifndef STATE_MACHINE_HPP_
#define STATE_MACHINE_HPP_

#include <atomic>
#include <functional>
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
  T* get() const {}
};

template <typename T>
class Argument : public ArgumentBase {
 public:
  explicit Argument(T* value) : value_(value) {}
  virtual ~Argument() {}

  T* get() const { return value_.get(); }

 private:
  std::unique_ptr<T> value_;
};

class Arguments {
 public:
  template <typename T>
  void Allocate(const std::string& key, T* value) {
    if (args_.find(key) != args_.end()) {
#if defined(STATE_MACHINE_DEBUG)
      std::cout << "[" << key << "] already exist\n";
#endif
      return;
    }

    args_.insert({key, static_cast<ArgumentBase*>(new Argument<T>(value))});
  }

  template <typename T>
  T* At(const std::string& key) {
    if (args_.find(key) == args_.end()) {
#if defined(STATE_MACHINE_DEBUG)
      std::cout << "[" << key << "] invalid\n";
#endif
      return nullptr;
    }

    Argument<T>* arg = dynamic_cast<Argument<T>*>(args_[key]);
    return arg->get();
  }

  int32_t NumberOfArgs() const { return args_.size(); }
  bool Find(const std::string& key) { return args_.find(key) != args_.end(); }

 private:
  std::map<std::string, ArgumentBase*> args_;
};

template <typename S>
class State {
 public:
  explicit State(S state) : state_(state) {}

  void Change(S state) { state_.store(state); }
  const S Now() const { return state_.load(); }

  bool IsStartState() const { return state_.load() == S::START; }
  bool IsDoneState() const { return state_.load() == S::DONE; }

 private:
  std::atomic<S> state_;
};

template <typename S>
class StateMachine {
 public:
  explicit StateMachine(Arguments args)
      : args_(args), state_(S::START), running_(false) {}

  template <typename F>
  void On(S state, F&& func) {
    if (HasTask(state)) {
#if defined(STATE_MACHINE_DEBUG)
      std::cout << "state already exist\n";
#endif
      return;
    }

    auto task = std::bind(std::forward<F>(func), std::ref(args_));
    worker_.insert({state, task});
  }

  template <typename F, typename T>
  void On(S state, F&& func, T&& class_obj) {
    if (HasTask(state)) {
#if defined(STATE_MACHINE_DEBUG)
      std::cout << "state already exist\n";
#endif
      return;
    }

    auto task = std::bind(std::forward<F>(func), std::forward<T>(class_obj),
                          std::ref(args_));
    worker_.insert({state, task});
  }

  void Run() {
    ChangeRunning(true);

    while (IsRunning()) {
      if (!HasTask(state_.Now())) {
        break;
      }

      if (state_.IsDoneState()) {
        ChangeRunning(false);
      }

      S next_state = worker_[state_.Now()]();
      state_.Change(next_state);
    }
  }

  bool HasTask(S state) const { return worker_.find(state) != worker_.end(); }
  bool IsRunning() const { return running_.load(); }

  S NowState() const { return state_.Now(); }

 private:
  void ChangeRunning(bool run) { running_.store(run); }

 private:
  Arguments args_;
  State<S> state_;
  std::map<S, std::function<S()>> worker_;

  std::atomic_bool running_ = false;
};

}  // namespace sm

#endif  // STATE_MACHINE_HPP_
