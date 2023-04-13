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
        std::queue<Task *> tasksToSchedule;
        for (auto &[_, task] : tasks) {
            task.dependencies.insert(task.dataDependencies.begin(), task.dataDependencies.end());
            task.dependencies.insert(task.taskDependencies.begin(), task.taskDependencies.end());

            task.dependents.insert(task.dataDependents.begin(), task.dataDependents.end());
            task.dependents.insert(task.taskDependents.begin(), task.taskDependents.end());

            if (!task.hasUnscheduledDependencies()) {
                tasksToSchedule.push(&task);
            }
        }

        std::vector<Disk *> sortedDisks;
        for (auto &[_, disk] : disks) {
            sortedDisks.push_back(&disk);
        }

        std::sort(sortedDisks.begin(), sortedDisks.end(), [](const Disk *a, const Disk *b) {
            return a->speed > b->speed;
        });

        while (!tasksToSchedule.empty()) {
            Task *task = tasksToSchedule.front();

            Disk *bestDisk = *std::find_if(sortedDisks.begin(), sortedDisks.end(), [&](const Disk *d) {
                return d->usedCapacity + task->dataSize <= d->capacity;
            });

            int minStartTime = 0;
            int readTime = 0;
            int writeTime = std::ceil((double) task->dataSize / (double) bestDisk->speed);

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
            task->disk = bestDisk;

            task->endRunTime = bestEndTime - writeTime;
            task->endWriteTime = bestEndTime;

            bestMachine->nextStartTime = bestEndTime;
            bestDisk->usedCapacity += task->dataSize;

            for (auto *t : task->dependents) {
                if (!t->hasUnscheduledDependencies()) {
                    tasksToSchedule.push(t);
                }
            }

            tasksToSchedule.pop();
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
