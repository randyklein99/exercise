#ifndef VOLUME_H
#define VOLUME_H

#include <string>
#include <vector>
#include <unordered_map>
#include "exercise_definitions.h"

std::unordered_map<std::string, double> calculate_volume(const std::unordered_map<int, std::vector<Structure>>& routine, const std::unordered_map<std::string, Exercise>& exercises_map);
std::unordered_map<std::string, double> calculate_day_coverage(const std::vector<std::string>& day_structures, const std::unordered_map<std::string, std::unordered_map<std::string, double>>& exercise_contributions, int default_sets = 3);

#endif // VOLUME_H