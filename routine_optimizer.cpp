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

using namespace std;

// Constants
const int TOTAL_DAYS = 6;
const double TIME_PER_SET = 5.0; // minutes per set
const int MIN_EXERCISES_PER_DAY = 3;
const int MAX_EXERCISES_PER_DAY = 6;
const int MIN_SETS = 2;
const int MAX_SETS = 5;
const double MAX_TIME_PER_DAY = 50.0; // minutes

// Struct for an exercise
struct Exercise {
    string name;
    vector<string> primary;
    vector<string> secondary;
    vector<string> isometric;
    bool is_compound;
    bool is_leg;
};

// Struct for muscle group targets
struct MuscleGroup {
    double target;
    double upper_bound;
};

// Struct for routine entry
struct RoutineEntry {
    string exercise;
    int sets;
};

// Muscle recovery days
unordered_map<string, int> muscle_recovery_days = {
    {"Quads", 2}, {"Hamstrings", 2}, {"Glutes", 1}, {"Chest", 1},
    {"Back", 1}, {"Shoulders", 1}, {"Triceps", 1}, {"Biceps", 1},
    {"Short Head", 1}, {"Long Head", 1}, {"Lower Traps", 1},
    {"Lateral Delts", 1}, {"Lower Back", 1}
};

// Exercise definitions
vector<Exercise> exercises = {
    {"Bench Press", {"Chest"}, {"Triceps", "Shoulders"}, {}, true, false},
    {"Squat", {"Quads", "Glutes"}, {"Hamstrings", "Lower Back"}, {}, true, true},
    {"Deadlift", {"Back", "Glutes"}, {"Hamstrings", "Lower Back"}, {}, true, false},
    {"Overhead Press", {"Shoulders"}, {"Triceps"}, {}, true, false},
    {"Pull-Up", {"Back", "Biceps"}, {}, {}, true, false},
    {"Leg Curl", {"Hamstrings"}, {"Short Head"}, {}, false, true},
    {"Leg Extension", {"Quads"}, {}, {}, false, true},
    {"Bicep Curl", {"Biceps"}, {}, {}, false, false},
    {"Tricep Extension", {"Triceps"}, {}, {}, false, false},
    {"Lateral Raise", {"Lateral Delts"}, {"Shoulders"}, {}, false, false},
    {"Kelso Shrugs", {"Lower Traps"}, {"Back"}, {}, false, false},
    {"Stiff-Legged Deadlift", {"Hamstrings", "Lower Back"}, {"Glutes"}, {}, false, true}
};

// Muscle volume targets (MAV: Minimum Adaptive Volume)
unordered_map<string, MuscleGroup> mav_targets = {
    {"Quads", {12.0, 20.0}}, {"Hamstrings", {12.0, 20.0}}, {"Glutes", {8.0, 16.0}},
    {"Chest", {10.0, 18.0}}, {"Back", {12.0, 20.0}}, {"Shoulders", {10.0, 18.0}},
    {"Triceps", {8.0, 16.0}}, {"Biceps", {8.0, 16.0}}, {"Short Head", {8.0, 16.0}},
    {"Long Head", {8.0, 16.0}}, {"Lower Traps", {8.0, 16.0}}, {"Lateral Delts", {8.0, 16.0}},
    {"Lower Back", {8.0, 16.0}}
};

// Check if a muscle is recently used based on recovery days
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

// Compute muscle volumes across the routine
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

