#include <iostream>
#include <array>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <chrono>

#include "enums.hpp"
#include "state.hpp"
#include "actions.hpp"
#include "config.hpp"

typedef std::pair<float, float> data;
std::unordered_map<std::size_t, std::vector<data>> sav;

int get_cp_cost(const State state, const Action action) {
    return Actions::cp_cost[int(action)];
}

int get_durability_cost(const State state, const Action action) {
    if (state.effects[int(Effect::WasteNot)] != 0)
        return (Actions::dur_cost[int(action)] + 1) / 2;
    return Actions::dur_cost[int(action)];
}

float get_progress_potency(const State state, const Action action) {
    float condition_pim = 1.00;
    switch (state.condition) {
        case Condition::Malleable: condition_pim = 1.50; break;
        default:;
    }
    float effect_pim = 1.00;
    if (state.effects[int(Effect::MuscleMemory)] > 0) effect_pim += 1.0;
    if (state.effects[int(Effect::Veneration)] > 0) effect_pim += 0.5;
    float action_pim = Actions::pim[int(action)];
    return condition_pim * effect_pim * action_pim;
}

float get_quality_potency(const State state, const Action action) {
    float condition_qim = 1.00;
    switch (state.condition) {
        case Condition::Good: condition_qim = 1.50; break;
        case Condition::Excellent: condition_qim = 4.00; break;
        case Condition::Poor: condition_qim = 0.50; break;
        default:;
    }
    float effect_qim = 1.00 + state.effects[int(Effect::InnerQuiet)] * 0.10;
    if (state.effects[int(Effect::GreatStrides)] != 0) effect_qim += 1.0;
    if (state.effects[int(Effect::Innovation)] != 0) effect_qim += 0.5;
    float action_qim = Actions::qim[int(action)];
    if (action == Action::ByregotsBlessing) action_qim += state.effects[int(Effect::InnerQuiet)] * 0.20;
    return condition_qim * effect_qim * action_qim;
}

bool can_use_action(const State state, const Action action) {
    if (state.cp < get_cp_cost(state, action)) return false;
    if (state.durability <= 0) return false;

    if (Actions::combo_action[int(action)] != Action::Null && state.last_action != Actions::combo_action[int(action)]) return false;

    if (action == Action::PreciseTouch || action == Action::IntensiveSynthesis) {
        if (state.condition != Condition::Good && state.condition != Condition::Excellent) return false;
    } else if (action == Action::PrudentTouch || action == Action::PrudentSynthesis) {
        if (state.effects[int(Effect::WasteNot)] != 0) return false;
    } else if (action == Action::Groundwork) {
        if (state.durability < get_durability_cost(state, action)) return false;
    } else if (action == Action::TrainedFinesse) {
        if (state.effects[int(Effect::InnerQuiet)] < 10) return false;
    }

    return true;
}

bool should_use_action(const State state, const Action action) {
    if (action == Action::Groundwork || action == Action::PreparatoryTouch) {
        if (state.effects[int(Effect::WasteNot)] == 0) return false;
    } else if (action == Action::Manipulation) {
        if (state.effects[int(Effect::Manipulation)] != 0) return false;
    } else if (action == Action::WasteNot || action == Action::WasteNot2) {
        if (state.effects[int(Effect::WasteNot)] != 0) return false;
    } else if (action == Action::ByregotsBlessing) {
        if (state.effects[int(Effect::InnerQuiet)] < 6) return false;
        if (state.effects[int(Effect::Innovation)] == 0 && state.effects[int(Effect::GreatStrides)] == 0) return false;
    } else if (action == Action::GreatStrides) {
        if (state.effects[int(Effect::GreatStrides)] != 0) return false;
        if (state.effects[int(Effect::InnerQuiet)] < 6) return false;
        if (state.effects[int(Effect::Veneration)] > 3) return false;
    } else if (action == Action::Veneration) {
        if (state.effects[int(Effect::Veneration)] > 3) return false;
        if (state.effects[int(Effect::InnerQuiet)] != 0) return false;
        if (state.effects[int(Effect::GreatStrides)] != 0) return false;
        if (state.effects[int(Effect::Innovation)] > 3) return false;
    } else if (action == Action::Innovation) {
        if (state.effects[int(Effect::Innovation)] > 3) return false;
        if (state.effects[int(Effect::MuscleMemory)] != 0) return false;
        if (state.effects[int(Effect::Veneration)] > 3) return false;
    } else if (action == Action::MasterMend) {
        if (state.durability + 30 > Config::MAX_DURABILITY) return false;
    } else if (action == Action::BasicTouch) {
        if (state.last_action == Action::BasicTouch) return false;
        if (state.last_action == Action::StandardTouch) return false;
    } else if (action == Action::StandardTouch) {
        if (state.last_action == Action::StandardTouch) return false;
    }

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
        
    return true;
}

void apply_effect(State &state, const Effect effect, const int stacks) {
    state.effects[int(effect)] = (state.condition == Condition::Primed ? stacks + 2 : stacks);
}

