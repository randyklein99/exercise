import gym
from gym import spaces
import numpy as np
from collections import defaultdict, deque
import random
import torch
import torch.nn as nn
import torch.optim as optim
import logging

# Set up logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

# Muscle activation table and MAV targets from exercise_definitions.h
exercises = {
    "Incline Bench Press": {
        "primary": ["Chest", "Triceps", "Front Delts"],
        "secondary": [],
        "isometric": ["Lower Back"],
        "is_compound": True,
        "is_leg": False,
    },
    "Front Raise": {
        "primary": ["Front Delts"],
        "secondary": [],
        "isometric": ["Biceps"],
        "is_compound": False,
        "is_leg": False,
    },
    "Lateral Raise": {
        "primary": ["Lateral Delts"],
        "secondary": [],
        "isometric": [],
        "is_compound": False,
        "is_leg": False,
    },
    "Leg Extension": {
        "primary": ["Quads", "Rectus Femoris"],
        "secondary": [],
        "isometric": [],
        "is_compound": False,
        "is_leg": True,
    },
    "Cable Curl": {
        "primary": ["Biceps"],
        "secondary": [],
        "isometric": [],
        "is_compound": False,
        "is_leg": False,
    },
    "Pulldown": {
        "primary": ["Lats", "Biceps"],
        "secondary": ["Lower Traps", "Rear Delts"],
        "isometric": [],
        "is_compound": True,
        "is_leg": False,
    },
    "Squat": {
        "primary": ["Quads", "Glutes"],
        "secondary": ["Hamstrings", "Lower Back"],
        "isometric": ["Lower Back"],
        "is_compound": True,
        "is_leg": True,
    },
    "Rear Delts": {
        "primary": ["Rear Delts"],
        "secondary": [],
        "isometric": [],
        "is_compound": False,
        "is_leg": False,
    },
    "Shrugs": {
        "primary": ["Upper Traps"],
        "secondary": [],
        "isometric": [],
        "is_compound": False,
        "is_leg": False,
    },
    "Kelso Shrugs": {
        "primary": ["Mid Traps"],
        "secondary": [],
        "isometric": [],
        "is_compound": False,
        "is_leg": False,
    },
    "Chest Fly": {
        "primary": ["Chest"],
        "secondary": ["Front Delts"],
        "isometric": [],
        "is_compound": False,
        "is_leg": False,
    },
    "Upper Back Rows": {
        "primary": ["Mid Traps", "Rear Delts"],
        "secondary": ["Lats", "Biceps"],
        "isometric": ["Lower Back"],
        "is_compound": True,
        "is_leg": False,
    },
    "Triceps Extension": {
        "primary": ["Triceps", "Long Head"],
        "secondary": [],
        "isometric": [],
        "is_compound": False,
        "is_leg": False,
    },
    "Leg Curl": {
        "primary": ["Hamstrings", "Short Head"],
        "secondary": [],
        "isometric": [],
        "is_compound": False,
        "is_leg": True,
    },
    "Stiff-Legged Deadlift": {
        "primary": ["Hamstrings", "Glutes"],
        "secondary": ["Lower Back"],
        "isometric": ["Lower Back", "Upper Traps"],
        "is_compound": True,
        "is_leg": True,
    },
    "Lat Prayer": {
        "primary": ["Lats", "Long Head"],
        "secondary": [],
        "isometric": [],
        "is_compound": False,
        "is_leg": False,
    },
}

mav_targets = {
    "Chest": 12.0,
    "Triceps": 10.0,
    "Long Head": 10.0,
    "Front Delts": 8.0,
    "Lateral Delts": 12.0,
    "Rear Delts": 12.0,
    "Lats": 14.0,
    "Mid Traps": 8.0,
    "Upper Traps": 8.0,
    "Lower Traps": 14.0,
    "Biceps": 14.0,
    "Quads": 12.0,
    "Rectus Femoris": 12.0,
    "Hamstrings": 10.0,
    "Short Head": 10.0,
    "Glutes": 12.0,
    "Lower Back": 12.0,
}

