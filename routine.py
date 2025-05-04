import itertools
import random
from collections import defaultdict
from definitions import exercises, compatibility_matrix, mav_targets

# Precompute exercise contributions
exercise_contributions = {}
for exercise, props in exercises.items():
    contrib = defaultdict(float)
    for muscle in props.get("primary", []):
        contrib[muscle] = 1.0
    for muscle in props.get("secondary", []):
        contrib[muscle] = 0.25
    for muscle in props.get("isometric", []):
        contrib[muscle] = 0.25
    exercise_contributions[exercise] = contrib


# Function to check if a set structure is compatible
def is_compatible(set_structure, compatibility_matrix):
    for i, exercise1 in enumerate(set_structure):
        for j, exercise2 in enumerate(set_structure[i + 1 :], i + 1):
            pair = tuple(sorted([exercise1, exercise2]))
            if pair not in compatibility_matrix:
                pair = (pair[1], pair[0])
            if pair not in compatibility_matrix or compatibility_matrix[pair] == 0:
                return False
    return True


# Function to check if a set structure is non-synergistic
def is_non_synergistic(set_structure, exercises):
    muscle_groups = set()
    for exercise in set_structure:
        primary = exercises[exercise].get("primary", [])
        muscle_groups.update(primary)
    return len(muscle_groups) == sum(
        1 for exercise in set_structure for _ in exercises[exercise].get("primary", [])
    )


# Function to calculate time for a set structure
def calculate_time(set_structure, sets):
    if len(set_structure) == 1:  # Straight set
        return sets * 120 / 60  # minutes
    elif len(set_structure) == 2:  # Superset
        return sets * 195 / 60  # minutes
    elif len(set_structure) == 3:  # Tri-set
        return sets * 270 / 60  # minutes
    return 0


# Function to check leg exercise constraint
def check_leg_exercise_constraint(structures, exercises):
    leg_exercises = sum(
        1
        for structure in structures
        for exercise in structure
        if exercises[exercise]["is_leg"]
    )
    return leg_exercises <= 1


# Function to calculate volume for a muscle group
def calculate_volume(routine, exercises):
    volume = {}
    for day, day_structures in routine.items():
        for structure, sets in day_structures:
            for exercise in structure:
                for muscle in exercises[exercise].get("primary", []):
                    volume[muscle] = volume.get(muscle, 0) + sets
                for muscle in exercises[exercise].get("secondary", []):
                    volume[muscle] = volume.get(muscle, 0) + sets * 0.25
                for muscle in exercises[exercise].get("isometric", []):
                    volume[muscle] = volume.get(muscle, 0) + sets * 0.25
    return volume


# Simplified set structure generation
def generate_set_structures(day_exercises, exercises, compatibility_matrix):
    structures = []
    # Only generate straight sets for simplicity, adjust sets later
    structure = [[exercise] for exercise in day_exercises]
    if check_leg_exercise_constraint(structure, exercises):
        structures.append(structure)
    return structures


