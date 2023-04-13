import argparse
import json
import subprocess
import sys
from pathlib import Path
from multiprocessing import Pool
from score import get_score
from typing import List

def update_overview() -> None:
    scores_by_solver = {}
    outputs_root = Path(__file__).parent.parent / "results" / "output"

    for directory in outputs_root.iterdir():
        scores_by_input = {}

        for file in directory.iterdir():
            if file.name.endswith(".txt"):
                scores_by_input[file.stem] = float(file.read_text(encoding="utf-8").strip())

        scores_by_solver[directory.name] = scores_by_input

    overview_template_file = Path(__file__).parent.parent / "results" / "overview.tmpl.html"
    overview_file = Path(__file__).parent.parent / "results" / "overview.html"

    overview_template = overview_template_file.read_text(encoding="utf-8")
    overview = overview_template.replace("/* scores_by_solver */{}", json.dumps(scores_by_solver))

    with overview_file.open("w+", encoding="utf-8") as file:
        file.write(overview)

    print(f"Overview: file://{overview_file.resolve()}")

def run_input(solver: Path, input: Path, output_directory: Path) -> int:
    stdout_file = output_directory / f"{input.stem}.out"
    stderr_file = output_directory / f"{input.stem}.log"

    with input.open("rb") as stdin, stdout_file.open("wb+") as stdout, stderr_file.open("wb+") as stderr:
            try:
                process = subprocess.run([str(solver)], stdin=stdin, stdout=stdout, stderr=stderr, timeout=15000)

                if process.returncode != 0:
                    raise RuntimeError(f"Solver exited with status code {process.returncode} for input {input.stem}")
            except subprocess.TimeoutExpired:
                raise RuntimeError(f"Solver timed out on input {input.stem}")

    input_data = input.read_text(encoding="utf-8")
    output_data = stdout_file.read_text(encoding="utf-8")

    try:
        return get_score(input_data, output_data)
    except ValueError as err:
        raise RuntimeError(f"Solver provided invalid output for input {input.stem}: {str(err)}")

def run(solver: Path, inputs: List[Path], output_directory: Path) -> None:
    if not output_directory.is_dir():
        output_directory.mkdir(parents=True)

    with Pool() as pool:
        try:
            scores = pool.starmap(run_input, [(solver, input, output_directory) for input in inputs])
        except RuntimeError as err:
            print(f"\033[91m{str(err)}\033[0m")
            sys.exit(1)

    for i, input in enumerate(inputs):
        print(f"{input.stem}: {scores[i]:,.3f}")

        score_file = output_directory / f"{input.stem}.txt"
        with score_file.open("w+", encoding="utf-8") as file:
            file.write(str(scores[i]))

    if len(inputs) > 1:
        print(f"Total score: {sum(scores):,.3f}")

def main() -> None:
    parser = argparse.ArgumentParser(description="Run a solver.")
    parser.add_argument("solver", type=str, help="the solver to run")
    parser.add_argument("--input", type=str, help="the input to run on (defaults to all inputs)")

    args = parser.parse_args()

    solver = Path(__file__).parent.parent / "cmake-build-release" / args.solver
    if not solver.is_file():
        raise RuntimeError(f"Solver not found, {solver} is not a file")

    output_directory = Path(__file__).parent / "output" / args.solver

    if args.input is not None:
        inputs = [Path(__file__).parent / "input" / f"{args.input}.in"]
        if not inputs[0].is_file():
            raise RuntimeError(f"Input not found, {inputs[0]} is not a file")
    else:
        inputs = sorted((Path(__file__).parent / "input").glob("*.in"))

    run(solver, inputs, output_directory)
    update_overview()

if __name__ == "__main__":
    main()