// Cost function with penalties
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
                return numeric_limits<double>::max(); // No repeats within a day
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
        if (!compound_first && !routine[day].empty()) {
            compound_first_penalty += 50000.0;
        }
        if (routine[day].size() < MIN_EXERCISES_PER_DAY || routine[day].size() > MAX_EXERCISES_PER_DAY || leg_exercises > 2) {
            return numeric_limits<double>::max();
        }
        if (day_time > MAX_TIME_PER_DAY) {
            total_time_penalty += 4000.0 * (day_time - MAX_TIME_PER_DAY);
        }
        day_times[day] = day_time;
    }

    if (included_exercises.find("Leg Curl") == included_exercises.end()) {
        inclusion_penalty += 80000.0; // Ensure Short Head coverage
    }

    double avg_time = accumulate(day_times.begin(), day_times.end(), 0.0) / TOTAL_DAYS;
    for (double time : day_times) {
        double diff = time - avg_time;
        time_variance_penalty += (diff > 0 ? 1500.0 : 1000.0) * pow(diff, 2);
    }

    for (const auto& [muscle, target] : mav_targets) {
        if (muscle == "Glutes" || muscle == "Lower Back") continue;
        double vol = volumes.count(muscle) ? volumes[muscle] : 0.0;
        double deficit = target.target - vol;
        if (deficit > 0) {
            double weight = (muscle == "Short Head" || muscle == "Lower Traps" || muscle == "Lateral Delts" || muscle == "Quads") ? 15000.0 : 8000.0;
            volume_penalty += weight * pow(deficit, 2);
        } else if (vol > target.upper_bound) {
            volume_penalty += 200.0 * pow(vol - target.upper_bound, 2);
        }
    }

    for (const auto& [ex, freq] : exercise_frequency) {
        if (freq > 2) {
            frequency_penalty += 10000.0 * (freq - 2);
        }
    }

    return volume_penalty + frequency_penalty + total_time_penalty + time_variance_penalty + compound_first_penalty + inclusion_penalty;
}

// Perturb the routine
void perturb_routine(vector<vector<RoutineEntry>>& routine, 
                     const vector<Exercise>& exercises, 
                     const unordered_map<string, MuscleGroup>& mav_targets,
                     mt19937& gen) {
    uniform_int_distribution<> day_dist(0, TOTAL_DAYS - 1);
    uniform_int_distribution<> action_dist(0, 5);
    int action = action_dist(gen);

    auto volumes = compute_volumes(routine, exercises);
    vector<string> under_target_muscles;
    for (const auto& [muscle, target] : mav_targets) {
        double vol = volumes.count(muscle) ? volumes[muscle] : 0.0;
        if (vol < target.target) under_target_muscles.push_back(muscle);
    }

    if (action == 0 && routine[day_dist(gen)].size() > MIN_EXERCISES_PER_DAY) { // Remove
        int day = day_dist(gen);
        uniform_int_distribution<> ex_dist(1, routine[day].size() - 1);
        routine[day].erase(routine[day].begin() + ex_dist(gen));
    } else if (action == 1 && !routine[day_dist(gen)].empty()) { // Replace
        int day = day_dist(gen);
        uniform_int_distribution<> idx_dist(1, routine[day].size() - 1);
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
            bool is_leg = ex.is_leg;
            if (!current_exercises.count(ex.name) && (!is_leg || leg_count < 2) && !ex.is_compound) {
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
            uniform_int_distribution<> sets_dist(MIN_SETS + 1, MAX_SETS);
            routine[day][idx].sets = sets_dist(gen);
        }
    } else if (action == 2 && routine[day_dist(gen)].size() < MAX_EXERCISES_PER_DAY) { // Add
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
            bool is_leg = ex.is_leg;
            if (!current_exercises.count(ex.name) && (!is_leg || leg_count < 2) && !ex.is_compound) {
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
            uniform_int_distribution<> sets_dist(MIN_SETS + 1, MAX_SETS);
            routine[day].push_back({new_ex, sets_dist(gen)});
        }
    } else if (action == 3 && TOTAL_DAYS > 1) { // Swap days
        int day1 = day_dist(gen);
        int day2 = day_dist(gen);
        while (day2 == day1) day2 = day_dist(gen);
        swap(routine[day1], routine[day2]);
    } else if (action == 4) { // Adjust sets
        int day = day_dist(gen);
        for (auto& entry : routine[day]) {
            auto ex = find_if(exercises.begin(), exercises.end(), [&](const Exercise& e) { return e.name == entry.exercise; });
            if (ex != exercises.end()) {
                for (const auto& muscle : ex->primary) {
                    if (find(under_target_muscles.begin(), under_target_muscles.end(), muscle) != under_target_muscles.end()) {
                        uniform_int_distribution<> sets_adj(1, 2);
                        entry.sets = min(entry.sets + sets_adj(gen), MAX_SETS);
                        break;
                    }
                }
            }
        }
    } else if (action == 5 && routine[day_dist(gen)].size() > 2) { // Swap within day
        int day = day_dist(gen);
        uniform_int_distribution<> idx1_dist(1, routine[day].size() - 1);
        uniform_int_distribution<> idx2_dist(1, routine[day].size() - 1);
        int idx1 = idx1_dist(gen);
        int idx2 = idx2_dist(gen);
        while (idx2 == idx1) idx2 = idx2_dist(gen);
        swap(routine[day][idx1], routine[day][idx2]);
    }
}

