#ifndef CONSTRAINTS_H
#define CONSTRAINTS_H

#include <string>
#include <vector>
#include <unordered_map>
#include "exercise_definitions.h"

bool check_leg_exercise_constraint(const std::vector<std::vector<std::string>>& structures, const std::unordered_map<std::string, Exercise>& exercises_map);
bool can_work_muscle(const std::string& muscle, int current_day, const std::unordered_map<std::string, int>& last_worked_day);

#endif // CONSTRAINTS_H