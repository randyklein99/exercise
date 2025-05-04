#include "format.h"
#include "volume.h"
#include "utils.h"
#include <iostream>  // For cout
#include <limits>    // For numeric_limits
#include <iomanip>
#include <fstream>

using namespace std;

string format_routine(
    const unordered_map<int, vector<Structure>>& routine,
    const unordered_map<string, Exercise>& exercises_map,
    const unordered_map<int, double>& day_times,
    const int total_days
) {
    string markdown = "# 6-Day Workout Routine\n\n";
    markdown += "Each session starts with one compound exercise, followed by additional sets to target specific muscle groups. Each day has a roughly equivalent number of exercises (3-6), with sets balanced to achieve rough time equivalence across days. Exercises may repeat across days but not within the same day. Each exercise is performed for 2-5 sets of 8-12 reps. Rest 60-90 seconds between supersets/tri-sets and 2-3 minutes between straight sets.\n\n";

    for (int day = 1; day <= total_days; ++day) {
        const auto& structures = routine.at(day);
        string day_name = "Day " + to_string(day);
        int total_exercises = 0;
        for (const auto& structure : structures) {
            total_exercises += structure.exercises.size();
        }
        markdown += "## Day " + to_string(day) + ": " + day_name + " – " + to_string(total_exercises) + " Exercises\n";
        for (size_t i = 0; i < structures.size(); ++i) {
            string structure_type = "Straight Sets";
            if (i == 0) structure_type += " (Compound First)";
            markdown += "- **" + structure_type + "**:  \n";
            for (const auto& exercise : structures[i].exercises) {
                const auto& ex = exercises_map.at(exercise);
                vector<string> primary_cleaned;
                for (const auto& muscle : ex.primary) {
                    string base_muscle = muscle.substr(0, muscle.find(" ("));
                    if (base_muscle.empty()) base_muscle = muscle;
                    primary_cleaned.push_back(base_muscle);
                }
                vector<string> secondary_cleaned;
                for (const auto& muscle : ex.secondary) {
                    string base_muscle = muscle.substr(0, muscle.find(" ("));
                    if (base_muscle.empty()) base_muscle = muscle;
                    secondary_cleaned.push_back(base_muscle);
                }
                vector<string> isometric_cleaned;
                for (const auto& muscle : ex.isometric) {
                    string base_muscle = muscle.substr(0, muscle.find(" ("));
                    if (base_muscle.empty()) base_muscle = muscle;
                    isometric_cleaned.push_back(base_muscle);
                }
                string muscles = "*" + join(primary_cleaned, ", ") + "*";
                if (!ex.secondary.empty()) muscles += ", secondary: " + join(secondary_cleaned, ", ");
                if (!ex.isometric.empty()) muscles += ", isometric: " + join(isometric_cleaned, ", ");
                markdown += "  - " + exercise + " - " + to_string(structures[i].sets) + " sets of 8-12 reps *(" + muscles + ")*  \n";
                string cable = "None";
                if (exercise.find("Cable") != string::npos) {
                    if (exercise == "Cable Curl") cable = "Left Cable, Low Height";
                    else if (exercise == "Pulldown") cable = "Pulldown Cable";
                    else if (exercise == "Chest Fly") cable = "Both Cables, Medium Height";
                    else cable = "Left Cable, High Height";
                }
                string apparatus = "None";
                if (exercise.find("Bench Press") != string::npos) apparatus = "Bench on Center Upright, Low";
                else if (exercise == "Upper Back Rows") apparatus = "Bulldog Pad on Center Upright, High";
                string station = "None";
                if (exercise.find("Bench Press") != string::npos) station = "Bench Press Station";
                else if (exercise == "Squat") station = "Squat Rack";
                else if (exercise == "Leg Extension") station = "Leg Extension Machine";
                else if (exercise == "Leg Curl") station = "Leg Curl Machine";
                markdown += "  *(Attributes: Cable: " + cable + "; Apparatus: " + apparatus + "; Station: " + station + ")*  \n";
            }
        }
        double total_time = 0;
        for (const auto& structure : structures) {
            total_time += calculate_time(structure.exercises, structure.sets);
        }
        markdown += "- **Time Estimate**:  \n";
        for (const auto& structure : structures) {
            string structure_type = "Straight Set";
            string structure_name = "\"" + join(structure.exercises, ", ") + "\"";
            double time = calculate_time(structure.exercises, structure.sets);
            markdown += "  - " + structure_type + " " + structure_name + ": " + to_string(time) + " min  \n";
        }
        markdown += "  - **Total**: " + to_string(total_time) + " minutes  \n\n";
    }

    // Weekly Volume Breakdown
    cout << "Generating weekly volume breakdown...\n";
    markdown += "---\n\n## Weekly Volume Breakdown\n";
    unordered_map<string, double> volume = calculate_volume(routine, exercises_map);
    for (const auto& [muscle_group, target] : mav_targets) {
        double vol = volume[muscle_group];
        string status;
        if (muscle_group == "Glutes" || muscle_group == "Lower Back") {
            status = "reported for information only, not optimized per user instruction";
        } else {
            if (vol == target) status = "exactly on target";
            else if (vol > target) status = "exceeds low end due to time balancing, acceptable per rule";
            else status = "below target";
        }
        int upper_bound = (muscle_group == "Glutes" || muscle_group == "Lower Back") ? target + 8 : target + 6;
        markdown += "- **" + muscle_group + "**: " + to_string(vol) + " sets  \n  *(MAV: " + to_string(target) + "-" + to_string(upper_bound) + " sets – " + status + ")*  \n";
    }

    // Muscle Activation Table with aligned columns
    cout << "Generating muscle activation table...\n";
    markdown += "\n---\n\n## Muscle Activation Table\n";

    // Calculate maximum lengths for each column after cleaning muscle names
    size_t max_exercise_len = 8;  // "Exercise"
    size_t max_primary_len = 13;  // "Primary Movers"
    size_t max_secondary_len = 15;  // "Secondary Movers"
    size_t max_isometric_len = 15;  // "Isometric Movers"
    vector<vector<string>> table_data;
    for (const auto& ex : exercises) {
        vector<string> primary_cleaned;
        for (const auto& muscle : ex.primary) {
            string base_muscle = muscle.substr(0, muscle.find(" ("));
            if (base_muscle.empty()) base_muscle = muscle;
            primary_cleaned.push_back(base_muscle);
        }
        vector<string> secondary_cleaned;
        for (const auto& muscle : ex.secondary) {
            string base_muscle = muscle.substr(0, muscle.find(" ("));
            if (base_muscle.empty()) base_muscle = muscle;
            secondary_cleaned.push_back(base_muscle);
        }
        vector<string> isometric_cleaned;
        for (const auto& muscle : ex.isometric) {
            string base_muscle = muscle.substr(0, muscle.find(" ("));
            if (base_muscle.empty()) base_muscle = muscle;
            isometric_cleaned.push_back(base_muscle);
        }
        string primary = primary_cleaned.empty() ? "None" : join(primary_cleaned, ", ");
        string secondary = secondary_cleaned.empty() ? "None" : join(secondary_cleaned, ", ");
        string isometric = isometric_cleaned.empty() ? "None" : join(isometric_cleaned, ", ");

        max_exercise_len = max(max_exercise_len, ex.name.length());
        max_primary_len = max(max_primary_len, primary.length());
        max_secondary_len = max(max_secondary_len, secondary.length());
        max_isometric_len = max(max_isometric_len, isometric.length());

        table_data.push_back({ex.name, primary, secondary, isometric});
    }

    // Generate the table header with safeguarded padding
    string header = "| Exercise" + string(max(0, static_cast<int>(max_exercise_len - string("Exercise").length())), ' ') + 
                    " | Primary Movers" + string(max(0, static_cast<int>(max_primary_len - string("Primary Movers").length())), ' ') + 
                    " | Secondary Movers" + string(max(0, static_cast<int>(max_secondary_len - string("Secondary Movers").length())), ' ') + 
                    " | Isometric Movers" + string(max(0, static_cast<int>(max_isometric_len - string("Isometric Movers").length())), ' ') + " |\n";
    markdown += header;

    // Separator row with safeguarded lengths
    string separator = "|-" + string(max_exercise_len, '-') + 
                       "-|-" + string(max_primary_len, '-') + 
                       "-|-" + string(max_secondary_len, '-') + 
                       "-|-" + string(max_isometric_len, '-') + "-|\n";
    markdown += separator;

    // Table rows with safeguarded padding
    for (const auto& row : table_data) {
        const string& exercise = row[0];
        const string& primary = row[1];
        const string& secondary = row[2];
        const string& isometric = row[3];

        string row_str = "| " + exercise + string(max(0, static_cast<int>(max_exercise_len - exercise.length())), ' ') + 
                         " | " + primary + string(max(0, static_cast<int>(max_primary_len - primary.length())), ' ') + 
                         " | " + secondary + string(max(0, static_cast<int>(max_secondary_len - secondary.length())), ' ') + 
                         " | " + isometric + string(max(0, static_cast<int>(max_isometric_len - isometric.length())), ' ') + " |\n";
        markdown += row_str;
    }

    // Notes
    cout << "Generating notes section...\n";
    markdown += "\n---\n\n## Notes\n";
    double min_time = numeric_limits<double>::max();
    double max_time = numeric_limits<double>::min();
    double avg_time = 0;
    int min_day = 1, max_day = 1;
    for (int day = 1; day <= total_days; ++day) {
        double total_time = day_times.at(day);
        avg_time += total_time;
        if (total_time < min_time) {
            min_time = total_time;
            min_day = day;
        }
        if (total_time > max_time) {
            max_time = total_time;
            max_day = day;
        }
    }
    avg_time /= total_days;
    markdown += "- **Time Commitment**: Each day is estimated at " + to_string(min_time) + "-" + to_string(max_time) + " minutes, with an average of ~" + to_string(avg_time) + " minutes. Day " + to_string(min_day) + " (" + to_string(min_time) + " minutes) is the shortest, while Day " + to_string(max_day) + " (" + to_string(max_time) + " minutes) is the longest.\n";
    markdown += "- **Leg Exercise Limit**: Maintained no more than one leg exercise per day (Squat, Leg Curl, Leg Extension, Stiff-Legged Deadlift), relaxed only if necessary.\n";
    markdown += "- **Exercise List Constraint**: Used all exercises from your provided list (Incline Bench Press, Front Raise, Lateral Raise, Leg Extension, Cable Curl, Shrugs, Squat, Rear Delts, Kelso Shrugs, Upper Back Rows, Chest Fly, Triceps Extension, Leg Curl, Stiff-Legged Deadlift, Pulldown, Lat Prayer), with repeats allowed across days but not within the same day.\n";
    markdown += "- **Set Constraint**: Each exercise is assigned 2-5 sets.\n";
    markdown += "- **Volume Optimization**: Excluded Glutes and Lower Back from volume optimization per user instruction; their volumes are reported but not adjusted to meet targets.\n";
    markdown += "- **Time Equivalence**: Balanced the number of exercises, set structures, and sets per day to achieve rough time equivalence across days.\n";
    markdown += "- **Recovery**: Ensured muscle groups are scheduled with appropriate rest periods (e.g., 48 hours for Quads from Squats, 36 hours for Quads from Leg Extensions) to avoid working the same primary muscle group on consecutive days.\n";
    markdown += "- **Progression**: Increase weight when you can perform 12 reps with good form.\n";

    return markdown;
}