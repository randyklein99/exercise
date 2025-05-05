# Workout Optimizer Requirements

## Functional Requirements

1. **Workout Routine Generation**:
   - The system must generate a 6-day workout routine with 3–6 exercises per day.
   - Each day’s routine must start with one compound exercise, followed by isolation exercises.
   - The routine must include at least one instance of the "Leg Curl" exercise across the 6 days.

2. **Muscle Volume Optimization**:
   - The system must optimize weekly muscle volume to meet target ranges for each muscle group (e.g., Quads: 12–18 sets).
   - Muscle groups like Glutes and Lower Back are not optimized but must be tracked.

3. **Constraints**:
   - Each day must respect a maximum time limit of 50 minutes, assuming 2 minutes per set.
   - No more than 2 leg exercises per day.
   - Exercises must respect muscle recovery periods (e.g., 2 days for Quads, 1 day for Triceps).
   - No exercise can be repeated within the same day.
   - No exercise can appear more than twice across the week.

4. **Output**:
   - The system must save the routine to a Markdown file (`workout_routine.md`) with:
     - Daily exercise lists, including exercise name, sets, and muscle groups.
     - Time estimates per day.
     - Weekly volume breakdown per muscle group, indicating if targets are met.

## Non-Functional Requirements

1. **Performance**:
   - The optimization process should complete within a reasonable time (e.g., under 1 minute) on standard hardware.
   - Use simulated annealing to balance exploration and optimization efficiency.

2. **Maintainability**:
   - Code must be modular, with separate modules for data, structs, routines, and utilities.
   - Use clear naming conventions and documentation for functions and structs.

3. **Portability**:
   - The project must run on any system with Rust installed (version 2024 or compatible).
   - No external dependencies beyond the Rust standard library and `rand` crate.

4. **Reliability**:
   - The system must handle edge cases (e.g., empty exercise lists, invalid muscle targets) gracefully.
   - Output must be consistent across runs with the same seed for reproducibility.

## Dependencies
- Rust (edition 2024)
- `rand` crate (version 0.8.5) for random number generation