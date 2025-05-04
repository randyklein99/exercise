#include <iostream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <random>
#include <fstream>
#include <algorithm>
#include "exercise_definitions.h"
#include "utils.h"
#include "constraints.h"
#include "volume.h"
#include "assign.h"
#include "format.h"

using namespace std;

int main() {
    // Create a map for quick lookup
    unordered_map<string, Exercise> exercises_map;
    for (const auto& ex : exercises) {
        exercises_map[ex.name] = ex;
    }

    // Initialize days and usage tracking
    unordered_map<int, vector<string>> days;
    for (int i = 1; i <= 6; ++i) days[i] = vector<string>();
    unordered_map<string, int> exercise_usage;
    for (const auto& ex : exercises) {
        exercise_usage[ex.name] = 0;
    }
    unordered_map<string, double> muscle_coverage;
    unordered_map<string, int> last_worked_day;  // Track the last day each muscle group was worked

    // Precompute exercise contributions
    unordered_map<string, unordered_map<string, double>> exercise_contributions;
    for (size_t i = 0; i < exercises.size(); ++i) {
        unordered_map<string, double> contrib;
        const auto& ex = exercises[i];
        for (const auto& muscle : ex.primary) contrib[muscle] = 1.0;
        for (const auto& muscle : ex.secondary) contrib[muscle] = 0.25;
        for (const auto& muscle : ex.isometric) contrib[muscle] = 0.25;
        exercise_contributions[ex.name] = contrib;
    }

    // Target number of exercises per day
    const int target_exercises_per_day = 4;
    const int max_exercises_per_day = 6;  // Constraint: 3-6 exercises per day
    const int total_days = 6;
    const double max_time_per_day = 50.0;  // Max time per day

    // Calculate target muscle group coverage (excluding Glutes and Lower Back)
    unordered_map<string, double> target_coverage;
    unordered_map<string, double> target_upper_bounds;
    for (const auto& [muscle, target] : mav_targets) {
        if (muscle == "Glutes" || muscle == "Lower Back") continue;
        target_coverage[muscle] = target / static_cast<double>(total_days);
        target_upper_bounds[muscle] = target + 6;  // Upper bound for volume
    }
    cout << "Target muscle group coverage per day (excluding Glutes and Lower Back):\n";
    for (const auto& [muscle, target] : target_coverage) {
        cout << "  " << muscle << ": " << target << " sets\n";
    }

    // Assign exercises
    unordered_map<int, double> day_times;
    unordered_map<int, vector<Structure>> routine;
    random_device rd;
    mt19937 g(rd());
    assign_exercises(
        days,
        exercise_usage,
        muscle_coverage,
        last_worked_day,
        exercise_contributions,
        exercises_map,
        day_times,
        target_coverage,
        exercises,
        target_exercises_per_day,
        total_days,
        g,
        routine
    );

    // Simplified set structure generation with dynamic set adjustment
    cout << "Generating initial set structures for time estimation...\n";
    for (int day = 1; day <= total_days; ++day) {
        const auto& day_exercises = days[day];
        vector<vector<string>> structures;
        for (const auto& exercise : day_exercises) {
            structures.push_back({exercise});
        }
        vector<int> set_combo(structures.size(), 3);  // Start with 3 sets
        vector<Structure>& routine_structures = routine[day];
        for (size_t i = 0; i < structures.size(); ++i) {
            routine_structures.push_back({structures[i], set_combo[i]});
        }
        day_times[day] = 0;  // Reset day time
        for (const auto& structure : routine_structures) {
            day_times[day] += calculate_time(structure.exercises, structure.sets);
        }
    }

    // Optimize volumes, prioritizing volume above all else
    cout << "Optimizing volumes...\n";
    for (int iteration = 0; iteration < 20; ++iteration) {  // Increased iterations to 20
        cout << "Iteration " << iteration + 1 << " of volume optimization...\n";
        // Calculate current volume
        unordered_map<string, double> volume = calculate_volume(routine, exercises_map);
        // Sort muscle groups by volume deficit to prioritize the largest deficits
        vector<pair<string, double>> volume_deficits;
        for (const auto& [muscle_group, target] : mav_targets) {
            if (muscle_group == "Glutes" || muscle_group == "Lower Back") continue;
            double current_volume = volume[muscle_group];
            double deficit = target - current_volume;
            if (deficit > 0) {
                volume_deficits.emplace_back(muscle_group, deficit);
            }
        }
        sort(volume_deficits.begin(), volume_deficits.end(), 
             [](const auto& a, const auto& b) { return a.second > b.second; });

        // Reduce sets for over-target muscle groups only if significantly over target
        for (const auto& [muscle_group, target] : mav_targets) {
            if (muscle_group == "Glutes" || muscle_group == "Lower Back") continue;
            double current_volume = volume[muscle_group];
            double upper_bound = target_upper_bounds[muscle_group];
            if (current_volume <= upper_bound) continue;  // Only reduce if significantly over target
            int inner_iteration = 0;
            const int max_inner_iterations = 10;
            while (current_volume > target && inner_iteration < max_inner_iterations) {  // Reduce to target, not upper bound
                cout << "  Reducing volume for " << muscle_group << " (current: " << current_volume << ", target: " << target << ")\n";
                vector<string> contributing_exercises;
                for (const auto& ex : exercises) {
                    bool contributes = false;
                    for (const auto& muscle : ex.primary) {
                        string base_muscle = muscle.substr(0, muscle.find(" ("));
                        if (base_muscle == muscle_group) { contributes = true; break; }
                    }
                    for (const auto& muscle : ex.secondary) {
                        string base_muscle = muscle.substr(0, muscle.find(" ("));
                        if (base_muscle == muscle_group) { contributes = true; break; }
                    }
                    for (const auto& muscle : ex.isometric) {
                        string base_muscle = muscle.substr(0, muscle.find(" ("));
                        if (base_muscle == muscle_group) { contributes = true; break; }
                    }
                    if (contributes) contributing_exercises.push_back(ex.name);
                }
                if (contributing_exercises.empty()) break;
                bool reduced = false;
                vector<int> days_list(total_days);
                for (int d = 1; d <= total_days; ++d) days_list[d-1] = d;
                shuffle(days_list.begin(), days_list.end(), g);
                for (int day : days_list) {
                    for (auto& structure : routine[day]) {
                        if (structure.sets <= 2) continue;
                        if (find(contributing_exercises.begin(), contributing_exercises.end(), structure.exercises[0]) == contributing_exercises.end()) continue;
                        --structure.sets;
                        double contribution = 0;
                        for (const auto& muscle : exercises_map.at(structure.exercises[0]).primary) {
                            string base_muscle = muscle.substr(0, muscle.find(" ("));
                            if (base_muscle == muscle_group) contribution += 1.0;
                        }
                        for (const auto& muscle : exercises_map.at(structure.exercises[0]).secondary) {
                            string base_muscle = muscle.substr(0, muscle.find(" ("));
                            if (base_muscle == muscle_group) contribution += 0.25;
                        }
                        for (const auto& muscle : exercises_map.at(structure.exercises[0]).isometric) {
                            string base_muscle = muscle.substr(0, muscle.find(" ("));
                            if (base_muscle == muscle_group) contribution += 0.25;
                        }
                        current_volume -= contribution;
                        day_times[day] -= calculate_time(structure.exercises, 1);
                        cout << "  Removed 1 set from [\"" << structure.exercises[0] << "\"] on Day " << day
                             << " (contribution: " << contribution << "). New volume: " << current_volume << "\n";
                        reduced = true;
                        volume = calculate_volume(routine, exercises_map);
                        break;
                    }
                    if (reduced) break;
                }
                if (!reduced) break;
                inner_iteration++;
            }
            if (inner_iteration >= max_inner_iterations) {
                cout << "  Warning: Max inner iterations reached while reducing volume for " << muscle_group << "\n";
                break;
            }
        }

        // Add sets for under-target muscle groups, maximizing sets up to 5
        volume = calculate_volume(routine, exercises_map);
        volume_deficits.clear();
        for (const auto& [muscle_group, target] : mav_targets) {
            if (muscle_group == "Glutes" || muscle_group == "Lower Back") continue;
            double current_volume = volume[muscle_group];
            double deficit = target - current_volume;
            if (deficit > 0) {
                volume_deficits.emplace_back(muscle_group, deficit);
            }
        }
        sort(volume_deficits.begin(), volume_deficits.end(), 
             [](const auto& a, const auto& b) { return a.second > b.second; });

        for (const auto& [muscle_group, deficit] : volume_deficits) {
            double current_volume = volume[muscle_group];
            double target = mav_targets.at(muscle_group);
            int inner_iteration = 0;
            const int max_inner_iterations = 10;
            bool progress_made = false;
            while (current_volume < target && inner_iteration < max_inner_iterations) {
                cout << "  Increasing volume for " << muscle_group << " (current: " << current_volume << ", target: " << target << ")\n";
                vector<string> contributing_exercises;
                for (const auto& ex : exercises) {
                    bool contributes = false;
                    for (const auto& muscle : ex.primary) {
                        string base_muscle = muscle.substr(0, muscle.find(" ("));
                        if (base_muscle == muscle_group) { contributes = true; break; }
                    }
                    for (const auto& muscle : ex.secondary) {
                        string base_muscle = muscle.substr(0, muscle.find(" ("));
                        if (base_muscle == muscle_group) { contributes = true; break; }
                    }
                    for (const auto& muscle : ex.isometric) {
                        string base_muscle = muscle.substr(0, muscle.find(" ("));
                        if (base_muscle == muscle_group) { contributes = true; break; }
                    }
                    if (contributes) contributing_exercises.push_back(ex.name);
                }
                if (contributing_exercises.empty()) {
                    cout << "  No contributing exercises available for " << muscle_group << "\n";
                    break;
                }
                bool assigned = false;
                vector<int> days_list(total_days);
                for (int d = 1; d <= total_days; ++d) days_list[d-1] = d;
                shuffle(days_list.begin(), days_list.end(), g);
                for (int day : days_list) {
                    double current_day_time = day_times[day];
                    if (current_day_time >= max_time_per_day) continue;

                    for (auto& structure : routine[day]) {
                        if (find(contributing_exercises.begin(), contributing_exercises.end(), structure.exercises[0]) == contributing_exercises.end()) continue;
                        // Check recovery before adding sets
                        unordered_map<string, double> prev_day_coverage = day > 1 ? calculate_day_coverage(days[day-1], exercise_contributions, structure.sets) : unordered_map<string, double>();
                        unordered_set<string> primary_muscles(exercises_map.at(structure.exercises[0]).primary.begin(), exercises_map.at(structure.exercises[0]).primary.end());
                        double recovery_penalty = 0;
                        for (const auto& muscle : primary_muscles) {
                            if (prev_day_coverage[muscle] > 0) {
                                recovery_penalty += prev_day_coverage[muscle] * 25;
                            }
                        }
                        if (recovery_penalty > 0) {
                            cout << "    Skipped adding set to [\"" << structure.exercises[0] << "\"] on Day " << day << " due to recovery penalty: " << recovery_penalty << "\n";
                            continue;
                        }
                        // Maximize sets up to 5
                        int sets_to_add = min(5 - structure.sets, static_cast<int>((max_time_per_day - current_day_time) / 2.0));
                        if (sets_to_add <= 0) {
                            cout << "    Skipped adding sets to [\"" << structure.exercises[0] << "\"] on Day " << day << " as it would exceed 50 minutes (current: " << current_day_time << ")\n";
                            continue;
                        }
                        structure.sets += sets_to_add;
                        double contribution = 0;
                        for (const auto& muscle : exercises_map.at(structure.exercises[0]).primary) {
                            string base_muscle = muscle.substr(0, muscle.find(" ("));
                            if (base_muscle == muscle_group) contribution += 1.0;
                        }
                        for (const auto& muscle : exercises_map.at(structure.exercises[0]).secondary) {
                            string base_muscle = muscle.substr(0, muscle.find(" ("));
                            if (base_muscle == muscle_group) contribution += 0.25;
                        }
                        for (const auto& muscle : exercises_map.at(structure.exercises[0]).isometric) {
                            string base_muscle = muscle.substr(0, muscle.find(" ("));
                            if (base_muscle == muscle_group) contribution += 0.25;
                        }
                        current_volume += contribution * sets_to_add;
                        day_times[day] += calculate_time(structure.exercises, sets_to_add);
                        cout << "  Added " << sets_to_add << " sets to [\"" << structure.exercises[0] << "\"] on Day " << day
                             << " (contribution per set: " << contribution << "). New volume: " << current_volume << "\n";
                        assigned = true;
                        progress_made = true;
                        volume = calculate_volume(routine, exercises_map);
                        break;
                    }
                    if (assigned) break;
                }
                if (!assigned) {
                    cout << "  Could not assign more sets for " << muscle_group << " due to constraints\n";
                    break;
                }
                inner_iteration++;
            }
            if (inner_iteration >= max_inner_iterations) {
                cout << "  Warning: Max inner iterations reached while increasing volume for " << muscle_group << "\n";
                break;
            }
            if (!progress_made) {
                cout << "  No progress made for " << muscle_group << ", moving to next muscle group\n";
                break;
            }
        }
        // Ensure no day exceeds 50 minutes (only reduction if necessary, no balancing)
        for (int day = 1; day <= total_days; ++day) {
            int inner_iteration = 0;
            const int max_inner_iterations = 10;
            while (day_times[day] > max_time_per_day && inner_iteration < max_inner_iterations) {
                cout << "  Reducing time for Day " << day << " (current: " << day_times[day] << ", max: " << max_time_per_day << ")\n";
                for (auto& structure : routine[day]) {
                    if (structure.sets <= 2) continue;
                    --structure.sets;
                    day_times[day] -= calculate_time(structure.exercises, 1);
                    cout << "    Reduced 1 set from [\"" << structure.exercises[0] << "\"] on Day " << day << "\n";
                    break;
                }
                inner_iteration++;
            }
            if (inner_iteration >= max_inner_iterations) {
                cout << "  Warning: Max inner iterations reached while reducing time for Day " << day << "\n";
            }
        }
    }

    // Final pass: Add exercises to days to maximize volume for remaining deficits
    cout << "Final pass: Adding exercises to maximize volume...\n";
    auto current_volume = calculate_volume(routine, exercises_map);
    vector<pair<string, double>> final_deficits;
    for (const auto& [muscle_group, target] : mav_targets) {
        if (muscle_group == "Glutes" || muscle_group == "Lower Back") continue;
        double deficit = target - current_volume[muscle_group];
        if (deficit > 0) {
            final_deficits.emplace_back(muscle_group, deficit);
        }
    }
    sort(final_deficits.begin(), final_deficits.end(), 
         [](const auto& a, const auto& b) { return a.second > b.second; });

    for (const auto& [muscle_group, deficit] : final_deficits) {
        if (deficit <= 0) continue;
        cout << "  Final pass for " << muscle_group << " (deficit: " << deficit << ")\n";
        vector<string> contributing_exercises;
        for (const auto& ex : exercises) {
            bool contributes = false;
            for (const auto& muscle : ex.primary) {
                string base_muscle = muscle.substr(0, muscle.find(" ("));
                if (base_muscle == muscle_group) { contributes = true; break; }
            }
            for (const auto& muscle : ex.secondary) {
                string base_muscle = muscle.substr(0, muscle.find(" ("));
                if (base_muscle == muscle_group) { contributes = true; break; }
            }
            for (const auto& muscle : ex.isometric) {
                string base_muscle = muscle.substr(0, muscle.find(" ("));
                if (base_muscle == muscle_group) { contributes = true; break; }
            }
            if (contributes) contributing_exercises.push_back(ex.name);
        }
        if (contributing_exercises.empty()) continue;

        vector<int> days_list(total_days);
        for (int d = 1; d <= total_days; ++d) days_list[d-1] = d;
        shuffle(days_list.begin(), days_list.end(), g);

        for (int day : days_list) {
            double current_day_time = day_times[day];
            if (current_day_time >= max_time_per_day) continue;
            if (routine[day].size() >= max_exercises_per_day) continue;

            // Check if adding a new exercise is feasible
            bool can_add_exercise = true;
            vector<string> current_exercises;
            for (const auto& structure : routine[day]) {
                current_exercises.push_back(structure.exercises[0]);
            }
            int current_leg_exercises = 0;
            for (const auto& ex : current_exercises) {
                if (exercises_map.at(ex).is_leg) ++current_leg_exercises;
            }

            string best_exercise;
            double best_volume_score = numeric_limits<double>::lowest();
            for (const auto& ex : contributing_exercises) {
                // Skip if exercise is already in the day
                if (find(current_exercises.begin(), current_exercises.end(), ex) != current_exercises.end()) continue;

                // Check leg exercise constraint
                bool is_leg = exercises_map.at(ex).is_leg;
                if (is_leg && current_leg_exercises >= 1) continue;  // Only one leg exercise per day

                // Check recovery
                bool can_use = true;
                unordered_set<string> exercise_muscles;
                unordered_set<string> primary_muscles;
                for (const auto& muscle : exercises_map.at(ex).primary) {
                    exercise_muscles.insert(muscle);
                    primary_muscles.insert(muscle);
                    if (!can_work_muscle(muscle, day, last_worked_day)) {
                        can_use = false;
                        break;
                    }
                }
                for (const auto& muscle : exercises_map.at(ex).secondary) {
                    exercise_muscles.insert(muscle);
                    if (!can_work_muscle(muscle, day, last_worked_day)) {
                        can_use = false;
                        break;
                    }
                }
                for (const auto& muscle : exercises_map.at(ex).isometric) {
                    exercise_muscles.insert(muscle);
                    if (!can_work_muscle(muscle, day, last_worked_day)) {
                        can_use = false;
                        break;
                    }
                }
                if (!can_use) continue;

                // Calculate volume contribution
                double volume_score = 0;
                for (const auto& [muscle, target] : target_coverage) {
                    double current = current_volume[muscle];
                    double diff = target - current;
                    if (diff > 0) {
                        bool contributes = false;
                        double contribution = 0;
                        for (const auto& m : exercises_map.at(ex).primary) {
                            string base_muscle = m.substr(0, m.find(" ("));
                            if (base_muscle == muscle) {
                                contributes = true;
                                contribution += 1.0;
                                break;
                            }
                        }
                        for (const auto& m : exercises_map.at(ex).secondary) {
                            string base_muscle = m.substr(0, m.find(" ("));
                            if (base_muscle == muscle) {
                                contributes = true;
                                contribution += 0.25;
                                break;
                            }
                        }
                        for (const auto& m : exercises_map.at(ex).isometric) {
                            string base_muscle = m.substr(0, m.find(" ("));
                            if (base_muscle == muscle) {
                                contributes = true;
                                contribution += 0.25;
                                break;
                            }
                        }
                        if (contributes) {
                            volume_score -= diff * contribution * 200;
                        }
                    }
                }
                if (volume_score > best_volume_score) {
                    best_volume_score = volume_score;
                    best_exercise = ex;
                }
            }

            if (best_exercise.empty()) continue;

            // Add the exercise with maximum sets possible
            int sets_to_add = min(5, static_cast<int>((max_time_per_day - current_day_time) / 2.0));
            if (sets_to_add <= 0) continue;

            routine[day].push_back({{best_exercise}, sets_to_add});
            exercise_usage[best_exercise]++;
            for (const auto& [muscle, contrib] : exercise_contributions.at(best_exercise)) {
                muscle_coverage[muscle] += contrib * sets_to_add;
                if (muscle_recovery_days.find(muscle) != muscle_recovery_days.end()) {
                    last_worked_day[muscle] = day;
                }
            }
            day_times[day] += calculate_time({best_exercise}, sets_to_add);
            cout << "  Added " << best_exercise << " with " << sets_to_add << " sets to Day " << day << "\n";

            // Recalculate volume for the next muscle group
            current_volume = calculate_volume(routine, exercises_map);
        }
    }
    cout << "Final volume optimization pass completed.\n";

    cout << "Routine generation completed.\n";

    // Format output
    string markdown = format_routine(routine, exercises_map, day_times, total_days);

    // Save to file
    cout << "Saving output to workout_routine.md...\n";
    ofstream out_file("workout_routine.md");
    out_file << markdown;
    out_file.close();

    cout << "Workout routine generation complete!\n";
    cout << markdown << "\n";

    return 0;
}