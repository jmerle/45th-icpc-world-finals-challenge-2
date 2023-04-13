import math
import unittest
from dataclasses import dataclass, field
from typing import Optional

@dataclass
class Machine:
    id: int
    power: int

@dataclass
class Disk:
    id: int
    speed: int
    capacity: int

@dataclass
class Task:
    id: int

    task_size: int
    data_size: int

    affinities: list[int]

    data_dependencies: list["Task"] = field(default_factory=lambda: [])
    data_dependents: list["Task"] = field(default_factory=lambda: [])

    task_dependencies: list["Task"] = field(default_factory=lambda: [])
    task_dependents: list["Task"] = field(default_factory=lambda: [])

    start_read_time: int = 0
    end_run_time: int = 0
    end_write_time: int = 0

    machine: Optional[Machine] = None
    disk: Optional[Disk] = None

    dependents: int = 0

class StringReader:
    def __init__(self, data: str) -> None:
        self.nums = [int(value) for value in " ".join(data.splitlines()).split(" ") if value != ""]
        self.index = 0

    def next(self) -> int:
        if self.index == len(self.nums):
            raise ValueError("Ran out of numbers to read")

        value = self.nums[self.index]
        self.index += 1

        return value

def get_score(input: str, output: str) -> float:
    tasks: dict[int, Task] = {}
    machines: dict[int, Machine] = {}
    disks: dict[int, Disk] = {}

    inp = StringReader(input)
    out = StringReader(output)

    no_tasks = inp.next()
    for _ in range(no_tasks):
        task_id = inp.next()
        task_size = inp.next()
        data_size = inp.next()

        no_affinities = inp.next()
        affinities = [inp.next() for _ in range(no_affinities)]

        tasks[task_id] = Task(task_id, task_size, data_size, affinities)

    no_machines = inp.next()
    for _ in range(no_machines):
        machine_id = inp.next()
        power = inp.next()

        machines[machine_id] = Machine(machine_id, power)

    no_disks = inp.next()
    for _ in range(no_disks):
        disk_id = inp.next()
        speed = inp.next()
        capacity = inp.next()

        disks[disk_id] = Disk(disk_id, speed, capacity)

    no_data_dependencies = inp.next()
    for _ in range(no_data_dependencies):
        task_i = tasks[inp.next()]
        task_j = tasks[inp.next()]

        task_j.data_dependencies.append(task_i)
        task_i.data_dependents.append(task_j)

    no_task_dependencies = inp.next()
    for _ in range(no_task_dependencies):
        task_i = tasks[inp.next()]
        task_j = tasks[inp.next()]

        task_j.task_dependencies.append(task_i)
        task_i.task_dependents.append(task_j)

    for _ in range(no_tasks):
        task_id = out.next()
        start_time = out.next()
        machine_id = out.next()
        disk_id = out.next()

        if task_id not in tasks:
            raise ValueError(f"Task {task_id} does not exist")

        if start_time < 0:
            raise ValueError(f"Task {task_id} is set to start at negative time {start_time}")

        if machine_id not in machines:
            raise ValueError(f"Task {task_id} is set to run on non-existent machine {machine_id}")

        if disk_id not in disks:
            raise ValueError(f"Task {task_id} is set to save data on non-existent disk {disk_id}")

        task = tasks[task_id]
        task.start_read_time = start_time
        task.machine = machines[machine_id]
        task.disk = disks[disk_id]

    for task in tasks.values():
        if task.machine is None:
            raise ValueError(f"Task {task.id} has not been scheduled")

        if task.machine.id not in task.affinities:
            raise ValueError(f"Task {task.id} is set to run on non-affinitive machine {task.machine.id}")

    for task in tasks.values():
        read_time = sum(math.ceil(t.data_size / t.disk.speed) for t in task.data_dependencies)
        run_time = math.ceil(task.task_size / task.machine.power)
        write_time = math.ceil(task.data_size / task.disk.speed)

        task.end_run_time = task.start_read_time + read_time + run_time
        task.end_write_time = task.end_run_time + write_time

        task.dependents = len(set(t.id for t in task.data_dependents).union(set(t.id for t in task.task_dependents)))

    for disk in disks.values():
        needed_capacity = sum(task.data_size for task in tasks.values() if task.disk == disk.id)
        if needed_capacity > disk.capacity:
            raise ValueError(f"Disk {disk.id} has capacity {disk.capacity}, but {needed_capacity} is needed for given data assignment")

    for task in tasks.values():
        for t in task.data_dependencies:
            if task.start_read_time < t.end_write_time:
                raise ValueError(f"Task {task.id} is set to start at {task.start_read_time}, but data dependency {t.id} finishes at {t.end_write_time}")

        for t in task.task_dependencies:
            if task.start_read_time < t.end_run_time:
                raise ValueError(f"Task {task.id} is set to start at {task.start_read_time}, but task dependency {t.id} finishes execution at {t.end_run_time}")

    for machine in machines.values():
        machine_tasks = sorted([task for task in tasks.values() if task.machine == machine], key=lambda task: task.start_read_time)
        for i in range(len(machine_tasks) - 1):
            task1 = machine_tasks[i]
            task2 = machine_tasks[i + 1]

            if task2.start_read_time < task1.end_write_time:
                raise ValueError(f"Task {task2.id} is set to start at {task2.start_read_time}, but previous task {task1.id} finishes at {task1.end_write_time}")

    lower_bound_task_size = sum(task.task_size for task in tasks.values())
    lower_bound_power = sum(machine.power for machine in machines.values())
    lower_bound_data_size = sum(task.data_size * (task.dependents + 1) for task in tasks.values())
    lower_bound_speed = sum(disk.speed for disk in disks.values())
    lower_bound = 100 * (lower_bound_task_size / lower_bound_power + lower_bound_data_size / lower_bound_speed)

    makespan = max(task.end_write_time for task in tasks.values())

    return lower_bound / makespan

class ScoreTest(unittest.TestCase):
    def test_get_score_example(self) -> None:
        example_input = """
6
1 40 6 2 1 2
2 20 6 2 1 2
3 96 10 2 1 2
4 20 6 2 1 2
5 60 0 2 1 2
6 20 0 1 1
2
1 1
2 2
2
1 1 30
2 2 17
8
1 2
1 3
1 4
2 4
2 5
3 5
3 6
4 6
1
1 5
        """.strip() + "\n"

        example_output = """
1  0  2  2
2 23  1  1
3 23  2  2
4 52  1  1
5 79  2  1
6 87  1  2
        """.strip() + "\n"

        example_lower_bound = 100 * (256 / 3 + 90 / 3)
        example_makespan = 120
        example_score = example_lower_bound / example_makespan

        score = get_score(example_input, example_output)

        self.assertAlmostEqual(score, example_score)

if __name__ == "__main__":
    unittest.main()
