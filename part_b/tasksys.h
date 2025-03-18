#ifndef _TASKSYS_H
#define _TASKSYS_H
#include "itasksys.h"
#include <thread>
#include <vector>
#include <atomic>

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

//-----------------------------------------------------------------------------------------------------------------------------------
 using BulkId = int;
//we need this structure because each bulk launch is now an entity on which other bulk might depend.
struct BulkLaunch {
    BulkId id;                         //ID for this bulk launch.
    IRunnable* runnable;               
    int num_total_tasks;               //total no of tasks
    std::atomic<int> remaining;        //tasks left in this bulk
    std::vector<BulkId> dependencies;  // list of ids that this bulk launch depends on.
    bool scheduled;                    // if this bulk's tasks have been enqued(dependencies are met)
};

//in part 3, we enqued the task by their ids in the task queue but now we need the bulkId associated too.
//now each item in the task queue contains the pointer to the  bulkLaunch (so that its variabless can be manipulatedd )
struct Task_queue_item {
    BulkLaunch* bulk;
    int task_index;
};

class TaskSystemParallelThreadPoolSleeping: public ITaskSystem {
    private:
   
    bool shutdown;  // To signal threads to stop when the system is shutting down
    std::vector<std::thread> workers;
    std::queue<Task_queue_item> taskQueue;
    std::mutex queue_mutex;
    std::condition_variable cv;

    std::vector<BulkLaunch*> active_bulk_launch_records;
    std::mutex bulk_mutex; 
    std::condition_variable cv_bulk;  // used by sync method to wait for all bulk laucnhes to finish
    std::atomic<BulkId> new_bulk_id;    // generates unique bulk launch id.

    void workerFunction();  // Function executed by each worker thread
    void check_waiting_bulkLaunches(); //once a bulk laucnh is finished, we have to check if the finished bulk launch was the dependency of other bulk laucnhes
    //                                   so that we can enqueue those bulk launches.
    BulkLaunch* retrieve_bulk_launch(BulkId id);
    bool check_dependencies(const std::vector<BulkId>& deps);
    public:
        TaskSystemParallelThreadPoolSleeping(int num_threads);
        ~TaskSystemParallelThreadPoolSleeping();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        BulkId runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<BulkId>& deps);
        void sync();
};

#endif
