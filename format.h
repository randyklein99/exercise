#ifndef FORMAT_H
#define FORMAT_H

#include <string>
#include <vector>
#include <unordered_map>
#include "exercise_definitions.h"
#include "utils.h"

std::string format_routine(
    const std::unordered_map<int, std::vector<Structure>>& routine,
    const std::unordered_map<std::string, Exercise>& exercises_map,
    const std::unordered_map<int, double>& day_times,
    const int total_days
);

#endif // FORMAT_H