// Initialize routine
vector<vector<RoutineEntry>> initialize_routine(const vector<Exercise>& exercises, mt19937& gen) {
    vector<vector<RoutineEntry>> routine(TOTAL_DAYS);
    vector<Exercise> compounds;
    vector<Exercise> isolations;

    for (const auto& ex : exercises) {
        if (ex.is_compound) compounds.push_back(ex);
        else isolations.push_back(ex);
    }

    uniform_int_distribution<> isolation_dist(3, 5);
    set<string> critical_exercises = {"Leg Curl", "Kelso Shrugs", "Lateral Raise"};

    for (int day = 0; day < TOTAL_DAYS; ++day) {
        set<string> used_exercises;
        int leg_count = 0;

        shuffle(compounds.begin(), compounds.end(), gen);
        for (const auto& ex : compounds) {
            if (used_exercises.insert(ex.name).second && (!ex.is_leg || leg_count < 2)) {
                bool valid = true;
                for (const auto& muscle : ex.primary) {
                    if (is_muscle_recently_used(routine, day, muscle, exercises)) {
                        valid = false;
                        break;
                    }
                }
                if (valid) {
                    uniform_int_distribution<> sets_dist(MIN_SETS + 1, MAX_SETS);
                    routine[day].push_back({ex.name, sets_dist(gen)});
                    if (ex.is_leg) leg_count++;
                    break;
                }
            }
        }

        shuffle(isolations.begin(), isolations.end(), gen);
        for (const auto& ex : isolations) {
            if (critical_exercises.count(ex.name) && used_exercises.insert(ex.name).second && (!ex.is_leg || leg_count < 2)) {
                bool valid = true;
                for (const auto& muscle : ex.primary) {
                    if (is_muscle_recently_used(routine, day, muscle, exercises)) {
                        valid = false;
                        break;
                    }
                }
                if (valid) {
                    uniform_int_distribution<> sets_dist(MIN_SETS + 1, MAX_SETS);
                    routine[day].push_back({ex.name, sets_dist(gen)});
                    if (ex.is_leg) leg_count++;
                }
            }
        }

        int target_exercises = isolation_dist(gen);
        for (const auto& ex : isolations) {
            if (used_exercises.size() >= target_exercises) break;
            if (used_exercises.insert(ex.name).second && (!ex.is_leg || leg_count < 2)) {
                bool valid = true;
                for (const auto& muscle : ex.primary) {
                    if (is_muscle_recently_used(routine, day, muscle, exercises)) {
                        valid = false;
                        break;
                    }
                }
                if (valid) {
                    uniform_int_distribution<> sets_dist(MIN_SETS + 1, MAX_SETS);
                    routine[day].push_back({ex.name, sets_dist(gen)});
                    if (ex.is_leg) leg_count++;
                }
            }
        }
    }
    return routine;
}

// Convert vector to string
string vector_to_string(const vector<string>& vec) {
    stringstream ss;
    for (size_t i = 0; i < vec.size(); ++i) {
        if (i != 0) ss << ", ";
        ss << vec[i];
    }
    return ss.str();
}

