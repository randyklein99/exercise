#include "utils.h"

using namespace std;

// Utility function to join strings
string join(const vector<string>& vec, const string& delimiter) {
    string result;
    for (size_t i = 0; i < vec.size(); ++i) {
        result += vec[i];
        if (i < vec.size() - 1) result += delimiter;
    }
    return result;
}

// Function to calculate time for a set structure
double calculate_time(const vector<string>& structure, int sets) {
    if (structure.size() == 1) return sets * 120.0 / 60.0;  // Straight set
    else if (structure.size() == 2) return sets * 195.0 / 60.0;  // Superset
    else if (structure.size() == 3) return sets * 270.0 / 60.0;  // Tri-set
    return 0;
}