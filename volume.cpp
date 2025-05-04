#include "volume.h"

using namespace std;

// Function to calculate volume for a muscle group
unordered_map<string, double> calculate_volume(const unordered_map<int, vector<Structure>>& routine, const unordered_map<string, Exercise>& exercises_map) {
    unordered_map<string, double> volume;
    for (const auto& [day, day_structures] : routine) {
        for (const auto& structure : day_structures) {
            int sets = structure.sets;
            for (const auto& exercise : structure.exercises) {
                const auto& ex = exercises_map.at(exercise);
                for (const auto& muscle : ex.primary) {
                    // Strip the exercise-specific part for volume calculation
                    string base_muscle = muscle.substr(0, muscle.find(" ("));
                    if (base_muscle.empty()) base_muscle = muscle;  // If no " (", use the whole string
                    volume[base_muscle] += sets;
                }
                for (const auto& muscle : ex.secondary) {
                    string base_muscle = muscle.substr(0, muscle.find(" ("));
                    if (base_muscle.empty()) base_muscle = muscle;
                    volume[base_muscle] += sets * 0.25;
                }
                for (const auto& muscle : ex.isometric) {
                    string base_muscle = muscle.substr(0, muscle.find(" ("));
                    if (base_muscle.empty()) base_muscle = muscle;
                    volume[base_muscle] += sets * 0.25;
                }
            }
        }
    }
    return volume;
}

// Function to calculate muscle group coverage for a single day
unordered_map<string, double> calculate_day_coverage(const vector<string>& day_structures, const unordered_map<string, unordered_map<string, double>>& exercise_contributions, int default_sets) {
    unordered_map<string, double> muscle_coverage;
    for (const auto& exercise : day_structures) {
        int sets = default_sets;
        for (const auto& [muscle, contrib] : exercise_contributions.at(exercise)) {
            muscle_coverage[muscle] += contrib * sets;
        }
    }
    return muscle_coverage;
}