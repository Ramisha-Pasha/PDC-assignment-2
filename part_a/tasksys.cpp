#include "tasksys.h"
#include <thread>
#include <vector>
#include <atomic>
#include <algorithm> 
#include <queue>
#include <mutex>
#include <condition_variable>

IRunnable::~IRunnable() {}

//ITaskSystem::ITaskSystem(int num_threads) {}
ITaskSystem::~ITaskSystem() {}

/*
 * ================================================================
 * Serial task system implementation
 * ================================================================
 */

const char* TaskSystemSerial::name() {
    return "Serial";
}

TaskSystemSerial::TaskSystemSerial(int num_threads): ITaskSystem(num_threads) {
}

TaskSystemSerial::~TaskSystemSerial() {}

void TaskSystemSerial::run(IRunnable* runnable, int num_total_tasks) {
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemSerial::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                          const std::vector<TaskID>& deps) {
    // You do not need to implement this method.
    return 0;
}

void TaskSystemSerial::sync() {
    // You do not need to implement this method.
    return;
}

/*
 * ================================================================
 * Parallel Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelSpawn::name() {
    return "Parallel + Always Spawn";
}

TaskSystemParallelSpawn::TaskSystemParallelSpawn(int num_threads): ITaskSystem(num_threads) {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
}

TaskSystemParallelSpawn::~TaskSystemParallelSpawn() {}

void TaskSystemParallelSpawn::run(IRunnable* runnable, int num_total_tasks) {


    //
    // TODO: CS149 students will modify the implementation of this
    // method in Part A.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //
    //-----------------using dynamic task allocation to the worker threads
    std::atomic<int> task_counter(0);
    //determining the no of threads to spawn which is min of the max threads that the task system can spawn and the total no of tasks to be executed
    int num_threads_to_spawn= std::min(this->num_threads,num_total_tasks);
    //creating vector of threads
    std::vector<std::thread> workerThreads;
    //allocates memory for the vector equal to num_threads_to_spawn
    workerThreads.reserve(num_threads_to_spawn);
    for(int i = 0; i < num_threads_to_spawn; i++){
        // provide a callable in the form of lamda function that does exactly what you want the thread to do.
        // In our dynamic task assignment example, the lambda function needs to:
        //Access the runnable object (so it can call runTask).
        //Access the task_counter (to fetch the next task ID atomically).
       //Know the total number of tasks (num_total_tasks) (to stop when all tasks are completed).
        workerThreads.push_back(std::thread([runnable, &task_counter, num_total_tasks]() {
            // Each thread repeatedly gets a task index and increments the counter atomically until all tasks are done.
            while (true) {
                int task_id = task_counter.fetch_add(1);
                if (task_id >= num_total_tasks) break;
                runnable->runTask(task_id, num_total_tasks);
            }
        }));
    }
    
    // join all threads 
    for (auto& thread : workerThreads) {
        thread.join();
    }

    
}

TaskID TaskSystemParallelSpawn::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                 const std::vector<TaskID>& deps) {
    // You do not need to implement this method.
    return 0;
}

void TaskSystemParallelSpawn::sync() {
    // You do not need to implement this method.
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Spinning Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelThreadPoolSpinning::name() {
    return "Parallel + Thread Pool + Spin";
}

TaskSystemParallelThreadPoolSpinning::TaskSystemParallelThreadPoolSpinning(int num_threads): ITaskSystem(num_threads), stop_work(false), new_bulk_launch(false),
task_counter(0), tasks_done(0), active_bulk_total_tasks(0), current_runnable(nullptr) {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    //we need to create a thread pool upfront here and pass all the shared paramters to help them spin.
    // Create the pool of worker threads.
    for (int i = 0; i < this->num_threads; i++) {
        worker_threads.push_back(std::thread([this]() {
            // Each worker thread runs this loop until stop work is signaled.
            while (!stop_work.load()) {
                // Spin until a new bulk launch is signaled.
                if (!new_bulk_launch.load()) {
                    std::this_thread::yield();  // Yield to allow other threads to run.
                    continue;
                }
                
                int id = task_counter.fetch_add(1);
                if (id < active_bulk_total_tasks.load()) {
                    //get  the current runnable and execute the task.
                    IRunnable* run_able = current_runnable.load();
                    if (run_able != nullptr) {
                        run_able->runTask(id, active_bulk_total_tasks.load());
                    }
                    //increment the tasks completed counter
                    tasks_done.fetch_add(1);
                } else {
                    // No tasks available
                    std::this_thread::yield();
                }
            }
        }));
    }
    
}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {
    stop_work.store(true);
    new_bulk_launch.store(true);
    for (std::thread &t : worker_threads) {
        if (t.joinable())
            t.join();
    }
}

void TaskSystemParallelThreadPoolSpinning::run(IRunnable* runnable, int num_total_tasks) {


    //
    // TODO: CS149 students will modify the implementation of this
    // method in Part A.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //

   // Set up the new bulk launch.
   current_runnable.store(runnable);
   active_bulk_total_tasks.store(num_total_tasks);
   task_counter.store(0);
   tasks_done.store(0);
   new_bulk_launch.store(true);

   // Wait until all tasks have been completed in the curren bulk launch
   while (tasks_done.load() < num_total_tasks) {
       std::this_thread::yield();  
   }
   
   // Bulk launch is complete; signal workers to stop processing until next run.
   new_bulk_launch.store(false);
}

TaskID TaskSystemParallelThreadPoolSpinning::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                              const std::vector<TaskID>& deps) {
    // You do not need to implement this method.
    return 0;
}

void TaskSystemParallelThreadPoolSpinning::sync() {
    // You do not need to implement this method.
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Sleeping Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelThreadPoolSleeping::name() {
    return "Parallel + Thread Pool + Sleep";
}
void TaskSystemParallelThreadPoolSleeping::workerFunction() {
    while (true) {
        int task_id;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            cv.wait(lock, [this] { return !task_queue.empty() || shutdown;});
            // if (!cv.wait_for(lock, std::chrono::milliseconds(10),
            // [this] { return !task_queue.empty() || shutdown; })) {
            //     // Timed out; loop around to check again.
            //     continue;
            //     }


            //if queue is empty, safely exit else take a task and execute it
            if (shutdown && task_queue.empty()) {
                return;  // exit
            }


            task_id = task_queue.front();
            task_queue.pop();
        }


        // Execute task outside the lock
        current_runnable->runTask(task_id, active_bulk_total_tasks.load());


        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            remaining_tasks--;


            if (remaining_tasks == 0) {
                cv.notify_all();  // Notify `run()` that all tasks are done
            }
        }
    }
}




TaskSystemParallelThreadPoolSleeping::TaskSystemParallelThreadPoolSleeping(int num_threads)
    : ITaskSystem(num_threads), num_threads(num_threads), shutdown(false), active_bulk_total_tasks(0), remaining_tasks(0), current_runnable(nullptr) {


    // simply creating worker threads in constructor
    // threads are persistently created, and each thread runs the worker function.
    for (int i = 0; i < num_threads; i++) {
        workers.emplace_back(&TaskSystemParallelThreadPoolSleeping::workerFunction, this);
    }
}



TaskSystemParallelThreadPoolSleeping::~TaskSystemParallelThreadPoolSleeping() {
    //
    // TODO: CS149 student implementations may decide to perform cleanup
    // operations (such as thread pool shutdown construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    

    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        shutdown = true;
    }
    cv.notify_all();  // Wake up all threads so they can exit


    for (auto& worker : workers) {
        if (worker.joinable())
            worker.join();
    }

}

void TaskSystemParallelThreadPoolSleeping::run(IRunnable* runnable, int num_total_tasks) {


    //
    // TODO: CS149 students will modify the implementation of this
    // method in Parts A and B.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //

    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        current_runnable = runnable;
        remaining_tasks = num_total_tasks;
        active_bulk_total_tasks=num_total_tasks;


        for (int i = 0; i < num_total_tasks; i++) {
            task_queue.push(i);
        }
    }


    cv.notify_all();    // "wake up" worker threads


    // wait for all tasks to finish before returning
    std::unique_lock<std::mutex> lock(queue_mutex);
    //cv.wait(lock, [this] { return remaining_tasks == 0; });

    while (remaining_tasks > 0) {
        // Wait with a timeout to periodically re-check (and allow potential external interrupts)
        cv.wait_for(lock, std::chrono::milliseconds(30));
    }

}

TaskID TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                    const std::vector<TaskID>& deps) {


    //
    // TODO: CS149 students will implement this method in Part B.
    //

    return 0;
}

void TaskSystemParallelThreadPoolSleeping::sync() {

    //
    // TODO: CS149 students will modify the implementation of this method in Part B.
    //

    return;
}
