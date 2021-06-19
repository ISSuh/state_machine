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
  T* At(const std::string& key) const {
    if (args_.find(key) == args_.end()) {
#if defined(STATE_MACHINE_DEBUG)
      std::cout << "[" << key << "] invalid\n";
#endif
      return nullptr;
    }

    ArgumentBase* arg_base = args_.at(key);
    Argument<T>* arg = dynamic_cast<Argument<T>*>(arg_base);
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
class Task {
 public:
  using TaskFunction = std::function<S()>;
  Task() = default;
  ~Task() = default;

  template <typename F>
  explicit Task(F&& func) : task_(func) {}

  const int32_t Call() const { return static_cast<int32_t>(task_()); }

 private:
  TaskFunction task_;
};

template <typename S>
class StateMachine {
 public:
  explicit StateMachine(Arguments args)
      : args_(args),
        state_(S::START),
        running_(false) {}

  template <typename F>
  void On(S state, F&& func) {
    if (HasTask(state)) {
#if defined(STATE_MACHINE_DEBUG)
      std::cout << "state already exist\n";
#endif
      return;
    }

    auto task_func = std::bind(std::forward<F>(func), std::ref(args_));
    worker_.emplace(state, task_func);
  }

  template <typename F, typename T>
  void On(S state, F&& func, T&& class_obj) {
    if (HasTask(state)) {
#if defined(STATE_MACHINE_DEBUG)
      std::cout << "state already exist\n";
#endif
      return;
    }

    auto task_func = std::bind(std::forward<F>(func),
                               std::forward<T>(class_obj), std::ref(args_));
    worker_.emplace(state, task_func);
  }

  template<typename T>
  void RegistSubState(S state, StateMachine<T>* sub_state_machine, S output_state) {
    if (HasTask(state)) {
#if defined(STATE_MACHINE_DEBUG)
      std::cout << "state already exist\n";
#endif
      return;
    }

    // TODO: should add deleter for memory 
    SubStateMachine<T>* sub_machine = new SubStateMachine<T>(sub_state_machine, output_state);
    auto task_func = std::bind(&SubStateMachine<T>::Run, sub_machine);
    worker_.emplace(state, task_func);
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

      const Task<S>& task = worker_[state_.Now()];
      S next_state = static_cast<S>(task.Call());
      state_.Change(next_state);
    }
  }

  bool HasTask(S state) const { return worker_.find(state) != worker_.end(); }
  bool IsRunning() const { return running_.load(); }

  S NowState() const { return state_.Now(); }

 private:
  void ChangeRunning(bool run) { running_.store(run); }

  template <typename SubState>
  class SubStateMachine {
   public:
    SubStateMachine(StateMachine<SubState>* state_machine, S output_state)
      : state_machine_(state_machine), output_state_(output_state) {}

    S Run() {
      state_machine_->Run();
      return output_state_;
    }

   private:
    StateMachine<SubState>* state_machine_;
    S output_state_;
  };

 private:
  Arguments args_;
  State<S> state_;
  std::map<S, Task<S>> worker_;

  std::atomic_bool running_ = false;
};

}  // namespace sm

#endif  // STATE_MACHINE_HPP_
