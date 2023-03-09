#include <iostream>
#include <chrono>
#include <array>
#include <vector>
#include <unordered_map>
#include <algorithm>

#include "enums.hpp"
#include "state.hpp"
#include "actions.hpp"
#include "config.hpp"

struct entry { unsigned prog: 16, qual: 16; };
bool operator < (const entry lhs, const entry rhs) {
    if (lhs.prog != rhs.prog) return lhs.prog < rhs.prog;
    else return lhs.qual < rhs.qual;
}
bool operator > (const entry lhs, const entry rhs) {
    if (lhs.prog != rhs.prog) return lhs.prog > rhs.prog;
    else return lhs.qual > rhs.qual;
}

std::unordered_map<std::size_t, std::vector<entry>> sav;

bool should_use_action(const State &state, const Action action) {
    if (state.last_action == Action::Observe) {
        if (action != Action::FocusedSynthesis && action != Action::FocusedTouch) return false;
    } else if (state.last_action == Action::None) {
        if (action != Action::MuscleMemory && action != Action::Reflect) return false;
    }

    if (state.effects[int(Effect::GreatStrides)] > 0)
        if (action != Action::ByregotsBlessing) return false;
    if (state.effects[int(Effect::InnerQuiet)] != 0)
        if (Actions::pim[int(action)] != 0.0 && Actions::qim[int(action)] == 0.0) return false;
    if (state.effects[int(Effect::MuscleMemory)] != 0)
        if (Actions::pim[int(action)] == 0.0 && Actions::qim[int(action)] != 0.0) return false;
        
    switch (action) {
        case Action::Groundwork:
        case Action::PreparatoryTouch:
            return state.effects[int(Effect::WasteNot)] != 0;
        case Action::Manipulation:
            return state.effects[int(Effect::Manipulation)] == 0;
        case Action::WasteNot:
        case Action::WasteNot2:
            return state.effects[int(Effect::WasteNot)] == 0;
        case Action::ByregotsBlessing:
            return state.effects[int(Effect::InnerQuiet)] >= 6
                && (state.effects[int(Effect::Innovation)] != 0 || state.effects[int(Effect::GreatStrides)] != 0);
        case Action::GreatStrides:
            return state.effects[int(Effect::GreatStrides)] == 0
                && state.effects[int(Effect::InnerQuiet)] >= 6
                && state.effects[int(Effect::Veneration)] <= 2;
        case Action::Veneration:
            return state.effects[int(Effect::Veneration)] <= 1
                && state.effects[int(Effect::InnerQuiet)] == 0
                && state.effects[int(Effect::GreatStrides)] == 0
                && state.effects[int(Effect::Innovation)] <= 2;
        case Action::Innovation:
            return state.effects[int(Effect::Innovation)] <= 1
                && state.effects[int(Effect::MuscleMemory)] == 0
                && state.effects[int(Effect::Veneration)] <= 2;
        case Action::MasterMend:
            return state.durability + 30 <= Config::MAX_DURABILITY;
        case Action::BasicTouch:
        case Action::StandardTouch:
            return state.last_action != Action::StandardTouch;
        default:
            return true;
    }
}

const std::vector<entry> &solve(const State &state) {
    std::size_t hash = std::hash<State>()(state);
    if (sav.contains(hash) != 0) return sav.at(hash);

    std::vector<entry> tmp;
    for (const Action &action : ALL_ACTIONS) {
        if (!state.can_use_action(action) || !should_use_action(state, action)) continue;
        const unsigned prog = state.get_progress_potency(action);
        const unsigned qual = state.get_quality_potency(action);
        State new_state = state.use_action(action);
        if (new_state.durability > 0) { // only solve non-final states
            for (const auto &[x, y] : solve(new_state))
                tmp.emplace_back(x + prog, y + qual);
        }
        else if (prog != 0.0) // last step must be progress-increase
            tmp.emplace_back(prog, qual);
    }

    std::sort(tmp.begin(), tmp.end(), std::greater<entry>());
    std::vector<entry> front;
    unsigned max = (unsigned)-1;
    for (const auto &[x, y] : tmp)
        if (max == (unsigned)-1 || y > max) {
            max = y;
            front.emplace_back(x, y);
        }

    return (*(sav.emplace(hash, std::move(front)).first)).second;
}

unsigned get_max_quality(const State state, const unsigned needed_progress) {
    if (state.durability > 0) {
        const std::vector<entry> &entries = sav[std::hash<State>()(state)];
        const entry key = {needed_progress, 0};
        auto iter = std::lower_bound(entries.rbegin(), entries.rend(), key);
        if (iter != entries.rend()) return iter->qual;
    }
    return 0;
}

void trace(const State state, const unsigned needed_progress) {
    if (state.durability <= 0) return;
    unsigned best_prog = 0, best_qual = 0;
    Action best_action = Action::Null;
    for (const Action &action : ALL_ACTIONS) {
        if (!state.can_use_action(action) || !should_use_action(state, action)) continue;
        unsigned prog = state.get_progress_potency(action);
        unsigned qual = state.get_quality_potency(action);
        State new_state = state.use_action(action);
        if (new_state.durability > 0) {
            qual += get_max_quality(new_state, needed_progress - prog);
        } else {
            if (prog < needed_progress) continue;
        }
        if (qual > best_qual) {
            best_qual = qual;
            best_prog = prog;
            best_action = action;
        }
    }
    std::cout << " > " << Actions::display_name[int(best_action)];
    trace(state.use_action(best_action), needed_progress - best_prog);
}

int main() {
    Actions::init();
    State init = State(Config::MAX_CP, Config::MAX_DURABILITY);
    std::size_t hash = std::hash<State>()(init);

    auto t1 = std::chrono::high_resolution_clock::now();
    solve(init);
    auto t2 = std::chrono::high_resolution_clock::now();
    auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);

    std::cout << sav.size() << ' ' << sav[std::hash<State>()(init)].size() << ' ' << dt.count() << "ms" << '\n';
    for (auto [p, q] : sav[hash]) std::cout << "(" << p << ", " << q << ") ";
    std::cout << '\n';
    std::cout << get_max_quality(init, Config::MAX_PROGRESS);
    trace(init, Config::MAX_PROGRESS);
    std::cout << '\n';
}