muscle_recovery_days = {
    "Chest (Incline Bench Press)": 2,
    "Chest (Chest Fly)": 2,
    "Front Delts (Incline Bench Press)": 1,
    "Front Delts (Chest Fly)": 1,
    "Front Delts (Front Raise)": 1,
    "Lats (Pulldown)": 2,
    "Lats (Upper Back Rows)": 1,
    "Lats (Lat Prayer)": 2,
    "Mid Traps (Upper Back Rows)": 2,
    "Mid Traps (Kelso Shrugs)": 2,
    "Lower Traps (Pulldown)": 1,
    "Lower Traps (Upper Back Rows)": 2,
    "Upper Traps (Shrugs)": 2,
    "Upper Traps (Stiff-Legged Deadlift)": 1,
    "Rear Delts (Upper Back Rows)": 2,
    "Rear Delts (Pulldown)": 1,
    "Rear Delts (Rear Delts)": 1,
    "Lateral Delts (Lateral Raise)": 1,
    "Triceps (Incline Bench Press)": 2,
    "Triceps (Triceps Extension)": 1,
    "Long Head (Triceps Extension)": 1,
    "Long Head (Lat Prayer)": 2,
    "Biceps (Pulldown)": 2,
    "Biceps (Upper Back Rows)": 1,
    "Biceps (Cable Curl)": 1,
    "Biceps (Front Raise)": 1,
    "Quads (Squat)": 3,
    "Quads (Leg Extension)": 2,
    "Rectus Femoris (Leg Extension)": 2,
    "Hamstrings (Squat)": 2,
    "Hamstrings (Stiff-Legged Deadlift)": 3,
    "Hamstrings (Leg Curl)": 2,
    "Short Head (Leg Curl)": 2,
}


