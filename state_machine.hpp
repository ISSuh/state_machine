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
#include <queue>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace sm {

class ThreadPool {
 public:
  ThreadPool() : ThreadPool(0) {}
  explicit ThreadPool(size_t thread_num)
      : thread_num_(thread_num),
        threads_(std::vector<std::thread>(thread_num_)),
        running_(false) {
    for (size_t i = 0; i < thread_num_; ++i) {
      threads_.emplace_back([&]() { RunTask(); });
    }
  }

  virtual ~ThreadPool() {
    running_ = true;
    cv_.notify_all();

    for (auto& thread : threads_) {
      if (thread.joinable()) {
        thread.join();
      }
    }
  }

  template <class F, class... Args>
  std::future<typename std::result_of<F(Args...)>::type> PushTask(
      F&& func,
      Args&&... args) {
    if (running_) {
      throw std::runtime_error("Terminate ThreadPool");
    }

    using returnType = typename std::result_of<F(Args...)>::type;

    auto task = std::make_shared<std::packaged_task<returnType()>>(
        std::bind(std::forward<F>(func), std::forward<Args>(args)...));
    std::future<returnType> taskReturn = task->get_future();

    {
      std::lock_guard<std::mutex> lock(mutex_);
      task_queue_.push([task]() { (*task)(); });
    }

    cv_.notify_one();

    return taskReturn;
  }

 private:
  void RunTask() {
    while (true) {
      std::unique_lock<std::mutex> lock(mutex_);
      cv_.wait(lock, [&]() { return !task_queue_.empty() || running_; });

      if (running_ && task_queue_.empty()) {
        return;
      }

      std::function<void()> task = std::move(task_queue_.front());
      task_queue_.pop();

      lock.unlock();

      task();
    }
  }

 private:
  size_t thread_num_;
  std::vector<std::thread> threads_;
  std::queue<std::function<void()>> task_queue_;
  std::condition_variable cv_;
  std::mutex mutex_;

  bool running_;
};

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
  State(std::initializer_list<UserState> state) : states_(*state.begin()) {}
  explicit State(UserState state) : states_(state) {}

  State(State&& rhs) : states_(rhs.states_.load()) {}
  State(const State& rhs) : states_(rhs.states_.load()) {}

  void Change(UserState state) { states_.store(state); }
  const UserState Now() const { return states_.load(); }

  bool IsStartState() const { return states_.load() == UserState::START; }
  bool IsDoneState() const { return states_.load() == UserState::DONE; }

  const State& operator=(const State& rhs) {
    if (this == &rhs) {
      return *this;
    }

    states_.store(rhs.states_.load());
    return *this;
  }

 private:
  std::atomic<UserState> states_;
};

template <typename UserState>
using States = std::vector<sm::State<UserState>>;

template <typename UserState>
class Task {
 public:
  using TaskFunction = std::function<States<UserState>()>;

  template <typename F>
  explicit Task(F&& func) : task_(func) {}

  const States<UserState> call() const { return task_(); }
  const TaskFunction& get() const { return task_; }

 private:
  Task(const Task<UserState>& rhs) = delete;
  Task<UserState>& operator=(const Task<UserState>& rhs) = delete;

  TaskFunction task_;
};

template <typename UserState>
class StateMachine {
 public:
  StateMachine() : states_({{UserState::START}}), running_(false) {}

  explicit StateMachine(Arguments args)
      : args_(args), states_({{UserState::START}}), running_(false) {}

  template <typename F>
  void On(UserState state, F&& func);

  template <typename F, typename T>
  void On(UserState state, F&& func, T&& class_obj);

  template <typename SubState>
  void RegistSubState(UserState state,
                      StateMachine<SubState>* sub_state_machine,
                      UserState output_state);

  void Excute();

  bool HasTask(UserState state) const {
    return worker_.find(state) != worker_.end();
  }

  bool IsRunning() const { return running_.load(); }

  const States<UserState>& NowStates() const { return states_; }
  const State<UserState>& NowState() const { return states_[0]; }

 private:
  void ChangeRunning(bool run) { running_.store(run); }
  bool NeedConcurreny() const { return states_.size() > 1; }

  void SequenceStateRun(States<UserState>* next_state);
  void ConcurrentStateRun(States<UserState>* next_state);

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
  Arguments args_;
  States<UserState> states_;
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
      &SubStateMachine<SubState>::Excute,
      std::move(SubStateMachine<SubState>(sub_state_machine, output_state)));
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

    states_ = next_state;
  }
}

template <typename UserState>
void StateMachine<UserState>::SequenceStateRun(States<UserState>* next_state) {
  const State<UserState>& state = NowState();
  if (!HasTask(state.Now())) {
    ChangeRunning(false);
    return;
  }

  if (state.IsDoneState()) {
    ChangeRunning(false);
  }

  const Task<UserState>& task = worker_.at(state.Now());
  *next_state = task.call();
}

template <typename UserState>
void StateMachine<UserState>::ConcurrentStateRun(
    States<UserState>* next_state) {
  const States<UserState>& states = NowStates();

  ThreadPool thread_pool(states.size());

  std::vector<std::future<States<UserState>>> concurrent_tasks;
  for (const auto& state : states) {
    const Task<UserState>& task = worker_.at(state.Now());
    concurrent_tasks.push_back(thread_pool.PushTask(task.get()));
  }

  for (std::future<States<UserState>>& task : concurrent_tasks) {
    States<UserState> result = task.get();
    for (const State<UserState>& state : result) {
      auto iter = std::find_if(next_state->begin(), next_state->end(),
                               [&](const State<UserState>& element) {
                                 return element.Now() == state.Now();
                               });

      if (iter == next_state->end()) {
        next_state->emplace_back(state.Now());
      }
    }
  }
}

}  // namespace sm

#endif  // STATE_MACHINE_HPP_
