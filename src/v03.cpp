#include <algorithm>
#include <cmath>
#include <ios>
#include <iostream>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#ifdef LOCAL
#define log if (true) std::cerr
#else
#define log if (false) std::cerr
#endif

struct Machine {
    int id = 0;
    int power = 0;

    int nextStartTime = 0;
};

struct Disk {
    int id = 0;
    int speed = 0;
    int capacity = 0;

    int usedCapacity = 0;
};

struct Task {
    int id = 0;

    int taskSize = 0;
    int dataSize = 0;

    std::vector<Machine *> affinities;

    std::vector<Task *> dataDependencies;
    std::vector<Task *> dataDependents;

    std::vector<Task *> taskDependencies;
    std::vector<Task *> taskDependents;

    int startTime = 0;
    Machine *machine = nullptr;
    Disk *disk = nullptr;

    std::unordered_set<Task *> dependencies;
    std::unordered_set<Task *> dependents;

    double priority = -1;

    int endRunTime = 0;
    int endWriteTime = 0;

    [[nodiscard]] bool isScheduled() const {
        return machine != nullptr;
    }

    [[nodiscard]] bool hasUnscheduledDependencies() const {
        return std::any_of(dependencies.begin(), dependencies.end(), [](const Task *task) {
            return !task->isScheduled();
        });
    }

    [[nodiscard]] bool hasUnprioritizedDependents() const {
        return std::any_of(dependents.begin(), dependents.end(), [](const Task *task) {
            return task->priority == -1;
        });
    }
};

struct Solver {
    std::unordered_map<int, Task> tasks;
    std::unordered_map<int, Machine> machines;
    std::unordered_map<int, Disk> disks;

    void run() {
        int noTasks;
        std::cin >> noTasks;

        for (int i = 0; i < noTasks; i++) {
            int taskId, taskSize, dataSize, noAffinities;
            std::cin >> taskId >> taskSize >> dataSize >> noAffinities;

            Task &task = tasks[taskId];
            task.id = taskId;
            task.taskSize = taskSize;
            task.dataSize = dataSize;

            for (int j = 0; j < noAffinities; j++) {
                int machineId;
                std::cin >> machineId;

                task.affinities.push_back(&machines[machineId]);
            }
        }

        int noMachines;
        std::cin >> noMachines;

        for (int i = 0; i < noMachines; i++) {
            int machineId, power;
            std::cin >> machineId >> power;

            Machine &machine = machines[machineId];
            machine.id = machineId;
            machine.power = power;
        }

        int noDisks;
        std::cin >> noDisks;

        for (int i = 0; i < noDisks; i++) {
            int diskId, speed, capacity;
            std::cin >> diskId >> speed >> capacity;

            Disk &disk = disks[diskId];
            disk.id = diskId;
            disk.speed = speed;
            disk.capacity = capacity;
        }

        int noDataDependencies;
        std::cin >> noDataDependencies;

        for (int i = 0; i < noDataDependencies; i++) {
            int from, to;
            std::cin >> from >> to;

            Task &taskFrom = tasks[from];
            Task &taskTo = tasks[to];

            taskTo.dataDependencies.push_back(&taskFrom);
            taskFrom.dataDependents.push_back(&taskTo);
        }

        int noTaskDependencies;
        std::cin >> noTaskDependencies;

        for (int i = 0; i < noTaskDependencies; i++) {
            int from, to;
            std::cin >> from >> to;

            Task &taskFrom = tasks[from];
            Task &taskTo = tasks[to];

            taskTo.taskDependencies.push_back(&taskFrom);
            taskFrom.taskDependents.push_back(&taskTo);
        }

        scheduleTasks();

        for (const auto &[id, task] : tasks) {
            std::cout << task.id
                      << " " << task.startTime
                      << " " << task.machine->id
                      << " " << task.disk->id
                      << std::endl;
        }
    }

