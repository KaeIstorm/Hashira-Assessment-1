#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <utility>
#include <functional> // For std::function
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
 * @brief Evaluates a polynomial defined by a set of points at a specific x-coordinate.
 *
 * Uses Lagrange Interpolation to find P(x_to_evaluate) without explicitly
 * finding the polynomial coefficients. Uses a common denominator to maintain precision
 * with BigInt arithmetic.
 *
 * @param points The vector of (x, y) pairs defining the polynomial.
 * @param x_to_evaluate The x-coordinate at which to evaluate the polynomial.
 * @return The value of the polynomial at x_to_evaluate.
 */
BigInt lagrange_evaluate(const std::vector<std::pair<long long, BigInt>>& points, long long x_to_evaluate) {
    BigInt final_numerator = 0;
    BigInt common_denominator = 1;

    for (const auto& p_j : points) {
        BigInt num = p_j.second; // This is y_j
        BigInt den = 1;
        for (const auto& p_i : points) {
            if (p_i.first == p_j.first) continue; // Skip if i == j
            num *= (x_to_evaluate - p_i.first);
            den *= (p_j.first - p_i.first);
        }
        final_numerator = final_numerator * den + num * common_denominator;
        common_denominator *= den;
    }

    if (common_denominator == 0) {
        // This should not happen with valid inputs (distinct x-coordinates in the subset)
        throw std::runtime_error("Division by zero in Lagrange evaluation. Check for duplicate x-coords in a combination.");
    }
    return final_numerator / common_denominator;
}


/**
 * @brief Processes a single JSON file to find the polynomial's constant term.
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

    size_t n = all_points.size();
    if (n < k) {
        std::cerr << "Error: Not enough points in file. Have " << n << ", need " << k << "." << std::endl << std::endl;
        return;
    }

    // 2. Iterate through all combinations of k points to find the best-fit polynomial
    int best_inlier_count = -1;
    BigInt final_answer = 0;
    std::vector<int> combination_indices;

    // A recursive lambda function to generate combinations
    std::function<void(int, int)> generate_combinations = 
        [&](int offset, int count_needed) {
        
        // Base case: a combination is formed
        if (count_needed == 0) {
            std::vector<std::pair<long long, BigInt>> subset;
            for (int index : combination_indices) {
                subset.push_back(all_points[index]);
            }

            // Test this subset's polynomial against all points
            int current_inlier_count = 0;
            for (const auto& p : all_points) {
                if (lagrange_evaluate(subset, p.first) == p.second) {
                    current_inlier_count++;
                }
            }
            
            // If this is the best fit so far, store its constant term
            if (current_inlier_count > best_inlier_count) {
                best_inlier_count = current_inlier_count;
                final_answer = lagrange_evaluate(subset, 0); // Calculate P(0)
            }
            return;
        }
        
        // Recursive step
        for (int i = offset; i <= n - count_needed; ++i) {
            combination_indices.push_back(i);
            generate_combinations(i + 1, count_needed - 1);
            combination_indices.pop_back(); // backtrack
        }
    };

    std::cout << "Searching for best polynomial fit among " << n << " points (k=" << k << ")..." << std::endl;
    generate_combinations(0, k);

    std::cout << "Found a polynomial that fits " << best_inlier_count << " of " << n << " points." << std::endl;
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