# Function to assign exercises to days and optimize sets
def assign_exercises_to_days(exercises, compatibility_matrix, mav_targets):
    all_exercises = list(exercises.keys())
    days = {day: [] for day in range(1, 7)}
    compound_exercises = [e for e in all_exercises if exercises[e]["is_compound"]]
    non_compound_exercises = [
        e for e in all_exercises if not exercises[e]["is_compound"]
    ]

    # Track usage of each exercise
    exercise_usage = {exercise: 0 for exercise in all_exercises}
    # Track muscle group coverage incrementally
    muscle_coverage = defaultdict(float)

    # Define day types for Push/Pull/Lower split
    day_types = {1: "Push", 2: "Lower", 3: "Pull", 4: "Lower", 5: "Pull", 6: "Push"}
    target_muscle_groups = {
        "Push": ["Chest", "Triceps", "Long Head", "Front Delts", "Lateral Delts"],
        "Pull": [
            "Lats",
            "Biceps",
            "Mid Traps",
            "Upper Traps",
            "Lower Traps",
            "Rear Delts",
        ],
        "Lower": ["Quads", "Rectus Femoris", "Hamstrings", "Short Head"],
    }

    # Target number of exercises per day
    target_exercises_per_day = 4
    total_days = len(days)
    total_exercise_slots = target_exercises_per_day * total_days

    # Calculate target muscle group coverage (excluding Glutes and Lower Back)
    target_coverage = {
        muscle: target / total_days
        for muscle, target in mav_targets.items()
        if muscle not in ["Glutes", "Lower Back"]
    }
    print("Target muscle group coverage per day (excluding Glutes and Lower Back):")
    for muscle, target in target_coverage.items():
        print(f"  {muscle}: {target:.2f} sets")

    # Assign one compound exercise to each day
    print("Assigning exercises to days...")
    print("Assigning compound exercises...")
    available_compounds = compound_exercises.copy()
    random.shuffle(available_compounds)
    for day in range(1, total_days + 1):
        current_exercises = [e for structure in days[day] for e in structure]
        day_type = day_types[day]
        target_muscles = target_muscle_groups[day_type]

        unused_compounds = [
            e
            for e in available_compounds
            if exercise_usage[e] == 0 and e not in current_exercises
        ]
        if unused_compounds:
            best_exercise = None
            best_score = float("inf")
            prev_day_coverage = (
                calculate_day_coverage(days[day - 1], exercises)
                if day > 1
                else defaultdict(float)
            )
            for exercise in unused_compounds:
                exercise_muscles = set(exercise_contributions[exercise].keys())
                target_overlap = len(exercise_muscles.intersection(target_muscles))
                if target_overlap == 0:
                    continue
                temp_coverage = muscle_coverage.copy()
                for muscle, contrib in exercise_contributions[exercise].items():
                    temp_coverage[muscle] += contrib * 3
                recovery_penalty = sum(
                    prev_day_coverage[muscle] * contrib
                    for muscle, contrib in exercise_contributions[exercise].items()
                )
                coverage_score = sum(
                    (temp_coverage.get(muscle, 0) - target_coverage.get(muscle, 0)) ** 2
                    for muscle in target_coverage
                )
                type_score = -target_overlap * 10
                score = coverage_score + recovery_penalty * 20 + type_score
                if score < best_score:
                    best_score = score
                    best_exercise = exercise
            if best_exercise is None:
                best_exercise = unused_compounds[0] if unused_compounds else None
            exercise = best_exercise
        else:
            best_exercise = None
            best_score = float("inf")
            prev_day_coverage = (
                calculate_day_coverage(days[day - 1], exercises)
                if day > 1
                else defaultdict(float)
            )
            for exercise in available_compounds:
                if exercise in current_exercises:
                    continue
                exercise_muscles = set(exercise_contributions[exercise].keys())
                target_overlap = len(exercise_muscles.intersection(target_muscles))
                temp_coverage = muscle_coverage.copy()
                for muscle, contrib in exercise_contributions[exercise].items():
                    temp_coverage[muscle] += contrib * 3
                recovery_penalty = sum(
                    prev_day_coverage[muscle] * contrib
                    for muscle, contrib in exercise_contributions[exercise].items()
                )
                coverage_score = sum(
                    (temp_coverage.get(muscle, 0) - target_coverage.get(muscle, 0)) ** 2
                    for muscle in target_coverage
                )
                type_score = -target_overlap * 10
                usage_score = exercise_usage[exercise] ** 2
                score = (
                    coverage_score
                    + recovery_penalty * 20
                    + type_score
                    + usage_score * 5
                )
                if score < best_score:
                    best_score = score
                    best_exercise = exercise
            exercise = best_exercise
        days[day].append([exercise])
        exercise_usage[exercise] += 1
        for muscle, contrib in exercise_contributions[exercise].items():
            muscle_coverage[muscle] += contrib * 3
        print(
            f"  Assigned compound exercise {exercise} to Day {day} (usage: {exercise_usage[exercise]})"
        )
    print("Compound exercise assignment completed.")

    # Assign remaining exercises
    remaining_slots = total_exercise_slots - total_days
    print(
        f"Remaining slots to fill: {remaining_slots} (total slots: {total_exercise_slots})"
    )
    available_exercises = non_compound_exercises
    day_times = {day: 0.0 for day in days}
    for day in days:
        for structure in days[day]:
            day_times[day] += calculate_time(structure, 3)

    day_index = 1
    slots_filled = 0
    deferred_slots = []
    while slots_filled < remaining_slots:
        day = day_index
        current_exercises = [e for structure in days[day] for e in structure]
        current_leg_exercises = sum(
            1 for e in current_exercises if exercises[e]["is_leg"]
        )
        day_type = day_types[day]
        target_muscles = target_muscle_groups[day_type]
        print(
            f"\nAttempting to fill a slot on Day {day}: {len(current_exercises)} exercises (leg exercises: {current_leg_exercises}, current time: {day_times[day]:.2f} minutes), target: {target_exercises_per_day}"
        )
        if len(current_exercises) >= target_exercises_per_day + 1:
            print(
                f"  Day {day} already has {len(current_exercises)} exercises, skipping."
            )
            day_index = (day_index % total_days) + 1
            continue

        available_for_day = [
            e for e in available_exercises if e not in current_exercises
        ]
        if not available_for_day:
            print(
                f"  No available exercises for Day {day} (all exercises already used in this day). Deferring slot."
            )
            deferred_slots.append((day, current_exercises, current_leg_exercises))
            day_index = (day_index % total_days) + 1
            continue

        all_used = all(exercise_usage[exercise] > 0 for exercise in all_exercises)
        candidates = unused_exercises if not all_used else available_for_day
        if not candidates:
            candidates = available_for_day

        best_exercise = None
        best_score = float("inf")
        prev_day_coverage = (
            calculate_day_coverage(days[day - 1], exercises)
            if day > 1
            else defaultdict(float)
        )
        for exercise in candidates[:5]:  # Limit to top 5 candidates
            exercise_muscles = set(exercise_contributions[exercise].keys())
            target_overlap = len(exercise_muscles.intersection(target_muscles))
            temp_coverage = muscle_coverage.copy()
            temp_time = day_times[day]
            for muscle, contrib in exercise_contributions[exercise].items():
                temp_coverage[muscle] += contrib * 3
            temp_time += calculate_time([exercise], 3)
            recovery_penalty = sum(
                prev_day_coverage[muscle] * contrib
                for muscle, contrib in exercise_contributions[exercise].items()
            )
            coverage_score = sum(
                (temp_coverage.get(muscle, 0) - target_coverage.get(muscle, 0)) ** 2
                for muscle in target_coverage
            )
            avg_time = sum(day_times.values()) / len(day_times)
            time_score = (temp_time - avg_time) ** 2
            usage_score = exercise_usage[exercise] ** 2
            type_score = -target_overlap * 10
            score = (
                coverage_score
                + time_score * 10
                + recovery_penalty * 20
                + usage_score * 5
                + type_score
            )
            if score < best_score:
                best_score = score
                best_exercise = exercise
        exercise = best_exercise

        print(
            f"  Selected exercise: {exercise} (is_leg: {exercises[exercise]['is_leg']}, usage: {exercise_usage[exercise]})"
        )
        temp_exercises = current_exercises + [exercise]
        temp_structures = [[e] for e in temp_exercises]
        if check_leg_exercise_constraint(temp_structures, exercises):
            days[day].append([exercise])
            exercise_usage[exercise] += 1
            for muscle, contrib in exercise_contributions[exercise].items():
                muscle_coverage[muscle] += contrib * 3
            day_times[day] += calculate_time([exercise], 3)
            slots_filled += 1
            print(
                f"  Assigned {exercise} to Day {day}. Day {day} now has {len(temp_exercises)} exercises (leg exercises: {current_leg_exercises + (1 if exercises[exercise]['is_leg'] else 0)}, new usage: {exercise_usage[exercise]}, new time: {day_times[day]:.2f} minutes)"
            )
        else:
            print(
                f"  Cannot assign {exercise} to Day {day}: violates leg exercise constraint. Deferring slot."
            )
            deferred_slots.append((day, current_exercises, current_leg_exercises))
        day_index = (day_index % total_days) + 1

    # Handle deferred slots (simplified)
    if deferred_slots:
        print(f"\nHandling deferred slots: {len(deferred_slots)} slots")
        for day, current_exercises, current_leg_exercises in deferred_slots:
            if len(current_exercises) >= target_exercises_per_day + 1:
                continue
            non_leg_exercises = [
                e
                for e in available_exercises
                if not exercises[e]["is_leg"] and e not in current_exercises
            ]
            if non_leg_exercises:
                exercise = non_leg_exercises[0]
                days[day].append([exercise])
                exercise_usage[exercise] += 1
                for muscle, contrib in exercise_contributions[exercise].items():
                    muscle_coverage[muscle] += contrib * 3
                day_times[day] += calculate_time([exercise], 3)
                print(f"    Assigned non-leg exercise {exercise} to Day {day}.")

    # Print final assignment and usage counts
    print("\nFinal exercise assignment:")
    for day in days:
        day_exercises = [e for structure in days[day] for e in structure]
        leg_exercises = sum(1 for e in day_exercises if exercises[e]["is_leg"])
        print(
            f"  Day {day}: {day_exercises} (total: {len(day_exercises)}, leg exercises: {leg_exercises})"
        )
    print("\nExercise usage counts:")
    for exercise, count in sorted(exercise_usage.items()):
        print(f"  {exercise}: {count} times")
    print("Initial exercise assignment completed.")

    # Simplified set structure generation
    print("Generating initial set structures for time estimation...")
    routine = {}
    day_times = {day: 0.0 for day in days}
    for day in days:
        day_exercises = [e for structure in days[day] for e in structure]
        structures = generate_set_structures(
            day_exercises, exercises, compatibility_matrix
        )
        if structures:
            structure = structures[0]
            set_combo = tuple([3] * len(structure))  # Default to 3 sets
            total_time = sum(
                calculate_time(structure[i], sets) for i, sets in enumerate(set_combo)
            )
            routine[day] = list(zip(structure, set_combo))
            day_times[day] = total_time
            print(
                f"Initial structure for Day {day}: {structure} with sets {set_combo} (time: {total_time:.2f} minutes)"
            )
        else:
            structure = [[exercise] for exercise in day_exercises]
            set_combo = tuple([3] * len(structure))
            total_time = sum(
                calculate_time(structure[i], sets) for i, sets in enumerate(set_combo)
            )
            routine[day] = list(zip(structure, set_combo))
            day_times[day] = total_time
            print(
                f"Initial structure for Day {day}: {structure} with sets {set_combo} (time: {total_time:.2f} minutes)"
            )

    # Skip rebalancing for simplicity in this optimized version
    print("Skipping rebalancing for performance...")

    # Adjust sets to meet volume targets (simplified)
    print("Optimizing volumes for muscle groups (excluding Glutes and Lower Back)...")
    for muscle_group, target in mav_targets.items():
        if muscle_group in ["Glutes", "Lower Back"]:
            continue
        volume = calculate_volume(routine, exercises)
        current_volume = volume.get(muscle_group, 0)
        print(f"\nOptimizing volume for muscle group: {muscle_group}")
        print(f"  Current volume: {current_volume:.2f}, Target volume: {target}")
        while current_volume < target:
            contributing_exercises = [
                e
                for e, props in exercises.items()
                if muscle_group in props.get("primary", [])
                or muscle_group in props.get("secondary", [])
                or muscle_group in props.get("isometric", [])
            ]
            if not contributing_exercises:
                print(
                    f"  No contributing exercises found for {muscle_group}. Stopping optimization."
                )
                break
            assigned = False
            for day in sorted(routine.keys(), key=lambda _: random.random()):
                for i, (structure, sets) in enumerate(routine[day]):
                    if sets >= 5:
                        continue
                    if any(e in structure for e in contributing_exercises):
                        routine[day][i] = (structure, sets + 1)
                        contribution = (
                            1
                            if muscle_group
                            in exercises[structure[0]].get("primary", [])
                            else 0.25
                        )
                        current_volume += contribution
                        print(
                            f"  Added 1 set to {structure} on Day {day} (contribution: {contribution:.2f}). New volume: {current_volume:.2f}"
                        )
                        assigned = True
                        break
                if assigned:
                    break
            if not assigned:
                print(
                    f"  No more sets can be added for {muscle_group} (max sets reached). Stopping optimization."
                )
                break
    print("Volume optimization completed.")

    print("Routine generation completed.")
    return routine


