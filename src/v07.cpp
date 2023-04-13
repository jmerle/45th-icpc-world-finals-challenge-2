#include <algorithm>
#include <cmath>
#include <ios>
#include <iostream>
#include <limits>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#ifdef LOCAL
#define log if (true) std::cerr
#else
#define log if (false) std::cerr
#endif

struct Machine {
    int id = 0;
    int power = 0;

    std::vector<std::pair<int, int>> availableIntervals{{0, std::numeric_limits<int>::max()}};
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

    int writeTime = 0;

    int endRunTime = 0;
    int endWriteTime = 0;

    [[nodiscard]] bool hasUnscheduledDependencies() const {
        return std::any_of(dependencies.begin(), dependencies.end(), [](const Task *task) {
            return task->machine == nullptr;
        });
    }

    [[nodiscard]] bool hasUnprioritizedDependents() const {
        return std::any_of(dependents.begin(), dependents.end(), [](const Task *task) {
            return task->priority == -1;
        });
    }
};

struct ScheduleOption {
    Task *task = nullptr;
    Machine *machine = nullptr;
    int startTime = 0;
    int endTime = 0;
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
        setDependenciesDependents();
        setPriorities();
        scheduleDisks();
        scheduleMachines();
    }

    void setDependenciesDependents() {
        for (auto &[_, task] : tasks) {
            task.dependencies.insert(task.dataDependencies.begin(), task.dataDependencies.end());
            task.dependencies.insert(task.taskDependencies.begin(), task.taskDependencies.end());

            task.dependents.insert(task.dataDependents.begin(), task.dataDependents.end());
            task.dependents.insert(task.taskDependents.begin(), task.taskDependents.end());
        }
    }

    void setPriorities() {
        std::queue<Task *> priorityQueue;

        for (auto &[_, task] : tasks) {
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
    }

    void scheduleDisks() {
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
            task->writeTime = std::ceil((double) task->dataSize / (double) task->disk->speed);
        }
    }

    void scheduleMachines() {
        std::vector<Task *> tasksToSchedule;

        for (auto &[_, task] : tasks) {
            if (!task.hasUnscheduledDependencies()) {
                tasksToSchedule.push_back(&task);
            }
        }

        std::sort(tasksToSchedule.begin(), tasksToSchedule.end(), [](const Task *a, const Task *b) {
            return a->priority > b->priority;
        });

        while (!tasksToSchedule.empty()) {
            Task *task = tasksToSchedule.front();
            tasksToSchedule.erase(tasksToSchedule.begin());

            ScheduleOption option = findScheduleOption(task);

            task->startTime = option.startTime;
            task->machine = option.machine;

            task->endRunTime = option.endTime - task->writeTime;
            task->endWriteTime = option.endTime;

            std::vector<std::pair<int, int>> newIntervals;
            for (int i = 0; i < option.machine->availableIntervals.size(); i++) {
                int start = option.machine->availableIntervals[i].first;
                int end = option.machine->availableIntervals[i].second;

                if (option.startTime >= start && option.endTime <= end) {
                    option.machine->availableIntervals.erase(option.machine->availableIntervals.begin() + i);

                    if (option.startTime != start) {
                        option.machine->availableIntervals.emplace_back(start, option.startTime);
                    }

                    if (option.endTime != end) {
                        option.machine->availableIntervals.emplace_back(option.endTime, end);
                    }

                    break;
                }
            }

            std::sort(option.machine->availableIntervals.begin(),
                      option.machine->availableIntervals.end(),
                      [](const std::pair<int, int> &a, const std::pair<int, int> &b) {
                          return a.first < b.first;
                      });

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

    ScheduleOption findScheduleOption(Task *task) {
        int minStartTime = 0;
        int readTime = 0;

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
            for (const auto &[start, end] : machine->availableIntervals) {
                int startTime = std::max(minStartTime, start);
                if (startTime > end) {
                    continue;
                }

                int runTime = std::ceil((double) task->taskSize / (double) machine->power);

                int endTime = startTime + readTime + runTime + task->writeTime;
                if (endTime > end) {
                    continue;
                }

                if (bestMachine == nullptr
                    || endTime < bestEndTime
                    || (endTime == bestEndTime && machine->power < bestMachine->power)) {
                    bestMachine = machine;
                    bestStartTime = startTime;
                    bestEndTime = endTime;
                }
            }
        }

        ScheduleOption option;
        option.task = task;
        option.machine = bestMachine;
        option.startTime = bestStartTime;
        option.endTime = bestEndTime;

        return option;
    }
};

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    Solver solver;
    solver.run();

    return 0;
}
