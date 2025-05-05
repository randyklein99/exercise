#ifndef EXERCISE_DEFINITIONS_H
#define EXERCISE_DEFINITIONS_H

#include <string>
#include <vector>
#include <unordered_map>

using namespace std;

// Struct to represent an exercise
struct Exercise {
    string name;
    vector<string> primary;
    vector<string> secondary;
    vector<string> isometric;
    bool is_compound;
    bool is_leg;
};

// Struct to represent a structure (straight set in this simplified version)
struct Structure {
    vector<string> exercises;
    int sets;
};

// Struct to hold MAV target and upper bound for muscle groups
struct MuscleGroup {
    double target;
    double upper_bound;
};

// Hard-coded mav_targets with target and upper_bound
const unordered_map<string, MuscleGroup> mav_targets = {
    {"Chest", {12.0, 18.0}},
    {"Triceps", {10.0, 16.0}},
    {"Long Head", {10.0, 16.0}},
    {"Front Delts", {8.0, 14.0}},
    {"Lateral Delts", {12.0, 18.0}},
    {"Rear Delts", {12.0, 18.0}},
    {"Lats", {14.0, 20.0}},
    {"Mid Traps", {8.0, 14.0}},
    {"Upper Traps", {8.0, 14.0}},
    {"Lower Traps", {14.0, 20.0}},
    {"Biceps", {14.0, 20.0}},
    {"Quads", {12.0, 18.0}},
    {"Rectus Femoris", {12.0, 18.0}},
    {"Hamstrings", {10.0, 16.0}},
    {"Short Head", {10.0, 16.0}},
    {"Glutes", {12.0, 18.0}},
    {"Lower Back", {12.0, 18.0}}
};

// Recovery times in days for each muscle group (simplified, no exercise-specific suffixes)
const unordered_map<string, int> muscle_recovery_days = {
    {"Chest", 2},
    {"Triceps", 2},
    {"Long Head", 2},
    {"Front Delts", 1},
    {"Lateral Delts", 1},
    {"Rear Delts", 1},
    {"Lats", 2},
    {"Mid Traps", 2},
    {"Upper Traps", 2},
    {"Lower Traps", 2},
    {"Biceps", 2},
    {"Quads", 2},
    {"Rectus Femoris", 2},
    {"Hamstrings", 2},
    {"Short Head", 2},
    {"Glutes", 2},
    {"Lower Back", 2}
};

// Define exercises with base muscle names
const vector<Exercise> exercises = {
    {"Incline Bench Press", {"Chest", "Triceps", "Front Delts"}, {}, {"Lower Back"}, true, false},
    {"Front Raise", {"Front Delts"}, {}, {"Biceps"}, false, false},
    {"Lateral Raise", {"Lateral Delts"}, {}, {}, false, false},
    {"Leg Extension", {"Quads", "Rectus Femoris"}, {}, {}, false, true},
    {"Cable Curl", {"Biceps"}, {}, {}, false, false},
    {"Pulldown", {"Lats", "Biceps"}, {"Lower Traps", "Rear Delts"}, {}, true, false},
    {"Squat", {"Quads", "Glutes"}, {"Hamstrings", "Lower Back"}, {"Lower Back"}, true, true},
    {"Rear Delts", {"Rear Delts"}, {}, {}, false, false},
    {"Shrugs", {"Upper Traps"}, {}, {}, false, false},
    {"Kelso Shrugs", {"Mid Traps"}, {}, {}, false, false},
    {"Chest Fly", {"Chest"}, {"Front Delts"}, {}, false, false},
    {"Upper Back Rows", {"Mid Traps", "Rear Delts"}, {"Lats", "Biceps"}, {"Lower Back"}, true, false},
    {"Triceps Extension", {"Triceps", "Long Head"}, {}, {}, false, false},
    {"Leg Curl", {"Hamstrings", "Short Head"}, {}, {}, false, true},
    {"Stiff-Legged Deadlift", {"Hamstrings", "Glutes"}, {"Lower Back"}, {"Lower Back", "Upper Traps"}, true, true},
    {"Lat Prayer", {"Lats", "Long Head"}, {}, {}, false, false}
};

// Constants required by routine_optimizer.cpp
const int TOTAL_DAYS = 6;
const int MIN_EXERCISES_PER_DAY = 3;
const int MAX_EXERCISES_PER_DAY = 6;
const int MIN_SETS = 2;
const int MAX_SETS = 5;
const double TIME_PER_SET = 2.0;
const double MAX_TIME_PER_DAY = 50.0;
const double TARGET_AVG_TIME = 35.0;

#endif // EXERCISE_DEFINITIONS_H