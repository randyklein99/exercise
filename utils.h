#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>

std::string join(const std::vector<std::string>& vec, const std::string& delimiter);
double calculate_time(const std::vector<std::string>& structure, int sets);

#endif // UTILS_H