// Save to Markdown file
void save_to_file(const vector<vector<RoutineEntry>>& routine, const vector<Exercise>& exercises, const unordered_map<string, MuscleGroup>& mav_targets, const string& filename) {
    ofstream out(filename);
    if (!out) {
        cerr << "Error opening file" << endl;
        return;
    }

    out << "# 6-Day Workout Routine\n\n";
    out << "Each session starts with one compound exercise, followed by additional sets to target specific muscle groups.\n\n";

    for (int day = 0; day < TOTAL_DAYS; ++day) {
        out << "## Day " << (day + 1) << ": " << routine[day].size() << " Exercises\n";
        for (size_t i = 0; i < routine[day].size(); ++i) {
            const auto& entry = routine[day][i];
            auto it = find_if(exercises.begin(), exercises.end(), [&](const Exercise& e) { return e.name == entry.exercise; });
            if (it == exercises.end()) continue;
            const Exercise& ex = *it;
            string set_type = (i == 0 && ex.is_compound) ? "Straight Sets (Compound First)" : "Straight Sets";
            out << "- **" << set_type << "**: " << entry.exercise << " - " << entry.sets << " sets of 8-12 reps *(";
            stringstream ss;
            ss << "*" << vector_to_string(ex.primary) << "*";
            if (!ex.secondary.empty()) ss << ", secondary: " << vector_to_string(ex.secondary);
            if (!ex.isometric.empty()) ss << ", isometric: " << vector_to_string(ex.isometric);
            out << ss.str() << ")*\n";
        }
        out << "- **Time Estimate**: ";
        double total_time = 0.0;
        for (const auto& entry : routine[day]) {
            total_time += entry.sets * TIME_PER_SET;
        }
        out << static_cast<int>(total_time) << " minutes\n\n";
    }

    out << "## Weekly Volume Breakdown\n";
    auto volumes = compute_volumes(routine, exercises);
    for (const auto& muscle : mav_targets) {
        string name = muscle.first;
        double target = muscle.second.target;
        double upper_bound = muscle.second.upper_bound;
        double vol = volumes.count(name) ? volumes[name] : 0.0;
        string status = (name == "Glutes" || name == "Lower Back") ? "not optimized" : (vol < target ? "below target" : (vol > upper_bound ? "exceeds upper bound" : "on target"));
        out << "- **" << name << "**: " << fixed << setprecision(2) << vol << " sets (" << target << "-" << upper_bound << " sets – " << status << ")\n";
    }
    out.close();
}

// Optimize routine with simulated annealing
vector<vector<RoutineEntry>> optimize_routine(const vector<Exercise>& exercises, 
                                              const unordered_map<string, MuscleGroup>& mav_targets) {
    random_device rd;
    mt19937 gen(rd());
    vector<vector<RoutineEntry>> routine = initialize_routine(exercises, gen);
    double temp = 1500.0;
    double cooling_rate = 0.993;
    double best_cost = compute_cost(routine, mav_targets, exercises);
    vector<vector<RoutineEntry>> best_routine = routine;

    cout << "Starting optimization..." << endl;
    for (int iter = 0; iter < 75000; ++iter) {
        vector<vector<RoutineEntry>> new_routine = routine;
        perturb_routine(new_routine, exercises, mav_targets, gen);
        double new_cost = compute_cost(new_routine, mav_targets, exercises);
        double current_cost = compute_cost(routine, mav_targets, exercises);

        if (new_cost < current_cost || uniform_real_distribution<>(0, 1)(gen) < exp((current_cost - new_cost) / temp)) {
            routine = new_routine;
            if (new_cost < best_cost) {
                best_cost = new_cost;
                best_routine = routine;
                cout << "New best cost at iteration " << iter << ": " << best_cost << endl;
            }
        }

        if (iter % 1000 == 0) {
            cout << "Iteration " << iter << ": Temp = " << temp << ", Best Cost = " << best_cost << endl;
        }
        temp *= cooling_rate;
    }
    cout << "Optimization complete. Final best cost: " << best_cost << endl;
    return best_routine;
}

int main() {
    vector<vector<RoutineEntry>> routine = optimize_routine(exercises, mav_targets);
    for (int day = 0; day < TOTAL_DAYS; ++day) {
        cout << "Day " << day + 1 << ":\n";
        for (const auto& entry : routine[day]) {
            cout << "  " << entry.exercise << " - " << entry.sets << " sets\n";
        }
    }
    save_to_file(routine, exercises, mav_targets, "workout_routine.md");
    cout << "Workout routine saved to workout_routine.md\n";
    return 0;
}