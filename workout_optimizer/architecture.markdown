# Workout Optimizer Architecture

The `workout_optimizer` is a Rust-based command-line tool designed to generate an optimized 6-day workout routine using simulated annealing. The architecture is modular, separating concerns into distinct components for maintainability and clarity.

## System Overview
The system takes a set of exercises and muscle group volume targets as input, applies optimization to create a balanced workout routine, and outputs the result as a Markdown file. It uses simulated annealing to explore the solution space, balancing constraints like time limits, recovery periods, and exercise variety.

## Modules and Responsibilities

1. **Main (`src/main.rs`)**
   - **Purpose**: Entry point of the application.
   - **Responsibilities**:
     - Initializes the exercise and muscle group data.
     - Calls the optimization routine.
     - Saves the output to a Markdown file.
     - Prints a confirmation message with the number of days optimized.

2. **Structs (`src/structs.rs`)**
   - **Purpose**: Defines core data structures.
   - **Data Structures**:
     - `Exercise`: Represents an exercise with properties like name, primary/secondary/isometric muscles, compound status, leg status, and critical flag.
     - `MuscleGroup`: Defines a muscle group with target and upper-bound weekly volume.
     - `RoutineEntry`: Represents a single exercise in a day’s routine with its name and number of sets.
   - **Responsibilities**:
     - Provides reusable data models for the rest of the application.

3. **Data (`src/data.rs`)**
   - **Purpose**: Stores static data for exercises and muscle groups.
   - **Components**:
     - `muscle_recovery_days()`: Returns a map of muscle groups to their recovery periods (e.g., Quads: 2 days).
     - `exercises()`: Returns a list of exercises with their properties (e.g., Squat targets Quads and Glutes).
     - `mav_targets()`: Returns muscle group volume targets and upper bounds (e.g., Quads: 12–18 sets).
   - **Responsibilities**:
     - Centralizes data definitions for consistency across the application.

4. **Routines (`src/routines.rs`)**
   - **Purpose**: Handles routine generation and optimization.
   - **Components**:
     - `initialize_routine()`: Creates an initial 6-day routine with 3–6 exercises per day, prioritizing one compound exercise and critical isolations.
     - `perturb_routine()`: Modifies a routine by adjusting sets, adding/removing exercises, or swapping exercises between days.
     - `optimize_routine()`: Uses simulated annealing to iteratively improve the routine, minimizing a cost function.
   - **Responsibilities**:
     - Generates and refines routines to meet volume targets and constraints.

5. **Utils (`src/utils.rs`)**
   - **Purpose**: Provides utility functions and constants.
   - **Constants**:
     - `TOTAL_DAYS`: 6 (number of workout days).
     - `TIME_PER_SET`: 2.0 minutes.
     - `MIN/MAX_EXERCISES_PER_DAY`: 3–6 exercises.
     - `MIN/MAX_SETS`: 2–5 sets per exercise.
     - `MAX_TIME_PER_DAY`: 50 minutes.
   - **Functions**:
     - `is_muscle_recently_used()`: Checks if a muscle was used within its recovery period.
     - `compute_volumes()`: Calculates weekly volume per muscle group based on routine sets.
     - `compute_cost()`: Computes a cost score based on volume deviations, time penalties, and constraint violations.
     - `save_to_file()`: Writes the routine to a Markdown file with daily details and volume breakdown.
   - **Responsibilities**:
     - Supports core logic with calculations and output formatting.

## Optimization Approach
- **Algorithm**: Simulated annealing with 75,000 iterations, an initial temperature of 1000.0, and a cooling rate of 0.993.
- **Cost Function**: Combines penalties for:
  - Volume deficits (weighted higher for critical muscles like Quads).
  - Exceeding volume upper bounds.
  - Violating time limits (50 minutes/day).
  - Missing compound exercises or "Leg Curl".
  - Excessive exercise frequency (>2 times/week).
  - Uneven daily time distribution.
- **Perturbations**: Randomly modify the routine by adjusting sets, adding/removing exercises, or swapping exercises between days.

## Data Flow
1. **Input**: `main.rs` loads exercises and muscle targets from `data.rs`.
2. **Initialization**: `routines.rs` creates an initial routine using `initialize_routine`.
3. **Optimization**: `optimize_routine` iteratively perturbs the routine, evaluating costs with `utils.rs` functions.
4. **Output**: `save_to_file` in `utils.rs` writes the optimized routine to `workout_routine.md`.

## Dependencies
- **Rust Standard Library**: For collections (`HashMap`, `HashSet`), file I/O, and basic utilities.
- **rand (0.8.5)**: For random number generation in routine initialization and perturbation.

## Design Principles
- **Modularity**: Each module handles a specific concern (data, logic, utilities).
- **Encapsulation**: Structs encapsulate exercise and muscle properties; functions hide implementation details.
- **Extensibility**: New exercises or muscle groups can be added to `data.rs` without changing core logic.
- **Reliability**: Constraints are enforced via the cost function, ensuring valid routines.