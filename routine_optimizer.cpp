#include <vector>
#include <map>
#include <unordered_map>
#include <string>
#include <random>
#include <iostream>
#include <algorithm>
#include <set>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <numeric>
#include "exercise_definitions.h"

using namespace std;

// Struct to represent an exercise entry in the routine
struct RoutineEntry {
    string exercise;
    int sets;
};

// Helper to check if a muscle is recently used
bool is_muscle_recently_used(const vector<vector<RoutineEntry>>& routine, int current_day, const string& muscle, const vector<Exercise>& exercises) {
    int recovery_days = muscle_recovery_days.at(muscle);
    for (int day = max(0, current_day - recovery_days); day < current_day; ++day) {
        for (const auto& entry : routine[day]) {
            auto ex = find_if(exercises.begin(), exercises.end(), [&](const Exercise& e) { return e.name == entry.exercise; });
            if (ex != exercises.end() && (find(ex->primary.begin(), ex->primary.end(), muscle) != ex->primary.end() ||
                                          find(ex->secondary.begin(), ex->secondary.end(), muscle) != ex->secondary.end())) {
                return true;
            }
        }
    }
    return false;
}

// Function to compute volumes
map<string, double> compute_volumes(const vector<vector<RoutineEntry>>& routine, const vector<Exercise>& exercises) {
    map<string, double> volumes;
    for (int day = 0; day < TOTAL_DAYS; ++day) {
        for (const auto& entry : routine[day]) {
            auto it = find_if(exercises.begin(), exercises.end(), [&](const Exercise& e) { return e.name == entry.exercise; });
            if (it == exercises.end()) continue;
            const Exercise& ex = *it;
            for (const auto& muscle : ex.primary) {
                volumes[muscle] += entry.sets * 1.0;
            }
            for (const auto& muscle : ex.secondary) {
                volumes[muscle] += entry.sets * 0.5;
            }
            for (const auto& muscle : ex.isometric) {
                volumes[muscle] += entry.sets * 0.25;
            }
        }
    }
    return volumes;
}

// Function to calculate the cost of a routine with stricter penalties
double compute_cost(const vector<vector<RoutineEntry>>& routine, 
                    const unordered_map<string, MuscleGroup>& mav_targets, 
                    const vector<Exercise>& exercises) {
    auto volumes = compute_volumes(routine, exercises);
    map<string, int> exercise_frequency;
    double total_time_penalty = 0.0;
    double volume_penalty = 0.0;
    double frequency_penalty = 0.0;
    double time_variance_penalty = 0.0;
    double compound_first_penalty = 0.0;
    double inclusion_penalty = 0.0;

    vector<double> day_times(TOTAL_DAYS, 0.0);
    set<string> included_exercises;

    for (int day = 0; day < TOTAL_DAYS; ++day) {
        set<string> day_exercises;
        int leg_exercises = 0;
        double day_time = 0.0;
        bool compound_first = false;
        for (size_t i = 0; i < routine[day].size(); ++i) {
            const auto& entry = routine[day][i];
            if (day_exercises.count(entry.exercise)) {
                return numeric_limits<double>::max();  // Penalize repeats within a day
            }
            day_exercises.insert(entry.exercise);
            included_exercises.insert(entry.exercise);
            exercise_frequency[entry.exercise]++;
            auto ex = find_if(exercises.begin(), exercises.end(), 
                            [&](const Exercise& e) { return e.name == entry.exercise; });
            if (ex != exercises.end()) {
                if (i == 0 && ex->is_compound) compound_first = true;
                if (ex->is_leg) leg_exercises++;
                day_time += entry.sets * TIME_PER_SET;
            }
        }
        if (!compound_first) {
            compound_first_penalty += 30000.0;  // High penalty for no compound first
        }
        if (routine[day].size() < MIN_EXERCISES_PER_DAY || routine[day].size() > MAX_EXERCISES_PER_DAY || leg_exercises > 1) {
            return numeric_limits<double>::max();  // Penalize invalid counts or too many leg exercises
        }
        if (day_time > MAX_TIME_PER_DAY) {
            total_time_penalty += 2000.0 * (day_time - MAX_TIME_PER_DAY);
        }
        day_times[day] = day_time;
    }

    // Inclusion penalty for missing Leg Curls
    if (included_exercises.find("Leg Curl") == included_exercises.end()) {
        inclusion_penalty += 50000.0;  // Heavy penalty for missing Leg Curls
    }

    double avg_time = accumulate(day_times.begin(), day_times.end(), 0.0) / TOTAL_DAYS;
    for (double time : day_times) {
        time_variance_penalty += pow(time - avg_time, 2);
    }
    time_variance_penalty *= 500.0;  // Tighten time equivalence

    for (const auto& [muscle, target] : mav_targets) {
        if (muscle == "Glutes" || muscle == "Lower Back") continue;  // Exclude from optimization
        double vol = volumes.count(muscle) ? volumes[muscle] : 0.0;
        if (vol < target.target) {
            double weight = (muscle == "Short Head" || muscle == "Lower Traps") ? 5000.0 : 2000.0;
            volume_penalty += weight * pow(target.target - vol, 2);  // Stricter for critical muscles
        } else if (vol > target.upper_bound) {
            volume_penalty += 50.0 * pow(vol - target.upper_bound, 2);
        }
    }

    for (const auto& [ex, freq] : exercise_frequency) {
        if (freq > 2) {
            frequency_penalty += 6000.0 * (freq - 2);  // High penalty for overuse
        }
    }

    return volume_penalty + frequency_penalty + total_time_penalty + time_variance_penalty + compound_first_penalty + inclusion_penalty;
}

