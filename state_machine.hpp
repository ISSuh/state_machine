/**
 *
 *  Copyright:  Copyright (c) 2021, ISSuh
 *
 */

#ifndef STATE_MACHINE_HPP_
#define STATE_MACHINE_HPP_

#include <algorithm>
#include <atomic>
#include <functional>
#include <future>
#include <initializer_list>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

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

  ArgumentType* get() const { return value_; }

 private:
  ArgumentType* value_;
};

// TODO(issuh) : fix dangling pointer on args_
class Arguments {
 public:
  Arguments() = default;
  ~Arguments() {
    for (const auto& item : args_) {
      delete item.second;
    }

    args_.clear();
  }

  template <typename ArgumentType>
  void insert(const std::string& key, ArgumentType* value);

  template <typename ArgumentType>
  ArgumentType* at(const std::string& key);

  bool find(const std::string& key) const;
  void erase(const std::string& key);


 private:
  std::mutex mutex_;
  std::map<std::string, ArgumentBase*> args_;
};

template <typename ArgumentType>
void Arguments::insert(const std::string& key, ArgumentType* value) {
  std::lock_guard<std::mutex> lock_guard(mutex_);
  if (!find(key)) {
    args_.insert(
        {key, static_cast<ArgumentBase*>(new Argument<ArgumentType>(value))});
  }
}

template <typename ArgumentType>
ArgumentType* Arguments::at(const std::string& key) {
  std::lock_guard<std::mutex> lock_guard(mutex_);
  if (!find(key)) {
    return nullptr;
  }

  ArgumentBase* arg_base = args_.at(key);
  Argument<ArgumentType>* arg = dynamic_cast<Argument<ArgumentType>*>(arg_base);
  return arg->get();
}
bool Arguments::find(const std::string& key) const {
  return args_.find(key) != args_.end();
}

void Arguments::erase(const std::string& key) {
  std::lock_guard<std::mutex> lock_guard(mutex_);
  if (find(key)) {
    ArgumentBase* arg = args_.at(key);
    delete arg;

    args_.erase(key);
  }
}

template <typename UserState>
class State {
 public:
  explicit State(UserState state) : states_(state) {}
  State(std::initializer_list<UserState> state) : states_(*state.begin()) {}
  State(State&& rhs) noexcept : states_(rhs.states_) {}
  State(const State& rhs) : states_(rhs.states_) {}

  const UserState Now() const { return states_; }
  bool IsStartState() const { return states_ == UserState::START; }
  bool IsDoneState() const { return states_ == UserState::DONE; }

  const State& operator=(const State& rhs) {
    if (this == &rhs) {
      return *this;
    }

    states_ = rhs.states_;
    return *this;
  }

 private:
  UserState states_;
};

template <typename UserState>
using States = std::vector<sm::State<UserState>>;

template <typename UserState>
class Task {
 public:
  template <typename F>
  explicit Task(F&& func) : task_(func) {}

  States<UserState> call() const { return task_(); }

 private:
  using TaskFunction = std::function<States<UserState>()>;

  Task(const Task<UserState>& rhs) = delete;
  Task<UserState>& operator=(const Task<UserState>& rhs) = delete;

  TaskFunction task_;
};

template <typename UserState>
class StateMachine {
 public:
  StateMachine()
      : args_(nullptr),
        states_({{UserState::START}}),
        running_(false),
        count_concurrency_states_(0) {}

  explicit StateMachine(Arguments* args)
      : args_(args),
        states_({{UserState::START}}),
        running_(false),
        count_concurrency_states_(0) {}

  Arguments* GetArguments() const { return args_; }

  template <typename F>
  void On(UserState state, F&& func);

  template <typename F, typename T>
  void On(UserState state, F&& func, T&& class_obj);

  template <typename SubState>
  void RegistSubState(UserState state,
                      StateMachine<SubState>* sub_state_machine,
                      UserState output_state);

  void Excute();

  bool IsRunning() const { return running_.load(); }

  const States<UserState>& NowStates() const { return states_; }
  State<UserState> NowState() const { return *states_.begin(); }

 private:
  bool HasTask(UserState state) const {
    return worker_.find(state) != worker_.end();
  }