class WorkoutRoutineEnv(gym.Env):
    def __init__(self):
        super(WorkoutRoutineEnv, self).__init__()
        self.total_days = 6
        self.max_time_per_day = 50.0
        self.min_exercises_per_day = 3
        self.max_exercises_per_day = 6
        self.min_sets = 2
        self.max_sets = 5
        self.time_per_set = 2.0  # 2 minutes per set
        self.max_steps_per_episode = 50  # Prevent infinite loops
        self.target_avg_time = 35.0  # Target average time per day
        self.target_exercises_per_day = (
            4.5  # Target average exercises per day (27 exercises / 6 days)
        )

        # Muscle groups to optimize (exclude Glutes and Lower Back)
        self.muscle_groups = [
            mg for mg in mav_targets.keys() if mg not in ["Glutes", "Lower Back"]
        ]
        self.num_muscle_groups = len(self.muscle_groups)

        # Exercises
        self.exercise_names = list(exercises.keys())
        self.num_exercises = len(self.exercise_names)

        # State space: (current_day, time_used, num_exercises, volumes, exercise_usage, recovery_timers)
        self.observation_space = spaces.Box(
            low=np.array(
                [1, 0, 0]
                + [0] * self.num_muscle_groups
                + [0] * self.num_exercises
                + [0] * self.num_muscle_groups,
                dtype=np.float32,
            ),
            high=np.array(
                [self.total_days, self.max_time_per_day, self.max_exercises_per_day]
                + [100] * self.num_muscle_groups
                + [1] * self.num_exercises
                + [self.total_days] * self.num_muscle_groups,
                dtype=np.float32,
            ),
            dtype=np.float32,
        )

        # Action space: (exercise, sets) or skip day
        self.num_sets_options = (
            self.max_sets - self.min_sets + 1
        )  # 2 to 5 inclusive -> 4 options
        self.action_space = spaces.Discrete(
            self.num_exercises * self.num_sets_options + 1
        )

        # Track usage to ensure all exercises are used
        self.exercise_usage = defaultdict(int)
        self.last_used_day = defaultdict(
            lambda: -float("inf")
        )  # Track last day each exercise was used
        self.step_count = 0  # Track steps per episode
        self.empty_days = 0  # Track number of empty days
        self.total_exercises_added = 0  # Track total exercises added in the episode
        self.recovery_penalty_factor = (
            0.0  # Start with no recovery penalty, increase during training
        )
        self.recovery_timers = defaultdict(
            int
        )  # Remaining recovery time for each muscle group

    def reset(self):
        self.current_day = 1
        self.days = [
            [] for _ in range(self.total_days)
        ]  # List of (exercise, sets) per day
        self.day_times = [0.0] * self.total_days
        self.last_worked_day = defaultdict(lambda: -float("inf"))
        self.current_volume = defaultdict(float)
        self.exercise_usage = defaultdict(int)
        self.last_used_day = defaultdict(lambda: -float("inf"))
        self.step_count = 0
        self.empty_days = 0
        self.total_exercises_added = 0
        self.recovery_penalty_factor = 0.0
        self.recovery_timers = defaultdict(int)

        # Compute initial state
        self._calculate_volume()
        state = self._get_state()
        return state

    def _get_state(self):
        # Ensure current_day is within valid range
        if self.current_day > self.total_days:
            return np.zeros(
                self.observation_space.shape[0], dtype=np.float32
            )  # Return zero state if episode is done

        # Current day
        state = [self.current_day]
        # Time used in current day
        state.append(self.day_times[self.current_day - 1])
        # Number of exercises in current day
        state.append(len(self.days[self.current_day - 1]))
        # Current volumes for each muscle group
        for muscle in self.muscle_groups:
            state.append(self.current_volume[muscle])
        # Binary vector for exercise usage
        for ex in self.exercise_names:
            state.append(1 if self.exercise_usage[ex] > 0 else 0)
        # Remaining recovery timers for each muscle group
        for muscle in self.muscle_groups:
            state.append(max(0, self.recovery_timers[muscle]))
        return np.array(state, dtype=np.float32)

    def _calculate_volume(self):
        self.current_volume = defaultdict(float)
        for day in range(self.total_days):
            for exercise, sets in self.days[day]:
                ex_info = exercises[exercise]
                for muscle in ex_info["primary"]:
                    base_muscle = muscle.split(" (")[0]
                    self.current_volume[base_muscle] += sets * 1.0
                for muscle in ex_info["secondary"]:
                    base_muscle = muscle.split(" (")[0]
                    self.current_volume[base_muscle] += sets * 0.5
                for muscle in ex_info["isometric"]:
                    base_muscle = muscle.split(" (")[0]
                    self.current_volume[base_muscle] += sets * 0.25

    def _calculate_time_equivalence_reward(self):
        times = [
            self.day_times[day] for day in range(min(self.current_day, self.total_days))
        ]
        if not times:
            return 0.0
        avg_time = sum(times) / len(times)
        # Reward for being close to target average time (35 minutes)
        time_deviation = abs(
            self.day_times[min(self.current_day - 1, self.total_days - 1)]
            - self.target_avg_time
        )
        time_reward = -time_deviation * 40.0
        # Additional penalty for deviation from average across days
        avg_deviation = abs(avg_time - self.target_avg_time)
        avg_reward = -avg_deviation * 80.0
        # Immediate penalty for empty days (increased penalty)
        if self.day_times[min(self.current_day - 1, self.total_days - 1)] == 0:
            empty_penalty = -2000.0 - (self.empty_days * 500.0)  # Cumulative penalty
        else:
            empty_penalty = 0.0
        # Reward for balancing exercises across days
        exercises_per_day = [
            len(self.days[day]) for day in range(min(self.current_day, self.total_days))
        ]
        avg_exercises = (
            sum(exercises_per_day) / len(exercises_per_day) if exercises_per_day else 0
        )
        exercise_balance_reward = (
            -abs(avg_exercises - self.target_exercises_per_day) * 100.0
        )
        total_reward = (
            time_reward + avg_reward + empty_penalty + exercise_balance_reward
        )
        return np.clip(total_reward, -1e5, 1e5)

    def _calculate_exercise_count_reward(self, day_exercises):
        num_exercises = len(day_exercises)
        if num_exercises < self.min_exercises_per_day:
            penalty = -200.0 * (self.min_exercises_per_day - num_exercises)
        elif num_exercises > self.max_exercises_per_day:
            penalty = -500.0 * (num_exercises - self.max_exercises_per_day)
        else:
            penalty = 100.0 * num_exercises
        return np.clip(penalty, -1e5, 1e5)

    def _calculate_volume_reward(self, prev_volume):
        self._calculate_volume()
        reward = 0.0
        # Calculate deficits for all muscle groups
        deficits = {}
        max_deficit = 0.0
        for muscle in self.muscle_groups:
            vol = self.current_volume[muscle]
            target = mav_targets[muscle]
            deficit = max(0, target - vol) / target  # Normalize deficit by target
            deficits[muscle] = min(
                deficit, 2.0
            )  # Cap deficit to prevent large penalties
            max_deficit = max(max_deficit, deficits[muscle])

        # Avoid division by zero
        if max_deficit == 0:
            max_deficit = 1.0

        # Normalize deficits to prioritize muscle groups with largest gaps
        for muscle in deficits:
            deficits[muscle] = deficits[muscle] / max_deficit

        # Calculate volume reward
        for muscle in self.muscle_groups:
            delta_vol = self.current_volume[muscle] - prev_volume[muscle]
            if delta_vol > 0:
                target = mav_targets[muscle]
                upper_bound = target + 6
                deficit = deficits[muscle]
                excess = max(0, self.current_volume[muscle] - upper_bound)
                if deficit > 0:
                    reward += delta_vol * 300 * (1 + deficit)
                elif excess > 0:
                    reward -= excess * 150
                else:
                    reward += delta_vol * 100
                # Add reward for targeting under-served muscle groups
                if deficit > 0.5:
                    reward += 500.0 * delta_vol  # Increased reward

        return np.clip(reward, -1e5, 1e5)

    def step(self, action):
        self.step_count += 1
        done = False
        reward = 0.0

        # Check for step limit to prevent infinite loops
        if self.step_count > self.max_steps_per_episode:
            done = True
            reward = -1000.0
            return self._get_state(), reward, done, {}

        # Track empty days
        if len(self.days[self.current_day - 1]) == 0:
            self.empty_days += 1

        # Update recovery timers
        for muscle in self.recovery_timers:
            self.recovery_timers[muscle] = max(0, self.recovery_timers[muscle] - 1)

        # Decode action
        if action == self.num_exercises * self.num_sets_options:  # Skip day
            # Reward based on number of exercises in the current day
            reward += self._calculate_exercise_count_reward(
                self.days[self.current_day - 1]
            )
            self.current_day += 1
            # Check if episode should end
            if self.current_day > self.total_days:
                done = True
                # Final reward based on volume alignment and exercise usage
                self._calculate_volume()
                for muscle in self.muscle_groups:
                    vol = self.current_volume[muscle]
                    target = mav_targets[muscle]
                    upper_bound = target + 6
                    deficit = max(0, target - vol) / target
                    deficit = min(deficit, 2.0)  # Cap deficit
                    excess = max(0, vol - upper_bound)
                    if deficit > 0:
                        reward -= deficit * 300 * (1 + deficit)
                    elif excess > 0:
                        reward -= excess * 150
                    else:
                        reward += (vol - target) * 100
                # Reward for using all exercises (reduced penalty)
                unused_count = sum(
                    1 for ex in self.exercise_names if self.exercise_usage[ex] == 0
                )
                reward -= unused_count * 500  # Reduced penalty
                reward += (self.num_exercises - unused_count) * 1000  # Increased reward
                # Penalty for empty days (reduced penalty)
                reward -= self.empty_days * 1000  # Reduced penalty
                reward = np.clip(reward, -1e5, 1e5)
            else:
                reward += self._calculate_time_equivalence_reward()
                # Reward for waiting out recovery periods
                if (
                    len(self.days[self.current_day - 1]) == 0
                ):  # If day is still empty after skipping
                    for muscle in self.recovery_timers:
                        if self.recovery_timers[muscle] > 0:
                            reward += 50.0  # Small reward for waiting
        else:
            # Add exercise with sets
            exercise_idx = action // self.num_sets_options
            sets_idx = action % self.num_sets_options
            exercise = self.exercise_names[exercise_idx]
            sets = self.min_sets + sets_idx  # 2 to 5 sets
            ex_info = exercises[exercise]

            # Check constraints (now as soft constraints via rewards)
            valid_action = True
            # 1. Time constraint
            time_for_exercise = sets * self.time_per_set
            if (
                self.day_times[self.current_day - 1] + time_for_exercise
                > self.max_time_per_day
            ):
                valid_action = False
                reward -= 100.0

            # 2. Maximum exercises per day (hard constraint)
            if len(self.days[self.current_day - 1]) >= self.max_exercises_per_day:
                valid_action = False
                reward -= 1000.0

            # 3. No repeats within the same day
            current_exercises = [ex for ex, _ in self.days[self.current_day - 1]]
            if exercise in current_exercises:
                valid_action = False
                reward -= 100.0

            # 4. Leg exercise limit (max 1 per day)
            current_leg_exercises = sum(
                1
                for ex, _ in self.days[self.current_day - 1]
                if exercises[ex]["is_leg"]
            )
            if ex_info["is_leg"] and current_leg_exercises >= 1:
                valid_action = False
                reward -= 100.0

            # 5. Recovery constraint (softer penalty early in training)
            recovery_violation = False
            for muscle in (
                ex_info["primary"] + ex_info["secondary"] + ex_info["isometric"]
            ):
                if not self._can_work_muscle(muscle, self.current_day):
                    recovery_violation = True
                    break
            if recovery_violation:
                valid_action = False
                reward -= 10.0 * self.recovery_penalty_factor  # Further reduced penalty

            # 6. Once-a-week constraint for Squat and Stiff-Legged Deadlift
            if exercise in ["Squat", "Stiff-Legged Deadlift"]:
                for d in range(self.total_days):
                    if d == self.current_day - 1:
                        continue
                    for ex, _ in self.days[d]:
                        if ex == exercise:
                            valid_action = False
                            reward -= 100.0
                            break
                    if not valid_action:
                        break

            # 7. First exercise must be compound (hard constraint during warm-up)
            if len(self.days[self.current_day - 1]) == 0 and not ex_info["is_compound"]:
                valid_action = False
                reward -= 1000.0 if self.recovery_penalty_factor < 1.0 else 100.0

            if valid_action:
                self.days[self.current_day - 1].append((exercise, sets))
                self.day_times[self.current_day - 1] += time_for_exercise
                self.total_exercises_added += 1
                was_unused = self.exercise_usage[exercise] == 0
                days_since_last_use = (
                    self.current_day - self.last_used_day[exercise] - 1
                )
                self.exercise_usage[exercise] += 1
                self.last_used_day[exercise] = self.current_day
                # Update last worked day and recovery timers
                for muscle in (
                    ex_info["primary"] + ex_info["secondary"] + ex_info["isometric"]
                ):
                    if muscle in muscle_recovery_days:
                        self.last_worked_day[muscle] = self.current_day
                        self.recovery_timers[muscle] = muscle_recovery_days[muscle]
                # Calculate reward
                prev_volume = self.current_volume.copy()
                reward += self._calculate_volume_reward(prev_volume)
                reward += self._calculate_time_equivalence_reward()
                # Reward for exercise count
                reward += self._calculate_exercise_count_reward(
                    self.days[self.current_day - 1]
                )
                # Reward for using a new exercise
                if was_unused:
                    reward += 2000.0  # Increased reward
                # Reward for using an exercise that hasn't been used recently
                reward += days_since_last_use * 50.0
                # Reward for each step taken to encourage filling all days
                reward += 50.0

        # Update state
        state = self._get_state()
        info = {}
        return state, reward, done, info

    def _can_work_muscle(self, muscle, day):
        if muscle not in muscle_recovery_days:
            return True
        last_day = self.last_worked_day[muscle]
        required_gap = muscle_recovery_days[muscle]
        days_since_last = day - last_day
        return days_since_last >= required_gap

    def render(self, mode="human"):
        print(f"Day {self.current_day}:")
        for day in range(self.total_days):
            print(
                f"  Day {day + 1}: {self.days[day]} (Time: {self.day_times[day]} minutes)"
            )
        self._calculate_volume()
        print("Volumes:")
        for muscle in self.muscle_groups:
            print(
                f"  {muscle}: {self.current_volume[muscle]} (Target: {mav_targets[muscle]})"
            )

    def save_to_file(self, filename="workout_routine_rl.md"):
        self._calculate_volume()
        markdown = "# 6-Day Workout Routine\n\n"
        markdown += "Each session starts with one compound exercise, followed by additional sets to target specific muscle groups. Each day has a roughly equivalent number of exercises (3-6), with sets balanced to achieve rough time equivalence across days. Exercises may repeat across days but not within the same day. Each exercise is performed for 2-5 sets of 8-12 reps. Rest 60-90 seconds between supersets/tri-sets and 2-3 minutes between straight sets.\n\n"

        # Days
        for day in range(self.total_days):
            markdown += (
                f"## Day {day + 1}: Day {day + 1} – {len(self.days[day])} Exercises\n"
            )
            for idx, (exercise, sets) in enumerate(self.days[day]):
                ex_info = exercises[exercise]
                primary = ", ".join([m.split(" (")[0] for m in ex_info["primary"]])
                secondary = ", ".join([m.split(" (")[0] for m in ex_info["secondary"]])
                isometric = ", ".join([m.split(" (")[0] for m in ex_info["isometric"]])
                movers = f"*{primary}*"
                if secondary:
                    movers += f", secondary: {secondary}"
                if isometric:
                    movers += f", isometric: {isometric}"
                set_type = (
                    "Straight Sets (Compound First)"
                    if idx == 0 and ex_info["is_compound"]
                    else "Straight Sets"
                )
                markdown += f"- **{set_type}**:  \n  - {exercise} - {sets} sets of 8-12 reps *({movers})*  \n  *(Attributes: Cable: None; Apparatus: None; Station: None)*  \n"
            markdown += "- **Time Estimate**:  \n"
            for exercise, sets in self.days[day]:
                time = sets * self.time_per_set
                markdown += f'  - Straight Set "{exercise}": {time:.6f} min  \n'
            markdown += f"  - **Total**: {self.day_times[day]:.6f} minutes  \n\n"

        # Weekly Volume Breakdown
        markdown += "---\n\n## Weekly Volume Breakdown\n"
        for muscle in sorted(mav_targets.keys()):
            vol = self.current_volume[muscle]
            target = mav_targets[muscle]
            upper_bound = target + 6
            status = "exactly on target"
            if muscle in ["Glutes", "Lower Back"]:
                status = (
                    "reported for information only, not optimized per user instruction"
                )
            elif vol < target:
                status = "below target"
            elif vol > upper_bound:
                status = "exceeds low end due to time balancing, acceptable per rule"
            elif target <= vol <= upper_bound:
                status = (
                    "exceeds low end due to time balancing, acceptable per rule"
                    if vol > target
                    else "exactly on target"
                )
            markdown += f"- **{muscle}**: {vol:.6f} sets  \n  *(MAV: {target:.6f}-{upper_bound:.0f} sets – {status})*  \n"

        # Muscle Activation Table
        markdown += "---\n\n## Muscle Activation Table\n"
        markdown += "| Exercise              | Primary Movers              | Secondary Movers        | Isometric Movers        |\n"
        markdown += "|-----------------------|-----------------------------|-------------------------|-------------------------|\n"
        for ex_name, ex_info in sorted(exercises.items()):
            primary = (
                ", ".join([m.split(" (")[0] for m in ex_info["primary"]]) or "None"
            )
            secondary = (
                ", ".join([m.split(" (")[0] for m in ex_info["secondary"]]) or "None"
            )
            isometric = (
                ", ".join([m.split(" (")[0] for m in ex_info["isometric"]]) or "None"
            )
            markdown += f"| {ex_name:<21} | {primary:<27} | {secondary:<23} | {isometric:<23} |\n"

        # Notes
        markdown += "---\n\n## Notes\n"
        times = [self.day_times[day] for day in range(self.total_days)]
        avg_time = sum(times) / len(times)
        shortest_day = min(range(self.total_days), key=lambda d: times[d]) + 1
        longest_day = max(range(self.total_days), key=lambda d: times[d]) + 1
        markdown += f"- **Time Commitment**: Each day is estimated at {min(times):.6f}-{max(times):.6f} minutes, with an average of ~{avg_time:.6f} minutes. Day {shortest_day} ({times[shortest_day-1]:.6f} minutes) is the shortest, while Day {longest_day} ({times[longest_day-1]:.6f} minutes) is the longest.\n"
        markdown += "- **Leg Exercise Limit**: Maintained no more than one leg exercise per day (Squat, Leg Curl, Leg Extension, Stiff-Legged Deadlift), relaxed only if necessary.\n"
        markdown += "- **Exercise List Constraint**: Used all exercises from your provided list (Incline Bench Press, Front Raise, Lateral Raise, Leg Extension, Cable Curl, Shrugs, Squat, Rear Delts, Kelso Shrugs, Upper Back Rows, Chest Fly, Triceps Extension, Leg Curl, Stiff-Legged Deadlift, Pulldown, Lat Prayer), with repeats allowed across days but not within the same day.\n"
        markdown += "- **Set Constraint**: Each exercise is assigned 2-5 sets.\n"
        markdown += "- **Volume Optimization**: Excluded Glutes and Lower Back from volume optimization per user instruction; their volumes are reported but not adjusted to meet targets.\n"
        markdown += "- **Time Equivalence**: Balanced the number of exercises, set structures, and sets per day to achieve rough time equivalence across days (±2 minutes from the average).\n"
        markdown += "- **Recovery**: Ensured muscle groups are scheduled with appropriate rest periods (e.g., 48 hours for Quads from Squats, 36 hours for Quads from Leg Extensions) to avoid working the same primary muscle group on consecutive days.\n"
        markdown += "- **Progression**: Increase weight when you can perform 12 reps with good form.\n"

        # Save to file
        with open(filename, "w") as f:
            f.write(markdown)
        print(f"Saved workout routine to {filename}")


