import math
import numpy as np
from dataclasses import dataclass
from pathlib import Path
from xml.etree import ElementTree

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

    data_dependencies: list[int]
    task_dependencies: list[int]

class Generator:
    def generate(self) -> str:
        adjacency_matrix = self.create_adjacency_matrix()

        tasks: list[Task] = []
        machines: list[Machine] = []
        disks: list[Disk] = []

        task_dependency_chance = np.random.random_sample()

        for i in range(adjacency_matrix.shape[0]):
            task_id = i + 1

            task_size = np.random.randint(10, 601)
            data_size = np.random.randint(0, 21)

            affinities = []

            data_dependencies = []
            task_dependencies = []

            for j in range(i):
                if adjacency_matrix[i, j] == 1:
                    if np.random.random_sample() < task_dependency_chance:
                        task_dependencies.append(j + 1)
                    else:
                        data_dependencies.append(j + 1)

            tasks.append(Task(task_id, task_size, data_size, affinities, data_dependencies, task_dependencies))

        for i in range(50):
            machine_id = i + 1
            power = np.random.randint(1, 21)

            machines.append(Machine(machine_id, power))

        machine_ids = [machine.id for machine in machines]

        for task in tasks:
            affinity_count = np.random.randint(1, 51)
            task.affinities = np.random.choice(machine_ids, size=affinity_count, replace=False).tolist()

        total_data_size = sum(task.data_size for task in tasks)
        min_disk_size = math.ceil(total_data_size / 30)
        max_disk_size = math.ceil(total_data_size / 30 * 3)

        for i in range(30):
            disk_id = i + 1

            if i == 0:
                speed = 1
                capacity = total_data_size
            else:
                speed = np.random.randint(1, 21)
                capacity = np.random.randint(min_disk_size, max_disk_size)

            disks.append(Disk(disk_id, speed, capacity))

        lines: list[str] = []

        lines.append(f"{len(tasks)}")
        for task in tasks:
            line = f"{task.id} {task.task_size} {task.data_size} {len(task.affinities)}"
            if len(task.affinities) > 0:
                line += " " + " ".join(map(str, task.affinities))

            lines.append(line)

        lines.append(f"{len(machines)}")
        for machine in machines:
            lines.append(f"{machine.id} {machine.power}")

        lines.append(f"{len(disks)}")
        for disk in disks:
            lines.append(f"{disk.id} {disk.speed} {disk.capacity}")

        lines.append(f"{sum(len(task.data_dependencies) for task in tasks)}")
        for task in tasks:
            for t in task.data_dependencies:
                lines.append(f"{t} {task.id}")

        lines.append(f"{sum(len(task.task_dependencies) for task in tasks)}")
        for task in tasks:
            for t in task.task_dependencies:
                lines.append(f"{t} {task.id}")

        return "\n".join(lines) + "\n"

    def create_adjacency_matrix(self) -> np.ndarray:
        raise NotImplementedError()

@dataclass
class FileGenerator(Generator):
    data_file_name: str
    amount: int

    def create_adjacency_matrix(self) -> np.ndarray:
        tree = ElementTree.parse(str(Path(__file__).parent / "data" / self.data_file_name))

        id_to_index: dict[str, int] = {}
        for job in tree.findall("{http://pegasus.isi.edu/schema/DAX}job"):
            id_to_index[job.attrib["id"]] = len(id_to_index)

        single_task_count = len(id_to_index)
        single = np.zeros((single_task_count, single_task_count))

        for child in tree.findall("{http://pegasus.isi.edu/schema/DAX}child"):
            child_index = id_to_index[child.attrib["ref"]]

            for parent in child.findall("{http://pegasus.isi.edu/schema/DAX}parent"):
                parent_index = id_to_index[parent.attrib["ref"]]

                single[child_index, parent_index] = 1

        full = np.zeros((self.amount * single_task_count, self.amount * single_task_count))
        for i in range(self.amount):
            low = i * single_task_count
            high = (i + 1) * single_task_count
            full[low:high, low:high] = single

        return full