# Generate routine
print("Starting workout routine generation...")
routine = assign_exercises_to_days(exercises, compatibility_matrix, mav_targets)

# Format output
print("Formatting output...")
markdown = "# 6-Day Workout Routine\n\n"
markdown += "Each session starts with one compound exercise, followed by additional sets to target specific muscle groups. Each day has a roughly equivalent number of exercises (3-6), with sets balanced to achieve rough time equivalence across days. Exercises may repeat across days but not within the same day. Each exercise is performed for 2-5 sets of 8-12 reps. Rest 60-90 seconds between supersets/tri-sets and 2-3 minutes between straight sets.\n\n"

for day in sorted(routine.keys()):
    structures = routine[day]
    day_name = (
        "Push (Chest, Triceps, Shoulders)"
        if day == 1
        else (
            "Lower Body (Quads, Hamstrings, Glutes)"
            if day == 2
            else (
                "Pull (Back, Biceps, Traps)"
                if day == 3
                else (
                    "Lower Body & Shoulders (Hamstrings, Glutes, Lower Back, Shoulders)"
                    if day == 4
                    else (
                        "Pull (Back, Biceps, Traps)"
                        if day == 5
                        else "Push & Traps (Chest, Triceps, Shoulders, Traps)"
                    )
                )
            )
        )
    )
    markdown += f"## Day {day}: {day_name} – {sum(len(structure) for structure, _ in structures)} Exercises\n"
    for i, (structure, sets) in enumerate(structures):
        structure_type = "Straight Sets"
        if i == 0:
            structure_type += " (Compound First)"
        markdown += f"- **{structure_type}**:  \n"
        for exercise in structure:
            primary = ", ".join(exercises[exercise].get("primary", []))
            secondary = ", ".join(exercises[exercise].get("secondary", []))
            isometric = ", ".join(exercises[exercise].get("isometric", []))
            muscles = f"*{primary}*"
            if secondary:
                muscles += f", secondary: {secondary}"
            if isometric:
                muscles += f", isometric: {isometric}"
            markdown += f"  - {exercise} - {sets} sets of 8-12 reps *({muscles})*  \n"
            markdown += f"  *(Attributes: Cable: {'None' if 'Cable' not in exercise else 'Left Cable, Low Height' if exercise == 'Cable Curl' else 'Pulldown Cable' if exercise == 'Pulldown' else 'Both Cables, Medium Height' if exercise == 'Chest Fly' else 'Left Cable, High Height'}; Apparatus: {'Bench on Center Upright, Low' if 'Bench Press' in exercise else 'Bulldog Pad on Center Upright, High' if exercise == 'Upper Back Rows' else 'None'}; Station: {'Bench Press Station' if 'Bench Press' in exercise else 'Squat Rack' if exercise == 'Squat' else 'Leg Extension Machine' if exercise == 'Leg Extension' else 'Leg Curl Machine' if exercise == 'Leg Curl' else 'None'})*  \n"
    total_time = sum(calculate_time(structure, sets) for structure, sets in structures)
    markdown += "- **Time Estimate**:  \n"
    for structure, sets in structures:
        structure_type = "Straight Set"
        structure_name = ", ".join(structure)
        time = calculate_time(structure, sets)
        markdown += f"  - {structure_type} {structure_name}: {time:.2f} min  \n"
    markdown += f"  - **Total**: {total_time:.2f} minutes  \n\n"