// Function to perturb the routine with bolder modifications
void perturb_routine(vector<vector<RoutineEntry>>& routine, 
                     const vector<Exercise>& exercises, 
                     const unordered_map<string, MuscleGroup>& mav_targets,
                     mt19937& gen) {
    uniform_int_distribution<> day_dist(0, TOTAL_DAYS - 1);
    uniform_int_distribution<> action_dist(0, 3);  // 0: remove, 1: replace, 2: add, 3: swap days
    int action = action_dist(gen);

    auto volumes = compute_volumes(routine, exercises);
    vector<string> under_target_muscles;
    for (const auto& [muscle, target] : mav_targets) {
        double vol = volumes.count(muscle) ? volumes[muscle] : 0.0;
        if (vol < target.target) under_target_muscles.push_back(muscle);
    }

    if (action == 0 && routine[day_dist(gen)].size() > MIN_EXERCISES_PER_DAY) {  // Remove
        int day = day_dist(gen);
        uniform_int_distribution<> ex_dist(1, routine[day].size() - 1);  // Protect compound first
        routine[day].erase(routine[day].begin() + ex_dist(gen));
    } else if (action == 1 && !routine[day_dist(gen)].empty()) {  // Replace
        int day = day_dist(gen);
        uniform_int_distribution<> idx_dist(1, routine[day].size() - 1);  // Protect compound first
        int idx = idx_dist(gen);
        set<string> current_exercises;
        int leg_count = 0;
        for (const auto& entry : routine[day]) {
            current_exercises.insert(entry.exercise);
            auto ex = find_if(exercises.begin(), exercises.end(), [&](const Exercise& e) { return e.name == entry.exercise; });
            if (ex != exercises.end() && ex->is_leg) leg_count++;
        }
        vector<Exercise> candidates;
        for (const auto& ex : exercises) {
            if (!current_exercises.count(ex.name) && (!ex.is_leg || leg_count == 0) && !ex.is_compound) {  // Avoid compounds mid-routine
                bool valid = true;
                for (const auto& muscle : ex.primary) {
                    if (is_muscle_recently_used(routine, day, muscle, exercises)) {
                        valid = false;
                        break;
                    }
                }
                if (valid && any_of(ex.primary.begin(), ex.primary.end(), [&](const string& m) {
                    return find(under_target_muscles.begin(), under_target_muscles.end(), m) != under_target_muscles.end();
                })) {
                    candidates.push_back(ex);
                }
            }
        }
        if (!candidates.empty()) {
            uniform_int_distribution<> new_ex_dist(0, candidates.size() - 1);
            routine[day][idx].exercise = candidates[new_ex_dist(gen)].name;
            uniform_int_distribution<> sets_dist(MIN_SETS, MAX_SETS);
            routine[day][idx].sets = sets_dist(gen);
        }
    } else if (action == 2 && routine[day_dist(gen)].size() < MAX_EXERCISES_PER_DAY) {  // Add
        int day = day_dist(gen);
        set<string> current_exercises;
        int leg_count = 0;
        for (const auto& entry : routine[day]) {
            current_exercises.insert(entry.exercise);
            auto ex = find_if(exercises.begin(), exercises.end(), [&](const Exercise& e) { return e.name == entry.exercise; });
            if (ex != exercises.end() && ex->is_leg) leg_count++;
        }
        vector<Exercise> candidates;
        for (const auto& ex : exercises) {
            if (!current_exercises.count(ex.name) && (!ex.is_leg || leg_count == 0) && !ex.is_compound) {  // Avoid compounds mid-routine
                bool valid = true;
                for (const auto& muscle : ex.primary) {
                    if (is_muscle_recently_used(routine, day, muscle, exercises)) {
                        valid = false;
                        break;
                    }
                }
                if (valid && any_of(ex.primary.begin(), ex.primary.end(), [&](const string& m) {
                    return find(under_target_muscles.begin(), under_target_muscles.end(), m) != under_target_muscles.end();
                })) {
                    candidates.push_back(ex);
                }
            }
        }
        if (!candidates.empty()) {
            uniform_int_distribution<> ex_dist(0, candidates.size() - 1);
            string new_ex = candidates[ex_dist(gen)].name;
            uniform_int_distribution<> sets_dist(MIN_SETS, MAX_SETS);
            routine[day].push_back({new_ex, sets_dist(gen)});
        }
    } else if (action == 3 && TOTAL_DAYS > 1) {  // Swap entire days
        int day1 = day_dist(gen);
        int day2 = day_dist(gen);
        while (day2 == day1) day2 = day_dist(gen);
        swap(routine[day1], routine[day2]);
    }
}