    void scheduleTasks() {
        std::vector<Task *> tasksToSchedule;
        std::queue<Task *> priorityQueue;

        for (auto &[_, task] : tasks) {
            task.dependencies.insert(task.dataDependencies.begin(), task.dataDependencies.end());
            task.dependencies.insert(task.taskDependencies.begin(), task.taskDependencies.end());

            task.dependents.insert(task.dataDependents.begin(), task.dataDependents.end());
            task.dependents.insert(task.taskDependents.begin(), task.taskDependents.end());

            if (!task.hasUnscheduledDependencies()) {
                tasksToSchedule.push_back(&task);
            }

            if (!task.hasUnprioritizedDependents()) {
                priorityQueue.push(&task);
            }
        }

        while (!priorityQueue.empty()) {
            Task *task = priorityQueue.front();

            double maxDependentPriority = 0;
            for (auto *t : task->dependents) {
                maxDependentPriority = std::max(maxDependentPriority, (double) t->dataSize + t->priority);
            }

            task->priority = (double) task->taskSize + maxDependentPriority;

            for (auto *t : task->dependencies) {
                if (!t->hasUnprioritizedDependents()) {
                    priorityQueue.push(t);
                }
            }

            priorityQueue.pop();
        }

        std::sort(tasksToSchedule.begin(), tasksToSchedule.end(), [](const Task *a, const Task *b) {
            return a->priority > b->priority;
        });

        std::vector<Disk *> sortedDisks;
        for (auto &[_, disk] : disks) {
            sortedDisks.push_back(&disk);
        }

        std::sort(sortedDisks.begin(), sortedDisks.end(), [](const Disk *a, const Disk *b) {
            return a->speed > b->speed;
        });

        std::vector<Task *> sortedTasks;
        for (auto &[_, task] : tasks) {
            sortedTasks.push_back(&task);
        }

        std::sort(sortedTasks.begin(), sortedTasks.end(), [](const Task *a, const Task *b) {
            auto diskActivityA = a->dataSize * (a->dataDependents.size() + 1);
            auto diskActivityB = b->dataSize * (b->dataDependents.size() + 1);

            if (diskActivityA == diskActivityB) {
                return a->priority > b->priority;
            }

            return diskActivityA > diskActivityB;
        });

        for (auto *task : sortedTasks) {
            task->disk = *std::find_if(sortedDisks.begin(), sortedDisks.end(), [&](const Disk *d) {
                return d->usedCapacity + task->dataSize <= d->capacity;
            });

            task->disk->usedCapacity += task->dataSize;
        }

        while (!tasksToSchedule.empty()) {
            Task *task = tasksToSchedule.front();
            tasksToSchedule.erase(tasksToSchedule.begin());

            int minStartTime = 0;
            int readTime = 0;
            int writeTime = std::ceil((double) task->dataSize / (double) task->disk->speed);

            for (const auto *t : task->dataDependencies) {
                minStartTime = std::max(minStartTime, t->endWriteTime);
                readTime += std::ceil((double) t->dataSize / (double) t->disk->speed);
            }

            for (const auto *t : task->taskDependencies) {
                minStartTime = std::max(minStartTime, t->endRunTime);
            }

            Machine *bestMachine = nullptr;
            int bestStartTime = -1;
            int bestEndTime = -1;

            for (Machine *machine : task->affinities) {
                int startTime = std::max(minStartTime, machine->nextStartTime);
                int runTime = std::ceil((double) task->taskSize / (double) machine->power);
                int endTime = startTime + readTime + runTime + writeTime;

                if (bestMachine == nullptr || endTime < bestEndTime) {
                    bestMachine = machine;
                    bestStartTime = startTime;
                    bestEndTime = endTime;
                }
            }

            task->startTime = bestStartTime;
            task->machine = bestMachine;

            task->endRunTime = bestEndTime - writeTime;
            task->endWriteTime = bestEndTime;

            bestMachine->nextStartTime = bestEndTime;

            bool addedTasks = false;
            for (auto *t : task->dependents) {
                if (!t->hasUnscheduledDependencies()) {
                    tasksToSchedule.push_back(t);
                    addedTasks = true;
                }
            }

            if (addedTasks) {
                std::sort(tasksToSchedule.begin(), tasksToSchedule.end(), [](const Task *a, const Task *b) {
                    return a->priority > b->priority;
                });
            }
        }
    }
};

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    Solver solver;
    solver.run();

    return 0;
}