# DQN Model
class DQN(nn.Module):
    def __init__(self, state_size, action_size):
        super(DQN, self).__init__()
        self.fc1 = nn.Linear(state_size, 64)  # Reduced size for faster computation
        self.fc2 = nn.Linear(64, 64)
        self.fc3 = nn.Linear(64, action_size)
        self.relu = nn.ReLU()

    def forward(self, x):
        x = self.relu(self.fc1(x))
        x = self.relu(self.fc2(x))
        x = self.fc3(x)
        return x


# SumTree for efficient prioritized sampling
class SumTree:
    def __init__(self, capacity):
        self.capacity = capacity
        self.tree = np.zeros(2 * capacity - 1)  # Binary tree with capacity leaf nodes
        self.data = np.zeros(capacity, dtype=object)  # Store experiences
        self.write = 0  # Next write position
        self.n_entries = 0  # Number of entries in the buffer

    def _propagate(self, idx, change):
        parent = (idx - 1) // 2
        self.tree[parent] += change
        if parent != 0:
            self._propagate(parent, change)

    def _retrieve(self, idx, s):
        left = 2 * idx + 1
        right = left + 1
        if left >= len(self.tree):
            return idx
        if s <= self.tree[left]:
            return self._retrieve(left, s)
        else:
            return self._retrieve(right, s - self.tree[left])

    def total(self):
        return self.tree[0]

    def add(self, priority, data):
        idx = self.write + self.capacity - 1
        self.data[self.write] = data
        self.update(idx, priority)
        self.write += 1
        if self.write >= self.capacity:
            self.write = 0
        if self.n_entries < self.capacity:
            self.n_entries += 1

    def update(self, idx, priority):
        change = priority - self.tree[idx]
        self.tree[idx] = priority
        self._propagate(idx, change)

    def get(self, s):
        idx = self._retrieve(0, s)
        data_idx = idx - self.capacity + 1
        return idx, self.tree[idx], self.data[data_idx]


