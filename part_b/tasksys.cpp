#include "tasksys.h"


IRunnable::~IRunnable() {}

ITaskSystem::ITaskSystem(int num_threads) {}
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
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemSerial::sync() {
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
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
}

TaskSystemParallelSpawn::~TaskSystemParallelSpawn() {}

void TaskSystemParallelSpawn::run(IRunnable* runnable, int num_total_tasks) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemParallelSpawn::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                 const std::vector<TaskID>& deps) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemParallelSpawn::sync() {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
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

TaskSystemParallelThreadPoolSpinning::TaskSystemParallelThreadPoolSpinning(int num_threads): ITaskSystem(num_threads) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelThreadPoolSpinning in Part B.
}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {}

void TaskSystemParallelThreadPoolSpinning::run(IRunnable* runnable, int num_total_tasks) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelThreadPoolSpinning in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemParallelThreadPoolSpinning::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                              const std::vector<TaskID>& deps) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelThreadPoolSpinning in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemParallelThreadPoolSpinning::sync() {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelThreadPoolSpinning in Part B.
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
// Helper to get a BulkLaunch record by its ID.
BulkLaunch* TaskSystemParallelThreadPoolSleeping::retrieve_bulk_launch(BulkId id) {
    //std::lock_guard<std::mutex> lock(bulkMutex);
    for (BulkLaunch* b_launch : active_bulk_launch_records) {
        if (b_launch->id == id)
            return b_launch;
    }
    return nullptr;
}
// Helper to schedule bulk launches that are waiting for dependencies.
void TaskSystemParallelThreadPoolSleeping::check_waiting_bulkLaunches() {
    {
    std::lock_guard<std::mutex> lock(bulk_mutex);
    for (BulkLaunch* b_launch : active_bulk_launch_records) {
        if (!b_launch->scheduled) {
            bool ready = check_dependencies(b_launch->dependencies);
            if (ready) {
                // Enqueue all tasks for this bulk launch.
                {
                    std::lock_guard<std::mutex> qlock(queue_mutex);
                    for (int i = 0; i < b_launch->num_total_tasks; i++) {
                        taskQueue.push({b_launch, i});
                    }
                }
                cv.notify_all();
                b_launch->scheduled = true;
            }
        }
    }
   }
}


// Worker thread function.
void TaskSystemParallelThreadPoolSleeping::workerFunction() {
    while (true) {
        Task_queue_item task_item;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            cv.wait(lock, [this] { return !taskQueue.empty() || shutdown; });
            if (shutdown && taskQueue.empty())
                return;
            task_item = taskQueue.front();
            taskQueue.pop();
        }
        // Execute the task using the BulkLaunch's runnable.
        if (task_item.bulk && task_item.bulk->runnable) {
            task_item.bulk->runnable->runTask(task_item.task_index, task_item.bulk->num_total_tasks);
        }
       
        int task_left = task_item.bulk->remaining.fetch_sub(1) - 1;
        if (task_left == 0) {
            // Bulk launch is complete: notify sync() and check waiting launches.
            cv_bulk.notify_all();
            check_waiting_bulkLaunches();
        }
    }
}

bool TaskSystemParallelThreadPoolSleeping::check_dependencies(const std::vector<BulkId>& deps) {
    
    for (BulkId dep : deps) {
        // Use the retrieve_bulk_launch function that assumes the lock is held
        BulkLaunch* dependent_bl = retrieve_bulk_launch(dep);
        if (dependent_bl && dependent_bl->remaining.load() > 0) {
            return false;
        }
    }
    return true;
}
TaskSystemParallelThreadPoolSleeping::TaskSystemParallelThreadPoolSleeping(int num_threads)
    : ITaskSystem(num_threads),
      shutdown(false),
      new_bulk_id(0)
 {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    
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
    cv.notify_all();
    for (auto &worker : workers) {
        if (worker.joinable())
            worker.join();
    }
    {
        
        std::lock_guard<std::mutex> lock(bulk_mutex);
        for (BulkLaunch* b_launch : active_bulk_launch_records) {
            delete b_launch;
        }
        active_bulk_launch_records.clear();
    }
}

void TaskSystemParallelThreadPoolSleeping::run(IRunnable* runnable, int num_total_tasks) {



    runAsyncWithDeps(runnable, num_total_tasks, std::vector<BulkId>());
    sync();
}

BulkId TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                    const std::vector<BulkId>& deps) {


    //
    // TODO: CS149 students will implement this method in Part B.
    //

    // initialze a new bulk launch 
    BulkId new_id = new_bulk_id.fetch_add(1);
    BulkLaunch* b_launch = new BulkLaunch();
    b_launch->id = new_id;
    b_launch->runnable = runnable;
    b_launch->num_total_tasks = num_total_tasks;
    b_launch->remaining.store(num_total_tasks);
    b_launch->dependencies = deps;
    b_launch->scheduled = false;
    {
        std::lock_guard<std::mutex> lock(bulk_mutex);
        active_bulk_launch_records.push_back(b_launch);
    
    // Check if the dependencies are met we need to add this to the task queue immdiately.
    
        //std::lock_guard<std::mutex> lock(bulk_mutex);
        bool ready = check_dependencies(b_launch->dependencies);
        if (ready) {
            {
                std::lock_guard<std::mutex> lock(queue_mutex);
                for (int i = 0; i < num_total_tasks; i++) {
                    taskQueue.push({b_launch, i});
                }
            }
            cv.notify_all();
            b_launch->scheduled = true;
        }
}
    // Return the id
    return new_id;
}

void TaskSystemParallelThreadPoolSleeping::sync() {

    //
    // TODO: CS149 students will modify the implementation of this method in Part B.
    // 
    std::unique_lock<std::mutex> lock(bulk_mutex);
    //the caller to sync keeps waiting until the lambda function returns true ( that is when all bulk launches have completed.)
    cv_bulk.wait(lock, [this] {
        for (BulkLaunch* bl : active_bulk_launch_records) {
            if (bl->remaining.load() > 0)
                return false;
        }
        return true;
    });
    // (Optional) Cleanup finished BulkLaunch records.
        
    auto it = active_bulk_launch_records.begin();
    while (it != active_bulk_launch_records.end()) {
        BulkLaunch* bl = *it;
        if (bl->remaining.load() == 0) {
            // Remove and delete the finished bulk launch.
            it = active_bulk_launch_records.erase(it);
            delete bl;
        } else {
            ++it;
        }
    }

    return;
}
