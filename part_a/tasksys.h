#ifndef _TASKSYS_H
#define _TASKSYS_H
#include <thread>
#include <vector>
#include <atomic>
#include "itasksys.h"
#include <queue>
#include <mutex>
#include <condition_variable>

/*
 * TaskSystemSerial: This class is the student's implementation of a
 * serial task execution engine.  See definition of ITaskSystem in
 * itasksys.h for documentation of the ITaskSystem interface.
 */
class TaskSystemSerial: public ITaskSystem {
    public:
        TaskSystemSerial(int num_threads);
        ~TaskSystemSerial();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
};

/*
 * TaskSystemParallelSpawn: This class is the student's implementation of a
 * parallel task execution engine that spawns threads in every run()
 * call.  See definition of ITaskSystem in itasksys.h for documentation
 * of the ITaskSystem interface.
 */
class TaskSystemParallelSpawn: public ITaskSystem {
   
    public:

        TaskSystemParallelSpawn(int num_threads);
        ~TaskSystemParallelSpawn();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();

};

/*
 * TaskSystemParallelThreadPoolSpinning: This class is the student's
 * implementation of a parallel task execution engine that uses a
 * thread pool. See definition of ITaskSystem in itasksys.h for
 * documentation of the ITaskSystem interface.
 */
class TaskSystemParallelThreadPoolSpinning: public ITaskSystem {
    private:
    // Worker thread pool
    std::vector<std::thread> worker_threads;
    // Flag to signal threads to stop_work
    std::atomic<bool> stop_work;
    // Flag to signal that a new bulk launch is active.
    std::atomic<bool> new_bulk_launch;
    // Shared state for the current bulk launch:
    std::atomic<int> task_counter;         // For dynamic task assignment within each bulk launch
    std::atomic<int> tasks_done;      // Number of tasks finished.
    std::atomic<int> active_bulk_total_tasks;  // Total tasks for the current bulk launch.
    std::atomic<IRunnable*> current_runnable; // Pointer to the current runnable.
    public:
        TaskSystemParallelThreadPoolSpinning(int num_threads);
        ~TaskSystemParallelThreadPoolSpinning();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
};

/*
 * TaskSystemParallelThreadPoolSleeping: This class is the student's
 * optimized implementation of a parallel task execution engine that uses
 * a thread pool. See definition of ITaskSystem in
 * itasksys.h for documentation of the ITaskSystem interface.
 */
class TaskSystemParallelThreadPoolSleeping: public ITaskSystem {
    private:
    int num_threads;
    bool shutdown;  // To signal threads to stop when the system is shutting down
    std::vector<std::thread> workers;
    std::queue<int> task_queue;
    std::mutex queue_mutex;
    std::condition_variable cv;
    std::atomic<int> active_bulk_total_tasks; 
    std::atomic<int> remaining_tasks;  // Track unfinished tasks
    IRunnable* current_runnable;

    void workerFunction();  // Function executed by each worker thread

    public:
        TaskSystemParallelThreadPoolSleeping(int num_threads);
        ~TaskSystemParallelThreadPoolSleeping();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
};

#endif
