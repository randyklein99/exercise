# Workout Optimizer

A Rust-based command-line tool to generate an optimized 6-day workout routine using simulated annealing. The tool ensures balanced muscle volume, respects recovery periods, and adheres to time and exercise constraints.

## Features
- Generates a 6-day workout routine with 3–6 exercises per day.
- Optimizes weekly muscle volume to meet target ranges (e.g., Quads: 12–18 sets).
- Enforces constraints like maximum 50-minute sessions and no more than 2 leg exercises per day.
- Saves the routine to a Markdown file with detailed exercise and volume breakdowns.
- Uses simulated annealing for efficient optimization.

## Prerequisites
- **Rust**: Version 2024 or later (install via [rustup](https://rust-lang.org/tools/install)).
- **Cargo**: Included with Rust for dependency management.

## Setup
1. Clone the repository:
   ```bash
   git clone <repository-url>
   cd workout_optimizer
   ```
2. Ensure the `rand` crate is included in `Cargo.toml`:
   ```toml
   [dependencies]
   rand = "0.8.5"
   ```
3. Build the project:
   ```bash
   cargo build
   ```

## Usage
1. Run the optimizer:
   ```bash
   cargo run
   ```
2. The program generates a routine and saves it to `workout_routine.md`.
3. View the output file for the detailed routine, including daily exercises, time estimates, and weekly muscle volume.

Example output (`workout_routine.md`):
```markdown
# 6-Day Workout Routine

## Day 1: 4 Exercises
- **Straight Sets (Compound First)**: Squat - 4 sets of 8-12 reps *(Quads, Glutes, secondary: Hamstrings, Lower Back, isometric: Lower Back)*
- **Straight Sets**: Leg Curl - 3 sets of 8-12 reps *(Hamstrings, Short Head)*
- **Straight Sets**: Lateral Raise - 4 sets of 8-12 reps *(Lateral Delts)*
- **Straight Sets**: Kelso Shrugs - 3 sets of 8-12 reps *(Mid Traps)*
- **Time Estimate**: 28 minutes

## Weekly Volume Breakdown
- **Quads**: 12.0 sets (12–18 sets – on target)
- **Glutes**: 10.0 sets (12–18 sets – not optimized)
...
```

## Project Structure
- `src/main.rs`: Entry point, orchestrates routine generation and output.
- `src/structs.rs`: Defines core data structures (`Exercise`, `MuscleGroup`, `RoutineEntry`).
- `src/data.rs`: Contains exercise definitions, muscle recovery periods, and volume targets.
- `src/routines.rs`: Implements routine initialization and optimization logic.
- `src/utils.rs`: Provides utility functions for volume calculation, cost computation, and file output.

## Contributing
1. Fork the repository.
2. Create a feature branch (`git checkout -b feature-name`).
3. Commit changes (`git commit -m "Add feature"`).
4. Push to the branch (`git push origin feature-name`).
5. Open a pull request.

## License
MIT License (see `LICENSE` file for details).