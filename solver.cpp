#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <utility>
#include "nlohmann/json.hpp"
#include "BigInt.hpp"

// Use the nlohmann json namespace for convenience
using json = nlohmann::json;

/**
 * @brief Converts a number string from a given base to its BigInt representation.
 */
BigInt convertToBase10(const std::string& numStr, int base) {
    BigInt result = 0;
    BigInt power = 1;
    BigInt big_base = base;

    for (int i = numStr.length() - 1; i >= 0; i--) {
        int digit;
        if (numStr[i] >= '0' && numStr[i] <= '9') {
            digit = numStr[i] - '0';
        } else if (numStr[i] >= 'a' && numStr[i] <= 'z') {
            digit = numStr[i] - 'a' + 10;
        } else { // A-Z
            digit = numStr[i] - 'A' + 10;
        }
        result += power * digit;
        power *= big_base;
    }
    return result;
}

/**
 * @brief Calculates the polynomial's constant term P(0) using Lagrange Interpolation.
 *
 * @param points The vector of (x, y) pairs defining the polynomial.
 * @return The value of the polynomial at x=0.
 */
BigInt lagrange_interpolate_at_zero(const std::vector<std::pair<long long, BigInt>>& points) {
    BigInt final_result = 0;
    long long x_to_evaluate = 0;

    for (const auto& p_j : points) { // for each point j
        BigInt term_numerator = p_j.second; // y_j
        BigInt term_denominator = 1;
        
        // Calculate the Lagrange basis polynomial L_j(0)
        for (const auto& p_i : points) {
            if (p_i.first == p_j.first) continue; // Skip if i == j
            term_numerator *= (x_to_evaluate - p_i.first);
            term_denominator *= (p_j.first - p_i.first);
        }
        
        if (term_denominator == 0) {
            throw std::runtime_error("Division by zero in Lagrange basis. Check for duplicate x-coordinates.");
        }
        
        final_result += term_numerator / term_denominator;
    }

    return final_result;
}


/**
 * @brief Processes a single JSON file.
 */
void processFile(const char* filename) {
    std::cout << "===== Processing file: " << filename << " =====" << std::endl;

    std::ifstream json_file(filename);
    if (!json_file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl << std::endl;
        return;
    }
    json data;
    try {
        data = json::parse(json_file);
    } catch (json::parse_error& e) {
        std::cerr << "JSON parsing error: " << e.what() << std::endl << std::endl;
        return;
    }

    // 1. Read ALL points from the JSON into a vector
    size_t k = data["keys"]["k"];
    std::vector<std::pair<long long, BigInt>> all_points;
    for (auto const& [key, val] : data.items()) {
        if (key == "keys") continue;
        all_points.push_back({std::stoll(key), convertToBase10(val["value"].get<std::string>(), std::stoi(val["base"].get<std::string>()))});
    }

    if (all_points.size() < k) {
        std::cerr << "Error: Not enough points in file. Have " << all_points.size() << ", need " << k << "." << std::endl << std::endl;
        return;
    }

    // 2. Sort all points by their x-coordinate
    std::sort(all_points.begin(), all_points.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
    });

    // 3. Select the first k points (those with the smallest x-values) for the calculation
    std::vector<std::pair<long long, BigInt>> points_for_calc(all_points.begin(), all_points.begin() + k);
    
    std::cout << "Using the " << k << " points with the smallest x-values for calculation." << std::endl;

    // 4. Calculate the final answer
    BigInt final_answer = lagrange_interpolate_at_zero(points_for_calc);

    std::cout << "\n-----------------------------------------" << std::endl;
    std::cout << "Calculated constant term P(0) = " << final_answer << std::endl;
    std::cout << "-----------------------------------------" << std::endl << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <file1.json> <file2.json> ..." << std::endl;
        return 1;
    }

    for (int i = 1; i < argc; ++i) {
        processFile(argv[i]);
    }

    return 0;
}

