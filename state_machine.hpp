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

  template <typename ArgumentType>
  ArgumentType* get() const {}
};

template <typename ArgumentType>
class Argument : public ArgumentBase {
 public:
  explicit Argument(ArgumentType* value) : value_(value) {}
  virtual ~Argument() {}

  ArgumentType* get() const { return value_.get(); }

 private:
  std::unique_ptr<ArgumentType> value_;
};

class Arguments {
 public:
  template <typename ArgumentType>
  void Allocate(const std::string& key, ArgumentType* value);

  template <typename ArgumentType>
  ArgumentType* At(const std::string& key) const;

  int32_t NumberOfArgs() const { return args_.size(); }
  bool Find(const std::string& key) { return args_.find(key) != args_.end(); }

 private:
  std::map<std::string, ArgumentBase*> args_;
};

template <typename ArgumentType>
void Arguments::Allocate(const std::string& key, ArgumentType* value) {
  if (args_.find(key) != args_.end()) {
#if defined(STATE_MACHINE_DEBUG)
    std::cout << "[" << key << "] already exist\n";
#endif
    return;
  }

  args_.insert(
      {key, static_cast<ArgumentBase*>(new Argument<ArgumentType>(value))});
}

template <typename ArgumentType>
ArgumentType* Arguments::At(const std::string& key) const {
  if (args_.find(key) == args_.end()) {
#if defined(STATE_MACHINE_DEBUG)
    std::cout << "[" << key << "] invalid\n";
#endif
    return nullptr;
  }

  ArgumentBase* arg_base = args_.at(key);
  Argument<ArgumentType>* arg = dynamic_cast<Argument<ArgumentType>*>(arg_base);
  return arg->get();
}

template <typename UserState>
class State {
 public:
  explicit State(UserState state) : state_(state) {}

  void Change(UserState state) { state_.store(state); }
  const UserState Now() const { return state_.load(); }

  bool IsStartState() const { return state_.load() == UserState::START; }
  bool IsDoneState() const { return state_.load() == UserState::DONE; }

 private:
  std::atomic<UserState> state_;
};

template <typename UserState>
class Task {
 public:
  using TaskFunction = std::function<UserState()>;
  Task() = default;
  ~Task() = default;

  template <typename F>
  explicit Task(F&& func) : task_(func) {}

  const int32_t Call() const { return static_cast<int32_t>(task_()); }

 private:
  TaskFunction task_;
};

template <typename UserState>
class StateMachine {
 public:
  StateMachine()
      : state_(UserState::START), running_(false) {}

  explicit StateMachine(Arguments args)
      : args_(args), state_(UserState::START), running_(false) {}

  template <typename F>
  void On(UserState state, F&& func);

  template <typename F, typename T>
  void On(UserState state, F&& func, T&& class_obj);

  template <typename SubState>
  void RegistSubState(UserState state,
                      StateMachine<SubState>* sub_state_machine,
                      UserState output_state);

  void Run();

  bool HasTask(UserState state) const {
    return worker_.find(state) != worker_.end();
  }

  bool IsRunning() const { return running_.load(); }
  const UserState NowState() const { return state_.Now(); }

 private:
  void ChangeRunning(bool run) { running_.store(run); }

  template <typename SubState>
  class SubStateMachine {
   public:
    SubStateMachine(StateMachine<SubState>* state_machine,
                    UserState output_state)
        : state_machine_(state_machine), output_state_(output_state) {}

    UserState Run() {
      state_machine_->Run();
      return output_state_;
    }

   private:
    StateMachine<SubState>* state_machine_;
    UserState output_state_;
  };

 private:
  Arguments args_;
  State<UserState> state_;
  std::map<UserState, Task<UserState>> worker_;

  std::atomic_bool running_ = false;
};

template <typename UserState>
template <typename F>
void StateMachine<UserState>::On(UserState state, F&& func) {
  if (HasTask(state)) {
#if defined(STATE_MACHINE_DEBUG)
    std::cout << "state already exist\n";
#endif
    return;
  }

  auto task_func = std::bind(std::forward<F>(func), std::ref(args_));
  worker_.emplace(state, task_func);
}

template <typename UserState>
template <typename F, typename T>
void StateMachine<UserState>::On(UserState state, F&& func, T&& class_obj) {
  if (HasTask(state)) {
#if defined(STATE_MACHINE_DEBUG)
    std::cout << "state already exist\n";
#endif
    return;
  }

  auto task_func = std::bind(std::forward<F>(func), std::forward<T>(class_obj),
                             std::ref(args_));
  worker_.emplace(state, task_func);
}

template <typename UserState>
template <typename SubState>
void StateMachine<UserState>::RegistSubState(
    UserState state,
    StateMachine<SubState>* sub_state_machine,
    UserState output_state) {
  if (HasTask(state)) {
#if defined(STATE_MACHINE_DEBUG)
    std::cout << "state already exist\n";
#endif
    return;
  }

  auto task_func = std::bind(
      &SubStateMachine<SubState>::Run,
      std::move(SubStateMachine<SubState>(sub_state_machine, output_state)));
  worker_.emplace(state, task_func);
}

template <typename UserState>
void StateMachine<UserState>::Run() {
  ChangeRunning(true);

  while (IsRunning()) {
    if (!HasTask(state_.Now())) {
      break;
    }

    if (state_.IsDoneState()) {
      ChangeRunning(false);
    }

    {
      const Task<UserState>& task = worker_[state_.Now()];
      UserState next_state = static_cast<UserState>(task.Call());
      state_.Change(next_state);
    }
  }
}

template <typename UserState>
class NextState {
 public:
 private:
}

}  // namespace sm

#endif  // STATE_MACHINE_HPP_
