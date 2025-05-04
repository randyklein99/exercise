#ifndef ASSIGN_H
#define ASSIGN_H

#include <string>
#include <vector>
#include <unordered_map>
#include <random>
#include "exercise_definitions.h"

void assign_exercises(
    std::unordered_map<int, std::vector<std::string>>& days,
    std::unordered_map<std::string, int>& exercise_usage,
    std::unordered_map<std::string, double>& muscle_coverage,
    std::unordered_map<std::string, int>& last_worked_day,
    const std::unordered_map<std::string, std::unordered_map<std::string, double>>& exercise_contributions,
    const std::unordered_map<std::string, Exercise>& exercises_map,
    std::unordered_map<int, double>& day_times,
    const std::unordered_map<std::string, double>& target_coverage,
    const std::vector<Exercise>& exercises,
    int target_exercises_per_day,
    int total_days,
    std::mt19937& g,
    std::unordered_map<int, std::vector<Structure>>& routine  // Add routine as a parameter
);

#endif // ASSIGN_H