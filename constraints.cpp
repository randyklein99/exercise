#include "constraints.h"

using namespace std;

// Function to check leg exercise constraint
bool check_leg_exercise_constraint(const vector<vector<string>>& structures, const unordered_map<string, Exercise>& exercises_map) {
    int leg_exercises = 0;
    for (const auto& structure : structures) {
        for (const auto& exercise : structure) {
            if (exercises_map.at(exercise).is_leg) ++leg_exercises;
        }
    }
    return leg_exercises <= 1;
}

// Function to check if a muscle group can be worked based on recovery time
bool can_work_muscle(const string& muscle, int current_day, const unordered_map<string, int>& last_worked_day) {
    if (last_worked_day.find(muscle) == last_worked_day.end()) {
        return true;  // Muscle has not been worked yet
    }
    int last_day = last_worked_day.at(muscle);
    int required_gap = muscle_recovery_days.at(muscle);  // Use at() for const-safe access
    int days_since_last = current_day - last_day;
    return days_since_last >= required_gap;
}