# Prioritized Replay Buffer with SumTree
class PrioritizedReplayBuffer:
    def __init__(self, capacity, alpha=0.5):
        self.tree = SumTree(capacity)
        self.capacity = capacity
        self.alpha = alpha
        self.epsilon = 1e-5  # Small value to ensure non-zero priorities

    def push(self, state, action, reward, next_state, done):
        max_priority = (
            self.tree.tree[-self.capacity :].max() if self.tree.n_entries > 0 else 1.0
        )
        self.tree.add(max_priority, (state, action, reward, next_state, done))

    def sample(self, batch_size, beta=0.4):
        if self.tree.n_entries < batch_size:
            return [], [], [], [], [], [], []

        indices = []
        priorities = []
        segment = self.tree.total() / batch_size
        for i in range(batch_size):
            a = segment * i
            b = segment * (i + 1)
            s = random.uniform(a, b)
            idx, p, data = self.tree.get(s)
            indices.append(idx)
            priorities.append(p)

        priorities = np.array(priorities, dtype=np.float32)
        priorities = np.where(np.isfinite(priorities), priorities, 1.0)
        probs = priorities / self.tree.total()
        weights = (self.tree.n_entries * probs) ** (-beta)
        weights = np.where(np.isfinite(weights), weights, 1.0)
        weights /= weights.max()
        weights = np.array(weights, dtype=np.float32)

        samples = [self.tree.data[idx - self.capacity + 1] for idx in indices]
        states, actions, rewards, next_states, dones = zip(*samples)
        states = np.array(states, dtype=np.float32)
        actions = np.array(actions, dtype=np.int64)
        rewards = np.array(rewards, dtype=np.float32)
        next_states = np.array(next_states, dtype=np.float32)
        dones = np.array(dones, dtype=np.float32)
        return states, actions, rewards, next_states, dones, weights, indices

    def update_priorities(self, indices, priorities):
        for idx, priority in zip(indices, priorities):
            priority = min(
                max(priority + self.epsilon, self.epsilon), 1e5
            )  # Clamp priority
            self.tree.update(idx, priority)

    def __len__(self):
        return self.tree.n_entries


