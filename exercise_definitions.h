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

// Hard-coded mav_targets
const unordered_map<string, double> mav_targets = {
    {"Chest", 12.0}, {"Triceps", 10.0}, {"Long Head", 10.0}, {"Front Delts", 8.0},
    {"Lateral Delts", 12.0}, {"Rear Delts", 12.0}, {"Lats", 14.0}, {"Mid Traps", 8.0},
    {"Upper Traps", 8.0}, {"Lower Traps", 14.0}, {"Biceps", 14.0}, {"Quads", 12.0},
    {"Rectus Femoris", 12.0}, {"Hamstrings", 10.0}, {"Short Head", 10.0},
    {"Glutes", 12.0}, {"Lower Back", 12.0}
};

// Recovery times in days for each muscle group
const unordered_map<string, int> muscle_recovery_days = {
    {"Chest (Incline Bench Press)", 2},         // 48 hours
    {"Chest (Chest Fly)", 2},                   // 36 hours, conservative 2 days
    {"Front Delts (Incline Bench Press)", 1},   // 24 hours
    {"Front Delts (Chest Fly)", 1},             // 24 hours
    {"Front Delts (Front Raise)", 1},           // 24 hours
    {"Lats (Pulldown)", 2},                    // 48 hours
    {"Lats (Upper Back Rows)", 1},             // 24 hours (secondary)
    {"Lats (Lat Prayer)", 2},                   // 36 hours, conservative 2 days
    {"Mid Traps (Upper Back Rows)", 2},         // 48 hours
    {"Mid Traps (Kelso Shrugs)", 2},            // 36 hours, conservative 2 days
    {"Lower Traps (Pulldown)", 1},              // 24 hours (secondary)
    {"Lower Traps (Upper Back Rows)", 2},       // 48 hours
    {"Upper Traps (Shrugs)", 2},                // 36 hours, conservative 2 days
    {"Upper Traps (Stiff-Legged Deadlift)", 1}, // 24 hours (isometric)
    {"Rear Delts (Upper Back Rows)", 2},        // 24 hours (primary in this context)
    {"Rear Delts (Pulldown)", 1},               // 24 hours (secondary)
    {"Rear Delts (Rear Delts)", 1},             // 24 hours
    {"Lateral Delts (Lateral Raise)", 1},       // 24 hours
    {"Triceps (Incline Bench Press)", 2},       // 24 hours (primary in this context)
    {"Triceps (Triceps Extension)", 1},         // 24 hours
    {"Long Head (Triceps Extension)", 1},       // 24 hours (primary in this context)
    {"Long Head (Lat Prayer)", 2},              // 36 hours, conservative 2 days
    {"Biceps (Pulldown)", 2},                   // 24 hours (primary in this context)
    {"Biceps (Upper Back Rows)", 1},            // 24 hours (secondary)
    {"Biceps (Cable Curl)", 1},                 // 24 hours
    {"Biceps (Front Raise)", 1},                // 24 hours (isometric)
    {"Quads (Squat)", 3},                       // 48 hours
    {"Quads (Leg Extension)", 2},               // 36 hours, conservative 2 days
    {"Rectus Femoris (Leg Extension)", 2},      // 36 hours, conservative 2 days
    {"Hamstrings (Squat)", 2},                  // 48 hours (secondary)
    {"Hamstrings (Stiff-Legged Deadlift)", 3},  // 48 hours
    {"Hamstrings (Leg Curl)", 2},               // 36 hours, conservative 2 days
    {"Short Head (Leg Curl)", 2}                // 36 hours, conservative 2 days
};

// Define exercises based on the muscle activation table (Bench Press removed)
const vector<Exercise> exercises = {
    {"Incline Bench Press", {"Chest (Incline Bench Press)", "Triceps (Incline Bench Press)", "Front Delts (Incline Bench Press)"}, {}, {"Lower Back"}, true, false},
    {"Front Raise", {"Front Delts (Front Raise)"}, {}, {"Biceps (Front Raise)"}, false, false},
    {"Lateral Raise", {"Lateral Delts (Lateral Raise)"}, {}, {}, false, false},
    {"Leg Extension", {"Quads (Leg Extension)", "Rectus Femoris (Leg Extension)"}, {}, {}, false, true},
    {"Cable Curl", {"Biceps (Cable Curl)"}, {}, {}, false, false},
    {"Pulldown", {"Lats (Pulldown)", "Biceps (Pulldown)"}, {"Lower Traps (Pulldown)", "Rear Delts (Pulldown)"}, {}, true, false},
    {"Squat", {"Quads (Squat)", "Glutes"}, {"Hamstrings (Squat)", "Lower Back"}, {"Lower Back"}, true, true},
    {"Rear Delts", {"Rear Delts (Rear Delts)"}, {}, {}, false, false},
    {"Shrugs", {"Upper Traps (Shrugs)"}, {}, {}, false, false},
    {"Kelso Shrugs", {"Mid Traps (Kelso Shrugs)"}, {}, {}, false, false},
    {"Chest Fly", {"Chest (Chest Fly)"}, {"Front Delts (Chest Fly)"}, {}, false, false},
    {"Upper Back Rows", {"Mid Traps (Upper Back Rows)", "Rear Delts (Upper Back Rows)"}, {"Lats (Upper Back Rows)", "Biceps (Upper Back Rows)"}, {"Lower Back"}, true, false},
    {"Triceps Extension", {"Triceps (Triceps Extension)", "Long Head (Triceps Extension)"}, {}, {}, false, false},
    {"Leg Curl", {"Hamstrings (Leg Curl)", "Short Head (Leg Curl)"}, {}, {}, false, true},
    {"Stiff-Legged Deadlift", {"Hamstrings (Stiff-Legged Deadlift)", "Glutes"}, {"Lower Back"}, {"Lower Back", "Upper Traps (Stiff-Legged Deadlift)"}, true, true},
    {"Lat Prayer", {"Lats (Lat Prayer)", "Long Head (Lat Prayer)"}, {}, {}, false, false}
};

#endif // EXERCISE_DEFINITIONS_H