#include <iostream>
#include <chrono>
#include <array>
#include <vector>
#include <unordered_map>
#include <algorithm>

#include "enums.hpp"
#include "state.hpp"
#include "actions.hpp"
#include "solve.hpp"
#include "config.hpp"

int main() {
    Actions::init();
    Solver solver;

    auto t1 = std::chrono::high_resolution_clock::now();
    State init = State(Config::MAX_CP, Config::MAX_DURABILITY);
    solver.get_best_action(init, Config::MAX_PROGRESS);
    auto t2 = std::chrono::high_resolution_clock::now();
    auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);

    std::cout << "Time: " << dt.count() << "ms\n";
    solver.print_debug_info(init);
}