  void ChangeRunning(bool run) { running_.store(run); }
  void SequenceStateRun(States<UserState>* next_state);
  void ConcurrentStateRun(States<UserState>* next_state);

  bool NeedConcurreny() const { return states_.size() > 1; }
  bool NeedWaitingConcurrentState() const {
    return count_concurrency_states_.load() > 1;
  }

  template <typename SubState>
  class SubStateMachine {
   public:
    SubStateMachine(StateMachine<SubState>* state_machine,
                    UserState output_state)
        : state_machine_(state_machine), output_state_(output_state) {}

    States<UserState> Excute() {
      state_machine_->Excute();
      return {{output_state_}};
    }

   private:
    StateMachine<SubState>* state_machine_;
    UserState output_state_;
  };

 private:
  Arguments* args_;
  States<UserState> states_;
  std::map<UserState, Task<UserState>> worker_;

  std::atomic_bool running_ = false;

  std::atomic_uint64_t count_concurrency_states_;
  std::mutex mutex_;
};

template <typename UserState>
template <typename F>
void StateMachine<UserState>::On(UserState state, F&& func) {
  std::lock_guard<std::mutex> lock_guard(mutex_);

  auto task_func = std::bind(std::forward<F>(func), std::ref(*args_));

  if (HasTask(state)) {
    worker_.erase(state);
  }

  worker_.emplace(state, task_func);
}

template <typename UserState>
template <typename F, typename T>
void StateMachine<UserState>::On(UserState state, F&& func, T&& class_obj) {
  std::lock_guard<std::mutex> lock_guard(mutex_);

  auto task_func = std::bind(std::forward<F>(func), std::forward<T>(class_obj),
                             std::ref(*args_));

  if (HasTask(state)) {
    worker_.erase(state);
  }

  worker_.emplace(state, task_func);
}

template <typename UserState>
template <typename SubState>
void StateMachine<UserState>::RegistSubState(
    UserState state,
    StateMachine<SubState>* sub_state_machine,
    UserState output_state) {
  std::lock_guard<std::mutex> lock_guard(mutex_);

  auto task_func = std::bind(
      &SubStateMachine<SubState>::Excute,
      std::move(SubStateMachine<SubState>(sub_state_machine, output_state)));

  if (HasTask(state)) {
    worker_.erase(state);
  }

  worker_.emplace(state, task_func);
}

template <typename UserState>
void StateMachine<UserState>::Excute() {
  ChangeRunning(true);

  while (IsRunning()) {
    States<UserState> next_state;
    if (NeedConcurreny()) {
      ConcurrentStateRun(&next_state);
    } else {
      SequenceStateRun(&next_state);
    }

    if (!next_state.empty()) {
      states_.swap(next_state);
    } else {
      ChangeRunning(false);
    }
  }
}

template <typename UserState>
void StateMachine<UserState>::SequenceStateRun(States<UserState>* next_state) {
  const State<UserState>& state = NowState();
  if (!HasTask(state.Now()) || state.IsDoneState()) {
    ChangeRunning(false);
    return;
  }

  if (NeedWaitingConcurrentState()) {
    return;
  }

  const Task<UserState>& task = worker_.at(state.Now());
  *next_state = task.call();
}

template <typename UserState>
void StateMachine<UserState>::ConcurrentStateRun(
    States<UserState>* next_state) {
  const States<UserState>& states = NowStates();

  std::vector<std::future<States<UserState>>> concurrent_tasks;
  for (const State<UserState>& state : states) {
    if (!HasTask(state.Now()) || state.IsDoneState()) {
      continue;
    }

    const Task<UserState>& task = worker_.at(state.Now());
    concurrent_tasks.emplace_back(std::async(std::launch::async, &Task<UserState>::call, &task));
    count_concurrency_states_.fetch_add(1);
  }

  for (std::future<States<UserState>>& task : concurrent_tasks) {
    States<UserState> result = task.get();
    for (const State<UserState>& state : result) {
      next_state->emplace_back(state.Now());
    }

    count_concurrency_states_.fetch_sub(1);
  }
}

}  // namespace sm

#endif  // STATE_MACHINE_HPP_