@dataclass
class RandomGenerator(Generator):
    no_tasks: int
    max_dependencies: int

    def create_adjacency_matrix(self) -> np.ndarray:
        if self.max_dependencies == 0:
            return np.zeros((self.no_tasks, self.no_tasks))

        arr = np.zeros((self.no_tasks ** 2,))

        arr[-self.max_dependencies:] = 1
        np.random.shuffle(arr)

        return np.tril(arr.reshape((self.no_tasks, self.no_tasks)), k=-1)

def main() -> None:
    np.random.seed(0)

    generators: dict[str, Generator] = {
        "montage-25": FileGenerator("Montage_25.xml", 1),
        "montage-50": FileGenerator("Montage_50.xml", 1),
        "montage-100": FileGenerator("Montage_100.xml", 1),
        "montage-1000": FileGenerator("Montage_1000.xml", 1),
        "montage-5000": FileGenerator("Montage_1000.xml", 5),
        "montage-10000": FileGenerator("Montage_1000.xml", 10),
        "cybershake-30": FileGenerator("CyberShake_30.xml", 1),
        "cybershake-50": FileGenerator("CyberShake_50.xml", 1),
        "cybershake-100": FileGenerator("CyberShake_100.xml", 1),
        "cybershake-1000": FileGenerator("CyberShake_1000.xml", 1),
        "cybershake-5000": FileGenerator("CyberShake_1000.xml", 5),
        "cybershake-10000": FileGenerator("CyberShake_1000.xml", 10),
        "epigenomics-24": FileGenerator("Epigenomics_24.xml", 1),
        "epigenomics-46": FileGenerator("Epigenomics_46.xml", 1),
        "epigenomics-100": FileGenerator("Epigenomics_100.xml", 1),
        "epigenomics-997": FileGenerator("Epigenomics_997.xml", 1),
        "epigenomics-10000": FileGenerator("Epigenomics_100.xml", 100),
        "inspiral-30": FileGenerator("Inspiral_30.xml", 1),
        "inspiral-50": FileGenerator("Inspiral_50.xml", 1),
        "inspiral-100": FileGenerator("Inspiral_100.xml", 1),
        "inspiral-1000": FileGenerator("Inspiral_1000.xml", 1),
        "inspiral-5000": FileGenerator("Inspiral_1000.xml", 5),
        "inspiral-10000": FileGenerator("Inspiral_1000.xml", 10),
        "sipht-30": FileGenerator("Sipht_30.xml", 1),
        "sipht-60": FileGenerator("Sipht_60.xml", 1),
        "sipht-100": FileGenerator("Sipht_100.xml", 1),
        "sipht-1000": FileGenerator("Sipht_1000.xml", 1),
        "sipht-5000": FileGenerator("Sipht_1000.xml", 5),
        "sipht-10000": FileGenerator("Sipht_1000.xml", 10),
        "random-6-0": RandomGenerator(6, 0),
        "random-10-0": RandomGenerator(10, 0),
        "random-50-0": RandomGenerator(50, 0),
        "random-100-0": RandomGenerator(100, 0),
        "random-500-0": RandomGenerator(500, 0),
        "random-1000-0": RandomGenerator(1_000, 0),
        "random-2000-0": RandomGenerator(2_000, 0),
        "random-3000-0": RandomGenerator(3_000, 0),
        "random-5000-0": RandomGenerator(5_000, 0),
        "random-10000-0": RandomGenerator(10_000, 0),
        "random-6-10": RandomGenerator(6, 10),
        "random-10-25": RandomGenerator(10, 25),
        "random-50-1000": RandomGenerator(50, 1_250),
        "random-100-5000": RandomGenerator(100, 5_000),
        "random-500-125000": RandomGenerator(500, 125_000),
        "random-1000-500000": RandomGenerator(1_000, 500_000),
        "random-2000-500000": RandomGenerator(2_000, 500_000),
        "random-3000-500000": RandomGenerator(3_000, 500_000),
        "random-5000-500000": RandomGenerator(5_000, 500_000),
        "random-10000-500000": RandomGenerator(10_000, 500_000)
    }

    for name, generator in generators.items():
        input_data = generator.generate()

        input_file = Path(__file__).parent / "input" / f"{name}.in"
        with input_file.open("w+", encoding="utf-8") as file:
            file.write(input_data)

        print(f"Successfully generated data for input {name}")

if __name__ == "__main__":
    main()