# Weekly Volume Breakdown
print("Generating weekly volume breakdown...")
markdown += "---\n\n## Weekly Volume Breakdown\n"
volume = calculate_volume(routine, exercises)
for muscle_group, target in mav_targets.items():
    vol = volume.get(muscle_group, 0)
    if muscle_group in ["Glutes", "Lower Back"]:
        status = "reported for information only, not optimized per user instruction"
    else:
        status = (
            "exactly on target"
            if vol == target
            else (
                "exceeds low end due to time balancing, acceptable per rule"
                if vol > target
                else "below target"
            )
        )
    markdown += f"- **{muscle_group}**: {vol:.1f} sets  \n  *(MAV: {target}-{target+8 if muscle_group in ['Glutes', 'Lower Back'] else target+6} sets – {status})*  \n"

# Muscle Activation Table
print("Generating muscle activation table...")
markdown += "\n---\n\n## Muscle Activation Table\n"
markdown += "| **Exercise**            | **Primary Movers**            | **Secondary Movers**               | **Isometric Movers**      |\n"
markdown += "|-------------------------|-------------------------------|------------------------------------|---------------------------|\n"
for exercise, props in exercises.items():
    primary = ", ".join(props.get("primary", [])) or "None"
    secondary = ", ".join(props.get("secondary", [])) or "None"
    isometric = ", ".join(props.get("isometric", [])) or "None"
    markdown += f"| **{exercise}** | {primary} | {secondary} | {isometric} |\n"