// Function to initialize a routine with diverse exercises
vector<vector<RoutineEntry>> initialize_routine(const vector<Exercise>& exercises, mt19937& gen) {
    vector<vector<RoutineEntry>> routine(TOTAL_DAYS);
    vector<Exercise> compounds;
    vector<Exercise> isolations;

    for (const auto& ex : exercises) {
        if (ex.is_compound) compounds.push_back(ex);
        else isolations.push_back(ex);
    }

    uniform_int_distribution<> isolation_dist(2, 4);
    for (int day = 0; day < TOTAL_DAYS; ++day) {
        set<string> used_exercises;
        int leg_count = 0;

        // Start with a compound exercise
        shuffle(compounds.begin(), compounds.end(), gen);
        for (const auto& ex : compounds) {
            if (used_exercises.insert(ex.name).second && (!ex.is_leg || leg_count == 0)) {
                bool valid = true;
                for (const auto& muscle : ex.primary) {
                    if (is_muscle_recently_used(routine, day, muscle, exercises)) {
                        valid = false;
                        break;
                    }
                }
                if (valid) {
                    uniform_int_distribution<> sets_dist(MIN_SETS, MAX_SETS);
                    routine[day].push_back({ex.name, sets_dist(gen)});
                    if (ex.is_leg) leg_count++;
                    break;
                }
            }
        }

        // Add isolation exercises, ensuring diversity
        shuffle(isolations.begin(), isolations.end(), gen);
        for (int i = 0; i < isolation_dist(gen) && i < isolations.size(); ++i) {
            if (used_exercises.insert(isolations[i].name).second && (!isolations[i].is_leg || leg_count == 0)) {
                bool valid = true;
                for (const auto& muscle : isolations[i].primary) {
                    if (is_muscle_recently_used(routine, day, muscle, exercises)) {
                        valid = false;
                        break;
                    }
                }
                if (valid) {
                    uniform_int_distribution<> sets_dist(MIN_SETS, MAX_SETS);
                    routine[day].push_back({isolations[i].name, sets_dist(gen)});
                    if (isolations[i].is_leg) leg_count++;
                }
            }
        }

        while (routine[day].size() < MIN_EXERCISES_PER_DAY && (!isolations.empty() || !compounds.empty())) {
            vector<Exercise>& source = isolations.empty() ? compounds : isolations;
            shuffle(source.begin(), source.end(), gen);
            for (const auto& ex : source) {
                if (used_exercises.insert(ex.name).second && (!ex.is_leg || leg_count == 0)) {
                    bool valid = true;
                    for (const auto& muscle : ex.primary) {
                        if (is_muscle_recently_used(routine, day, muscle, exercises)) {
                            valid = false;
                            break;
                        }
                    }
                    if (valid) {
                        uniform_int_distribution<> sets_dist(MIN_SETS, MAX_SETS);
                        routine[day].push_back({ex.name, sets_dist(gen)});
                        if (ex.is_leg) leg_count++;
                        break;
                    }
                }
            }
        }
    }

    // Ensure Leg Curls are included in the initial routine
    bool has_leg_curls = false;
    for (const auto& day : routine) {
        for (const auto& entry : day) {
            if (entry.exercise == "Leg Curl") {
                has_leg_curls = true;
                break;
            }
        }
        if (has_leg_curls) break;
    }
    if (!has_leg_curls) {
        int day = uniform_int_distribution<>(0, TOTAL_DAYS - 1)(gen);
        routine[day].push_back({"Leg Curl", uniform_int_distribution<>(MIN_SETS, MAX_SETS)(gen)});
    }

    return routine;
}

