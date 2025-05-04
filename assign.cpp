#include "assign.h"
#include "constraints.h"
#include "volume.h"
#include "utils.h"
#include <iostream>
#include <algorithm>
#include <unordered_set>

using namespace std;

void assign_exercises(
    unordered_map<int, vector<string>>& days,
    unordered_map<string, int>& exercise_usage,
    unordered_map<string, double>& muscle_coverage,
    unordered_map<string, int>& last_worked_day,
    const unordered_map<string, unordered_map<string, double>>& exercise_contributions,
    const unordered_map<string, Exercise>& exercises_map,
    unordered_map<int, double>& day_times,
    const unordered_map<string, double>& target_coverage,
    const vector<Exercise>& exercises,
    int target_exercises_per_day,
    int total_days,
    mt19937& g,
    unordered_map<int, vector<Structure>>& routine
) {
    const int total_exercise_slots = target_exercises_per_day * total_days;

    // Track days each muscle group is worked (for logging purposes only)
    unordered_map<string, int> muscle_days_worked;
    for (const auto& [muscle, _] : target_coverage) {
        muscle_days_worked[muscle] = 0;
    }

    // Assign compound exercises
    cout << "Assigning compound exercises...\n";
    vector<string> available_compounds;
    for (const auto& ex : exercises) {
        if (ex.is_compound) available_compounds.push_back(ex.name);
    }
    shuffle(available_compounds.begin(), available_compounds.end(), g);

    // Track usage of Squats and Stiff-Legged Deadlifts to ensure they are assigned only once
    bool squat_assigned = false;
    bool stiff_deadlift_assigned = false;

    for (int day = 1; day <= total_days; ++day) {
        vector<string>& current_exercises = days[day];

        // Filter available compounds based on usage constraints
        vector<string> eligible_compounds;
        for (const auto& ex : available_compounds) {
            if (ex == "Squat" && squat_assigned) continue;
            if (ex == "Stiff-Legged Deadlift" && stiff_deadlift_assigned) continue;
            if (find(current_exercises.begin(), current_exercises.end(), ex) == current_exercises.end()) {
                eligible_compounds.push_back(ex);
            }
        }

        string exercise;
        double best_score = numeric_limits<double>::max();
        string best_exercise;
        if (!eligible_compounds.empty()) {
            unordered_map<string, double> prev_day_coverage = day > 1 ? calculate_day_coverage(days[day-1], exercise_contributions) : unordered_map<string, double>();
            auto current_volume = calculate_volume(routine, exercises_map);
            for (const auto& ex : eligible_compounds) {
                // Check recovery for all affected muscle groups
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

                unordered_map<string, double> temp_coverage = muscle_coverage;
                for (const auto& [muscle, contrib] : exercise_contributions.at(ex)) {
                    temp_coverage[muscle] += contrib * 3;
                }
                double recovery_penalty = 0;
                for (const auto& muscle : exercise_muscles) {
                    double prev_workload = prev_day_coverage[muscle];
                    double penalty = prev_workload * (primary_muscles.count(muscle) ? 1.0 : 0.25);
                    if (primary_muscles.count(muscle)) {
                        recovery_penalty += penalty * 25;
                    } else {
                        recovery_penalty += penalty * 5;
                    }
                }
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
                            volume_score -= diff * contribution * 200;  // Heavily prioritize volume
                        }
                    }
                }
                double usage_score = exercise_usage[ex] * 10;  // Further reduced penalty
                double score = volume_score + recovery_penalty + usage_score;
                if (score < best_score) {
                    best_score = score;
                    best_exercise = ex;
                }
            }
            if (!best_exercise.empty()) {
                exercise = best_exercise;
            } else {
                // Fallback: relax constraints and pick any eligible compound
                for (const auto& ex : eligible_compounds) {
                    if (find(current_exercises.begin(), current_exercises.end(), ex) == current_exercises.end()) {
                        exercise = ex;
                        break;
                    }
                }
            }
        } else {
            unordered_map<string, double> prev_day_coverage = day > 1 ? calculate_day_coverage(days[day-1], exercise_contributions) : unordered_map<string, double>();
            auto current_volume = calculate_volume(routine, exercises_map);
            for (const auto& ex : available_compounds) {
                if (ex == "Squat" && squat_assigned) continue;
                if (ex == "Stiff-Legged Deadlift" && stiff_deadlift_assigned) continue;
                if (find(current_exercises.begin(), current_exercises.end(), ex) != current_exercises.end()) continue;
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

                unordered_map<string, double> temp_coverage = muscle_coverage;
                for (const auto& [muscle, contrib] : exercise_contributions.at(ex)) {
                    temp_coverage[muscle] += contrib * 3;
                }
                double recovery_penalty = 0;
                for (const auto& muscle : exercise_muscles) {
                    double prev_workload = prev_day_coverage[muscle];
                    double penalty = prev_workload * (primary_muscles.count(muscle) ? 1.0 : 0.25);
                    if (primary_muscles.count(muscle)) {
                        recovery_penalty += penalty * 25;
                    } else {
                        recovery_penalty += penalty * 5;
                    }
                }
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
                            volume_score -= diff * contribution * 200;  // Heavily prioritize volume
                        }
                    }
                }
                double usage_score = exercise_usage[ex] * 10;  // Further reduced penalty
                double score = volume_score + recovery_penalty + usage_score;
                if (score < best_score) {
                    best_score = score;
                    best_exercise = ex;
                }
            }
            if (!best_exercise.empty()) {
                exercise = best_exercise;
            } else {
                // Last resort: pick any eligible compound, ignoring recovery
                for (const auto& ex : available_compounds) {
                    if (ex == "Squat" && squat_assigned) continue;
                    if (ex == "Stiff-Legged Deadlift" && stiff_deadlift_assigned) continue;
                    if (find(current_exercises.begin(), current_exercises.end(), ex) == current_exercises.end()) {
                        exercise = ex;
                        break;
                    }
                }
            }
        }
        // Ensure an exercise was selected
        if (exercise.empty()) {
            cout << "  No eligible compound exercises available for Day " << day << "\n";
            continue;
        }
        days[day].push_back(exercise);
        ++exercise_usage[exercise];
        if (exercise == "Squat") squat_assigned = true;
        if (exercise == "Stiff-Legged Deadlift") stiff_deadlift_assigned = true;
        for (const auto& [muscle, contrib] : exercise_contributions.at(exercise)) {
            muscle_coverage[muscle] += contrib * 3;
            if (muscle_recovery_days.find(muscle) != muscle_recovery_days.end()) {
                last_worked_day[muscle] = day;
                string base_muscle = muscle.substr(0, muscle.find(" ("));
                muscle_days_worked[base_muscle]++;
            }
        }
        cout << "  Assigned compound exercise " << exercise << " to Day " << day << " (usage: " << exercise_usage[exercise] << ")\n";
    }
    cout << "Compound exercise assignment completed.\n";

    // Assign remaining exercises
    int remaining_slots = total_exercise_slots - days.size();
    cout << "Remaining slots to fill: " << remaining_slots << " (total slots: " << total_exercise_slots << ")\n";
    vector<string> available_exercises;
    for (const auto& ex : exercises) {
        if (!ex.is_compound) available_exercises.push_back(ex.name);
    }
    for (int day = 1; day <= total_days; ++day) {
        day_times[day] = 0.0;
        for (const auto& exercise : days[day]) {
            day_times[day] += calculate_time({exercise}, 3);
        }
    }

    int day_index = 1;
    int slots_filled = 0;
    vector<tuple<int, vector<string>, int>> deferred_slots;
    while (slots_filled < remaining_slots) {
        int day = day_index;
        vector<string>& current_exercises = days[day];
        int current_leg_exercises = 0;
        for (const auto& ex : current_exercises) {
            if (exercises_map.at(ex).is_leg) ++current_leg_exercises;
        }
        cout << "\nAttempting to fill a slot on Day " << day << ": " << current_exercises.size()
             << " exercises (leg exercises: " << current_leg_exercises << ", current time: "
             << day_times[day] << " minutes), target: " << target_exercises_per_day << "\n";
        if (current_exercises.size() >= target_exercises_per_day + 1) {
            cout << "  Day " << day << " already has " << current_exercises.size() << " exercises, skipping.\n";
            day_index = (day_index % total_days) + 1;
            continue;
        }

        vector<string> available_for_day;
        for (const auto& ex : available_exercises) {
            if (find(current_exercises.begin(), current_exercises.end(), ex) == current_exercises.end()) {
                available_for_day.push_back(ex);
            }
        }
        if (available_for_day.empty()) {
            cout << "  No available exercises for Day " << day << " (all exercises already used in this day). Deferring slot.\n";
            deferred_slots.emplace_back(day, current_exercises, current_leg_exercises);
            day_index = (day_index % total_days) + 1;
            continue;
        }

        bool all_used = true;
        for (const auto& ex : exercises) {
            if (exercise_usage[ex.name] == 0) {
                all_used = false;
                break;
            }
        }
        vector<string> candidates = all_used ? available_for_day : vector<string>();
        if (!all_used) {
            for (const auto& ex : available_for_day) {
                if (exercise_usage[ex] == 0) candidates.push_back(ex);
            }
        }
        if (candidates.empty()) candidates = available_for_day;

        string exercise;
        double best_score = numeric_limits<double>::max();
        string best_exercise;
        unordered_map<string, double> prev_day_coverage = day > 1 ? calculate_day_coverage(days[day-1], exercise_contributions) : unordered_map<string, double>();
        auto current_volume = calculate_volume(routine, exercises_map);
        for (size_t i = 0; i < candidates.size(); ++i) {
            const auto& ex = candidates[i];
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

            unordered_map<string, double> temp_coverage = muscle_coverage;
            double temp_time = day_times[day];
            for (const auto& [muscle, contrib] : exercise_contributions.at(ex)) {
                temp_coverage[muscle] += contrib * 3;
            }
            temp_time += calculate_time({ex}, 3);
            double recovery_penalty = 0;
            for (const auto& muscle : exercise_muscles) {
                double prev_workload = prev_day_coverage[muscle];
                double penalty = prev_workload * (primary_muscles.count(muscle) ? 1.0 : 0.25);
                if (primary_muscles.count(muscle)) {
                    recovery_penalty += penalty * 25;
                } else {
                    recovery_penalty += penalty * 5;
                }
            }
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
                        volume_score -= diff * contribution * 200;  // Heavily prioritize volume
                    }
                }
            }
            double usage_score = exercise_usage[ex] * 10;  // Further reduced penalty
            double score = volume_score + recovery_penalty + usage_score;
            if (score < best_score) {
                best_score = score;
                best_exercise = ex;
            }
        }
        if (!best_exercise.empty()) {
            exercise = best_exercise;
        } else {
            // Fallback: relax recovery constraints and pick any exercise
            for (const auto& ex : available_for_day) {
                exercise = ex;
                break;
            }
        }
        // Ensure an exercise was selected and is not already in the day's list
        if (exercise.empty()) {
            // Last resort: pick any available exercise, ignoring all constraints except duplicates
            for (const auto& ex : available_exercises) {
                if (find(current_exercises.begin(), current_exercises.end(), ex) == current_exercises.end()) {
                    exercise = ex;
                    break;
                }
            }
        }
        cout << "  Selected exercise: " << exercise << " (is_leg: " << exercises_map.at(exercise).is_leg
             << ", usage: " << exercise_usage[exercise] << ")\n";
        vector<string> temp_exercises = current_exercises;
        temp_exercises.push_back(exercise);
        vector<vector<string>> temp_structures;
        for (const auto& ex : temp_exercises) temp_structures.push_back({ex});
        if (check_leg_exercise_constraint(temp_structures, exercises_map)) {
            days[day].push_back(exercise);
            ++exercise_usage[exercise];
            for (const auto& [muscle, contrib] : exercise_contributions.at(exercise)) {
                muscle_coverage[muscle] += contrib * 3;
                if (muscle_recovery_days.find(muscle) != muscle_recovery_days.end()) {
                    last_worked_day[muscle] = day;
                    string base_muscle = muscle.substr(0, muscle.find(" ("));
                    muscle_days_worked[base_muscle]++;
                }
            }
            day_times[day] += calculate_time({exercise}, 3);
            ++slots_filled;
            cout << "  Assigned " << exercise << " to Day " << day << ". Day " << day << " now has "
                 << temp_exercises.size() << " exercises (leg exercises: "
                 << (current_leg_exercises + (exercises_map.at(exercise).is_leg ? 1 : 0))
                 << ", new usage: " << exercise_usage[exercise] << ", new time: "
                 << day_times[day] << " minutes)\n";
        } else {
            cout << "  Cannot assign " << exercise << " to Day " << day << ": violates leg exercise constraint. Deferring slot.\n";
            deferred_slots.emplace_back(day, current_exercises, current_leg_exercises);
        }
        day_index = (day_index % total_days) + 1;
    }

    // Handle deferred slots, prioritizing under-volume muscle groups
    if (!deferred_slots.empty()) {
        cout << "\nHandling deferred slots: " << deferred_slots.size() << " slots\n";
        // Calculate volume outside the lambda to capture it properly
        auto current_volume = calculate_volume(routine, exercises_map);
        // Sort deferred slots by the largest volume deficit of associated muscle groups
        sort(deferred_slots.begin(), deferred_slots.end(), 
             [&current_volume, &target_coverage](const auto& a, const auto& b) {
                 double max_deficit_a = 0;
                 double max_deficit_b = 0;
                 for (const auto& [muscle, target] : target_coverage) {
                     double deficit_a = target - current_volume[muscle];
                     double deficit_b = target - current_volume[muscle];
                     if (deficit_a > max_deficit_a) max_deficit_a = deficit_a;
                     if (deficit_b > max_deficit_b) max_deficit_b = deficit_b;
                 }
                 return max_deficit_a > max_deficit_b;
             });

        for (const auto& [day, current_exercises, current_leg_exercises] : deferred_slots) {
            if (current_exercises.size() >= target_exercises_per_day) continue;
            vector<string> non_leg_exercises;
            for (const auto& ex : available_exercises) {
                if (!exercises_map.at(ex).is_leg && find(current_exercises.begin(), current_exercises.end(), ex) == current_exercises.end()) {
                    non_leg_exercises.push_back(ex);
                }
            }
            if (!non_leg_exercises.empty()) {
                double best_score = numeric_limits<double>::max();
                string best_exercise;
                unordered_map<string, double> prev_day_coverage = day > 1 ? calculate_day_coverage(days[day-1], exercise_contributions) : unordered_map<string, double>();
                auto current_volume = calculate_volume(routine, exercises_map);
                for (const auto& ex : non_leg_exercises) {
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

                    unordered_map<string, double> temp_coverage = muscle_coverage;
                    for (const auto& [muscle, contrib] : exercise_contributions.at(ex)) {
                        temp_coverage[muscle] += contrib * 3;
                    }
                    double recovery_penalty = 0;
                    for (const auto& muscle : exercise_muscles) {
                        double prev_workload = prev_day_coverage[muscle];
                        double penalty = prev_workload * (primary_muscles.count(muscle) ? 1.0 : 0.25);
                        if (primary_muscles.count(muscle)) {
                            recovery_penalty += penalty * 25;
                        } else {
                            recovery_penalty += penalty * 5;
                        }
                    }
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
                    double usage_score = exercise_usage[ex] * 10;
                    double score = volume_score + recovery_penalty + usage_score;
                    if (score < best_score) {
                        best_score = score;
                        best_exercise = ex;
                    }
                }
                string exercise;
                if (!best_exercise.empty()) {
                    exercise = best_exercise;
                } else {
                    // Fallback: pick any non-leg exercise
                    exercise = non_leg_exercises[0];
                }
                days[day].push_back(exercise);
                ++exercise_usage[exercise];
                for (const auto& [muscle, contrib] : exercise_contributions.at(exercise)) {
                    muscle_coverage[muscle] += contrib * 3;
                    if (muscle_recovery_days.find(muscle) != muscle_recovery_days.end()) {
                        last_worked_day[muscle] = day;
                        string base_muscle = muscle.substr(0, muscle.find(" ("));
                        muscle_days_worked[base_muscle]++;
                    }
                }
                day_times[day] += calculate_time({exercise}, 3);
                cout << "    Assigned non-leg exercise " << exercise << " to Day " << day << ".\n";
            }
        }
    }

    // Print final assignment and usage counts
    cout << "\nFinal exercise assignment:\n";
    for (int day = 1; day <= total_days; ++day) {
        const auto& day_exercises = days[day];
        int leg_exercises = 0;
        for (const auto& ex : day_exercises) {
            if (exercises_map.at(ex).is_leg) ++leg_exercises;
        }
        cout << "  Day " << day << ": [";
        for (size_t i = 0; i < day_exercises.size(); ++i) {
            cout << "\"" << day_exercises[i] << "\"";
            if (i < day_exercises.size() - 1) cout << ", ";
        }
        cout << "] (total: " << day_exercises.size() << ", leg exercises: " << leg_exercises << ")\n";
    }
    cout << "\nExercise usage counts:\n";
    vector<pair<string, int>> usage_pairs(exercise_usage.begin(), exercise_usage.end());
    sort(usage_pairs.begin(), usage_pairs.end());
    for (const auto& [exercise, count] : usage_pairs) {
        cout << "  " << exercise << ": " << count << " times\n";
    }
    cout << "Initial exercise assignment completed.\n";
}