State use_action(const State state, const Action action) {
    State new_state = state;

    new_state.cp -= get_cp_cost(state, action);
    new_state.durability = std::max(0, new_state.durability - get_durability_cost(state, action));
    new_state.last_action = (is_combo_action(action) ? action : Action::Null);

    for (int i = 0; i < int(Effect::COUNT); ++i)
        if (i != int(Effect::InnerQuiet))
            new_state.effects[i] = std::max(0, new_state.effects[i] - 1);

    if (new_state.durability > 0) {
        if (Actions::pim[int(action)] > 0.00)
            new_state.effects[int(Effect::MuscleMemory)] = 0;

        if (Actions::qim[int(action)] > 0.00) {
            if (action == Action::PreciseTouch || action == Action::PreparatoryTouch || action == Action::Reflect)
                new_state.effects[int(Effect::InnerQuiet)] += 1;
            new_state.effects[int(Effect::InnerQuiet)] = std::min(10, new_state.effects[int(Effect::InnerQuiet)] + 1);
            if (action == Action::ByregotsBlessing)
                new_state.effects[int(Effect::InnerQuiet)] = 0;
            new_state.effects[int(Effect::GreatStrides)] = 0;
        }

        if (state.effects[int(Effect::Manipulation)] > 0)
            new_state.durability = std::min(Config::MAX_DURABILITY, new_state.durability + 5);
        if (action == Action::MasterMend)
            new_state.durability = std::min(Config::MAX_DURABILITY, new_state.durability + 30);

        if (action == Action::WasteNot) apply_effect(new_state, Effect::WasteNot, 4);
        else if (action == Action::WasteNot2) apply_effect(new_state, Effect::WasteNot, 8);
        else if (action == Action::Innovation) apply_effect(new_state, Effect::Innovation, 4);
        else if (action == Action::Veneration) apply_effect(new_state, Effect::Veneration, 4);
        else if (action == Action::GreatStrides) apply_effect(new_state, Effect::GreatStrides, 3);
        else if (action == Action::MuscleMemory) apply_effect(new_state, Effect::MuscleMemory, 5);
        else if (action == Action::Manipulation) apply_effect(new_state, Effect::Manipulation, 8);
    }

    return new_state;
}

std::vector<data> &solve(const State state) {
    std::size_t hash = std::hash<State>()(state);
    if (!sav.count(hash)) {
        std::vector<data> tmp;
        for (const Action &action : ALL_ACTIONS) {
            if (!can_use_action(state, action) || !should_use_action(state, action)) continue;
            float progress = get_progress_potency(state, action);
            float quality = get_quality_potency(state, action);
            State new_state = use_action(state, action);
            if (new_state.durability > 0) {
                const std::vector<data> &new_values = solve(new_state);
                for (const data &d : new_values)
                    tmp.emplace_back(d.first + progress, d.second + quality);
            } else if (progress != 0.0) {
                tmp.emplace_back(progress, quality);
            }
        }
        std::sort(tmp.begin(), tmp.end(), std::greater<data>());
        std::vector<data> values;
        float max = -1.0;
        for (const data &d : tmp)
            if (d.second > max) {
                values.push_back(d);
                max = d.second;
            }
        sav.emplace(hash, values);
    }
    return sav[hash];
}

float get_max_quality(const State state, const float needed_progress) {
    if (state.durability > 0) {
        std::size_t hash = std::hash<State>()(state);
        auto p = std::lower_bound(sav[hash].rbegin(), sav[hash].rend(), std::make_pair(needed_progress, (float)0.0));
        if (p != sav[hash].rend()) return p->second;
    }
    return -1.0;
}

void trace(const State state, const float needed_progress) {
    if (state.durability <= 0) return;
    float best_quality = -1.0;
    float best_progress = 0.0;
    Action best_action = Action::Null;
    for (const Action &action : ALL_ACTIONS) {
        if (!can_use_action(state, action) || !should_use_action(state, action)) continue;
        float progress = get_progress_potency(state, action);
        float quality = get_quality_potency(state, action);
        State new_state = use_action(state, action);
        if (new_state.durability > 0) {
            float tmp = get_max_quality(new_state, needed_progress - progress);
            if (tmp == -1.0) continue;
            quality += tmp;
        } else {
            if (progress < needed_progress) continue;
        }
        if (quality > best_quality) {
            best_quality = quality;
            best_progress = progress;
            best_action = action;
        }
    }
    std::cout << " > " << Actions::display_name[int(best_action)];
    trace(use_action(state, best_action), needed_progress - best_progress);
}

int main() {
    Actions::init();

    State init = State(Config::MAX_CP, Config::MAX_DURABILITY);
    auto t1 = std::chrono::high_resolution_clock::now();
    solve(init);
    auto t2 = std::chrono::high_resolution_clock::now();
    auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
    std::cout << sav.size() << ' ' << sav[std::hash<State>()(init)].size() << ' ' << dt.count() << "ms" << '\n';
    for (auto [p, q] : sav[std::hash<State>()(init)]) std::cout << "(" << p << ", " << q << ") ";
    std::cout << '\n';
    std::cout << get_max_quality(init, 10);
    trace(init, 10);
    std::cout << '\n';
    return 0;
}