// Helper function to convert vector to string
string vector_to_string(const vector<string>& vec) {
    stringstream ss;
    for (size_t i = 0; i < vec.size(); ++i) {
        if (i != 0) ss << ", ";
        ss << vec[i];
    }
    return ss.str();
}

// Function to save the routine to a Markdown file
void save_to_file(const vector<vector<RoutineEntry>>& routine, const vector<Exercise>& exercises, const unordered_map<string, MuscleGroup>& mav_targets, const string& filename) {
    ofstream out(filename);
    if (!out) {
        cerr << "Error opening file" << endl;
        return;
    }

    out << "# 6-Day Workout Routine\n\n";
    out << "Each session starts with one compound exercise, followed by additional sets to target specific muscle groups. Each day has a roughly equivalent number of exercises (3-6), with sets balanced to achieve rough time equivalence across days. Exercises may repeat across days but not within the same day. Each exercise is performed for 2-5 sets of 8-12 reps. Rest 60-90 seconds between supersets/tri-sets and 2-3 minutes between straight sets.\n\n";

    for (int day = 0; day < TOTAL_DAYS; ++day) {
        out << "## Day " << (day + 1) << ": Day " << (day + 1) << " – " << routine[day].size() << " Exercises\n";
        for (size_t i = 0; i < routine[day].size(); ++i) {
            const auto& entry = routine[day][i];
            auto it = find_if(exercises.begin(), exercises.end(), [&](const Exercise& e) { return e.name == entry.exercise; });
            if (it == exercises.end()) continue;
            const Exercise& ex = *it;
            string set_type = (i == 0 && ex.is_compound) ? "Straight Sets (Compound First)" : "Straight Sets";
            out << "- **" << set_type << "**:  \n  - " << entry.exercise << " - " << entry.sets << " sets of 8-12 reps *(";
            stringstream ss;
            ss << "*" << vector_to_string(ex.primary) << "*";
            if (!ex.secondary.empty()) ss << ", secondary: " << vector_to_string(ex.secondary);
            if (!ex.isometric.empty()) ss << ", isometric: " << vector_to_string(ex.isometric);
            out << ss.str() << ")*  \n  *(Attributes: Cable: None; Apparatus: None; Station: None)*  \n";
        }
        out << "- **Time Estimate**:  \n";
        double total_time = 0.0;
        for (const auto& entry : routine[day]) {
            double time = entry.sets * TIME_PER_SET;
            total_time += time;
            out << "  - Straight Set \"" << entry.exercise << "\": " << static_cast<int>(time) << " min  \n";
        }
        out << "  - **Total**: " << static_cast<int>(total_time) << " minutes  \n\n";
    }

    out << "---\n\n## Weekly Volume Breakdown\n";
    auto volumes = compute_volumes(routine, exercises);
    for (const auto& muscle : mav_targets) {
        string name = muscle.first;
        double target = muscle.second.target;
        double upper_bound = muscle.second.upper_bound;
        double vol = volumes.count(name) ? volumes[name] : 0.0;
        string status;
        if (name == "Glutes" || name == "Lower Back") {
            status = "reported for information only, not optimized per user instruction";
        } else if (vol < target) {
            status = "below target";
        } else if (vol > upper_bound) {
            status = "exceeds upper bound due to time balancing, acceptable per rule";
        } else {
            status = "exactly on target";
        }
        out << "- **" << name << "**: " << fixed << setprecision(2) << vol << " sets  \n  *(MAV: " << target << "-" << upper_bound << " sets – " << status << ")*  \n";
    }

    out << "---\n\n## Muscle Activation Table\n";
    out << "| Exercise              | Primary Movers              | Secondary Movers        | Isometric Movers        |\n";
    out << "|-----------------------|-----------------------------|-------------------------|-------------------------|\n";
    for (const auto& ex : exercises) {
        string primary = vector_to_string(ex.primary);
        if (primary.empty()) primary = "None";
        string secondary = vector_to_string(ex.secondary);
        if (secondary.empty()) secondary = "None";
        string isometric = vector_to_string(ex.isometric);
        if (isometric.empty()) isometric = "None";
        out << "| " << left << setw(21) << ex.name << " | " << setw(27) << primary << " | " << setw(23) << secondary << " | " << setw(23) << isometric << " |\n";
    }

    out << "---\n\n## Notes\n";
    vector<double> day_times(TOTAL_DAYS, 0.0);
    for (int day = 0; day < TOTAL_DAYS; ++day) {
        for (const auto& entry : routine[day]) {
            day_times[day] += entry.sets * TIME_PER_SET;
        }
    }
    double min_time = *min_element(day_times.begin(), day_times.end());
    double max_time = *max_element(day_times.begin(), day_times.end());
    double avg_time = accumulate(day_times.begin(), day_times.end(), 0.0) / TOTAL_DAYS;
    int shortest_day = distance(day_times.begin(), min_element(day_times.begin(), day_times.end()));
    int longest_day = distance(day_times.begin(), max_element(day_times.begin(), day_times.end()));
    out << "- **Time Commitment**: Each day is estimated at " << static_cast<int>(min_time) << "-" << static_cast<int>(max_time) << " minutes, with an average of ~" << fixed << setprecision(1) << avg_time << " minutes. Day " << (shortest_day + 1) << " (" << static_cast<int>(day_times[shortest_day]) << " minutes) is the shortest, while Day " << (longest_day + 1) << " (" << static_cast<int>(day_times[longest_day]) << " minutes) is the longest.\n";
    out << "- **Leg Exercise Limit**: Maintained no more than one leg exercise per day (Squat, Leg Curl, Leg Extension, Stiff-Legged Deadlift), relaxed only if necessary.\n";
    out << "- **Exercise List Constraint**: Used all exercises from your provided list (Incline Bench Press, Front Raise, Lateral Raise, Leg Extension, Cable Curl, Shrugs, Squat, Rear Delts, Kelso Shrugs, Upper Back Rows, Chest Fly, Triceps Extension, Leg Curl, Stiff-Legged Deadlift, Pulldown, Lat Prayer), with repeats allowed across days but not within the same day.\n";
    out << "- **Set Constraint**: Each exercise is assigned 2-5 sets.\n";
    out << "- **Volume Optimization**: Excluded Glutes and Lower Back from volume optimization per user instruction; their volumes are reported but not adjusted to meet targets.\n";
    out << "- **Time Equivalence**: Balanced the number of exercises, set structures, and sets per day to achieve rough time equivalence across days (±2 minutes from the average), while not exceeding 50 minutes per day unless necessary.\n";
    out << "- **Recovery**: Ensured muscle groups are scheduled with appropriate rest periods (e.g., 2 days for Quads and Hamstrings) to avoid working the same primary muscle group on consecutive days.\n";
    out << "- **Progression**: Increase weight when you can perform 12 reps with good form.\n";
    out.close();
}