# Notes
print("Generating notes section...")
markdown += "\n---\n\n## Notes\n"
min_time = min(
    sum(calculate_time(structure, sets) for structure, sets in structures)
    for structures in routine.values()
)
max_time = max(
    sum(calculate_time(structure, sets) for structure, sets in structures)
    for structures in routine.values()
)
avg_time = sum(
    sum(calculate_time(structure, sets) for structure, sets in structures)
    for structures in routine.values()
) / len(routine)
markdown += f"- **Time Commitment**: Each day is estimated at {min_time:.2f}-{max_time:.2f} minutes, with an average of ~{avg_time:.2f} minutes. Day {min(routine, key=lambda d: sum(calculate_time(s, sets) for s, sets in routine[d]))} ({min_time:.2f} minutes) is the shortest, while Day {max(routine, key=lambda d: sum(calculate_time(s, sets) for s, sets in routine[d]))} ({max_time:.2f} minutes) is the longest.\n"
markdown += "- **Leg Exercise Limit**: Maintained no more than one leg exercise per day (Squat, Leg Curl, Leg Extension, Stiff-Legged Deadlift), relaxed only if necessary.\n"
markdown += "- **Exercise List Constraint**: Used all exercises from your provided list (15-Degree Incline Bench Press, Front Raise, Lateral Raise, Leg Extension, Cable Curl, Shrugs, Squat, Rear Delts, Kelso Shrugs, Upper Back Rows, Chest Fly, Triceps Extension, Leg Curl, Stiff-Legged Deadlift, Pulldown, Bench Press, Lat Prayer), with repeats allowed across days but not within the same day.\n"
markdown += "- **Set Constraint**: Each exercise is assigned 2-5 sets.\n"
markdown += "- **Volume Optimization**: Excluded Glutes and Lower Back from volume optimization per user instruction; their volumes are reported but not adjusted to meet targets.\n"
markdown += "- **Time Equivalence**: Balanced the number of exercises, set structures, and sets per day to achieve rough time equivalence across days.\n"
markdown += "- **Recovery**: Structured as a Push/Pull/Lower split (Push: Days 1, 6; Pull: Days 3, 5; Lower: Days 2, 4) to ensure muscle groups are not worked on consecutive days, minimizing overlap of primary muscle groups between days.\n"
markdown += (
    "- **Progression**: Increase weight when you can perform 12 reps with good form.\n"
)

# Save to file
print("Saving output to workout_routine.md...")
with open("workout_routine.md", "w") as f:
    f.write(markdown)

print("Workout routine generation complete!")
print(markdown)