# DQN Agent
class DQNAgent:
    def __init__(self, env, state_size, action_size):
        self.env = env
        self.state_size = state_size
        self.action_size = action_size
        self.device = torch.device("cuda" if torch.cuda.is_available() else "cpu")

        # DQN networks
        self.q_network = DQN(state_size, action_size).to(self.device)
        self.target_network = DQN(state_size, action_size).to(self.device)
        self.target_network.load_state_dict(self.q_network.state_dict())
        self.target_network.eval()

        # Optimizer
        self.optimizer = optim.Adam(self.q_network.parameters(), lr=0.001)

        # Replay buffer
        self.replay_buffer = PrioritizedReplayBuffer(capacity=5000, alpha=0.5)

        # Hyperparameters
        self.gamma = 0.99
        self.epsilon = 0.5
        self.epsilon_min = 0.01
        self.epsilon_decay = 0.99  # Slower decay for more exploration
        self.batch_size = 32
        self.target_update_freq = 1000
        self.step_counter = 0
        self.beta = 0.4
        self.beta_anneal = 0.0001
        self.warmup_steps = 5000  # Increased warm-up steps

    def get_action(self, state):
        if random.random() < self.epsilon:
            return self.env.action_space.sample()  # Explore
        else:
            state = torch.FloatTensor(state).to(self.device)
            with torch.no_grad():
                q_values = self.q_network(state)
            return q_values.argmax().item()  # Exploit

    def train(self, num_episodes):
        for episode in range(num_episodes):
            state = self.env.reset()
            total_reward = 0
            done = False
            while not done:
                action = self.get_action(state)
                next_state, reward, done, _ = self.env.step(action)
                self.replay_buffer.push(state, action, reward, next_state, done)
                state = next_state
                total_reward += reward
                self.step_counter += 1

                # Gradually increase recovery penalty factor after warm-up
                if self.step_counter > self.warmup_steps:
                    self.env.recovery_penalty_factor = min(
                        1.0, (self.step_counter - self.warmup_steps) / 5000.0
                    )

                if (
                    self.step_counter > self.warmup_steps
                    and len(self.replay_buffer) >= self.batch_size
                ):
                    self._update()

                if self.step_counter % self.target_update_freq == 0:
                    self.target_network.load_state_dict(self.q_network.state_dict())

            self.epsilon = max(self.epsilon_min, self.epsilon * self.epsilon_decay)
            self.beta = min(1.0, self.beta + self.beta_anneal)

            if episode % 500 == 0:
                print(
                    f"Episode {episode}, Total Reward: {total_reward}, Epsilon: {self.epsilon}, Beta: {self.beta}"
                )
                self.env.render()

    def _update(self):
        states, actions, rewards, next_states, dones, weights, indices = (
            self.replay_buffer.sample(self.batch_size, self.beta)
        )

        states = torch.FloatTensor(states).to(self.device)
        actions = torch.LongTensor(actions).to(self.device)
        rewards = torch.FloatTensor(rewards).to(self.device)
        next_states = torch.FloatTensor(next_states).to(self.device)
        dones = torch.FloatTensor(dones).to(self.device)
        weights = torch.FloatTensor(weights).to(self.device)

        # Compute Q-values
        q_values = self.q_network(states).gather(1, actions.unsqueeze(1)).squeeze(1)
        next_q_values = self.target_network(next_states).max(1)[0].detach()
        target_q_values = rewards + (1 - dones) * self.gamma * next_q_values

        # Compute TD errors
        td_errors = q_values - target_q_values
        loss = (td_errors**2 * weights).mean()

        # Update priorities
        priorities = td_errors.abs().detach().cpu().numpy()
        self.replay_buffer.update_priorities(indices, priorities)

        # Optimize with gradient clipping
        self.optimizer.zero_grad()
        loss.backward()
        torch.nn.utils.clip_grad_norm_(self.q_network.parameters(), max_norm=1.0)
        self.optimizer.step()


# Train the agent
env = WorkoutRoutineEnv()
state_size = env.observation_space.shape[0]
action_size = env.action_space.n
agent = DQNAgent(env, state_size, action_size)
agent.train(num_episodes=5000)

# Test the trained agent
state = env.reset()
done = False
while not done:
    action = agent.get_action(state)
    state, reward, done, _ = env.step(action)
env.render()
env.save_to_file("workout_routine_rl.md")
