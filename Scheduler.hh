#ifndef SCHEDULER_HH_
#define SCHEDULER_HH_

#include "ThreadManager.hh"
#include "Task.hpp"

class Scheduler {
public:
  Scheduler(unsigned int nbThreads,
            ThreadManager& manager);

  virtual ~Scheduler();

  template<class F, class... Args>
  auto runAt(F&& function, const std::chrono::steady_clock::time_point& timePoint, Args&&... args)
    -> std::future<typename std::result_of<F(Args...)>::type> {
    using return_type = typename std::result_of<F(Args...)>::type;
    std::future<return_type> futureResult;

    auto packagedTask = std::make_shared<std::packaged_task<return_type()> >
    (std::bind(std::forward<F>(function), std::forward<Args>(args)...));
    futureResult = packagedTask->get_future();

    this->runAt(Task([this, packagedTask]() {
        (*packagedTask)();
      }), timePoint);
    return futureResult;
  }

  void runAt(const Task& task,
             const std::chrono::steady_clock::time_point& timePoint);
  void runEvery(const Task& task,
                const std::chrono::steady_clock::duration& duration);

  template<class F, class... Args>
  void runEvery(F&& function, const std::chrono::steady_clock::duration& duration, Args&&... args) {
    auto task = std::bind(std::forward<F>(function), std::forward<Args>(args)...);
    auto cuNow = std::chrono::steady_clock::now() + duration;
    this->runAt(Task([this, task, duration]() {
        task();
        // to replace
        auto now = std::chrono::steady_clock::now();
        this->runAt(task, now + duration);
      }), cuNow);
  }

public:
  std::pair<bool, std::string> start();
  std::pair<bool, std::string> pause();
  std::pair<bool, std::string> unpause();
  std::pair<bool, std::string> stop();

private:
  void mainFunction();
  std::tuple<bool, Task, std::chrono::steady_clock::time_point>
  getHighestPriorityTask();

private:
  ThreadManager& manager;
  std::atomic<state> 	status;

  std::condition_variable cv;
  std::mutex condvarMutex;

  std::vector<std::pair<Task, std::chrono::steady_clock::time_point> > taskContainer;
  std::vector<std::pair<Task, std::chrono::steady_clock::time_point> > uniqueTask;
  std::mutex taskMutex;

  std::vector<std::shared_ptr<Worker> > workers;
  std::mutex workerMutex;
};

#endif /* end of include guard: SCHEDULER_HH_ */