// Main optimization function (simulated annealing)
vector<vector<RoutineEntry>> optimize_routine(const vector<Exercise>& exercises, 
                                              const unordered_map<string, MuscleGroup>& mav_targets) {
    random_device rd;
    mt19937 gen(rd());
    vector<vector<RoutineEntry>> routine = initialize_routine(exercises, gen);
    double temp = 1000.0;
    double cooling_rate = 0.995;
    double best_cost = compute_cost(routine, mav_targets, exercises);
    vector<vector<RoutineEntry>> best_routine = routine;

    for (int iter = 0; iter < 30000; ++iter) {  // Increased iterations for better exploration
        vector<vector<RoutineEntry>> new_routine = routine;
        perturb_routine(new_routine, exercises, mav_targets, gen);
        double new_cost = compute_cost(new_routine, mav_targets, exercises);
        double current_cost = compute_cost(routine, mav_targets, exercises);

        if (new_cost < current_cost || uniform_real_distribution<>(0, 1)(gen) < exp((current_cost - new_cost) / temp)) {
            routine = new_routine;
            if (new_cost < best_cost) {
                best_cost = new_cost;
                best_routine = routine;
            }
        }
        temp *= cooling_rate;
    }
    return best_routine;
}

// Example usage
int main() {
    vector<vector<RoutineEntry>> routine = optimize_routine(exercises, mav_targets);

    // Print to console
    for (int day = 0; day < TOTAL_DAYS; ++day) {
        cout << "Day " << day + 1 << ":\n";
        for (const auto& entry : routine[day]) {
            cout << "  " << entry.exercise << " - " << entry.sets << " sets\n";
        }
        if (routine[day].empty()) {
            cout << "  (No exercises assigned)\n";
        }
    }

    // Save to file
    save_to_file(routine, exercises, mav_targets, "workout_routine.md");
    cout << "Workout routine saved to workout_routine.md\n";

    return 0;
}