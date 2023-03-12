#pragma once

#include <iostream>

#include "actions.hpp"
#include "config.hpp"

struct State {
    int cp, durability;
    std::array<int, int(Effect::COUNT)> effects;
    Condition condition;
    Action last_action;

    State(int cp, int durability):
        cp(cp),
        durability(durability),
        effects(),
        condition(Condition::Normal),
        last_action(Action::None)
    {}

    int get_cp_cost(const Action action) const {
        return Actions::cp_cost[int(action)];
    }

    int get_durability_cost(const Action action) const {
        if (effects[int(Effect::WasteNot)] != 0)
            return (Actions::dur_cost[int(action)] + 1) / 2;
        return Actions::dur_cost[int(action)];
    }

    unsigned get_progress_potency(const Action action) const {
        float condition_pim = 1.00;
        switch (condition) {
            case Condition::Malleable: condition_pim = 1.50; break;
            default:;
        }
        float effect_pim = 1.00;
        if (effects[int(Effect::MuscleMemory)] > 0) effect_pim += 1.0;
        if (effects[int(Effect::Veneration)] > 0) effect_pim += 0.5;
        float action_pim = Actions::pim[int(action)];
        return (unsigned)(condition_pim * effect_pim * action_pim);
    }

    unsigned get_quality_potency(const Action action) const {
        float condition_qim = 1.00;
        switch (condition) {
            case Condition::Good: condition_qim = 1.50; break;
            case Condition::Excellent: condition_qim = 4.00; break;
            case Condition::Poor: condition_qim = 0.50; break;
            default:;
        }
        float effect_qim = 1.00 + effects[int(Effect::InnerQuiet)] * 0.10;
        if (effects[int(Effect::GreatStrides)] != 0) effect_qim += 1.0;
        if (effects[int(Effect::Innovation)] != 0) effect_qim += 0.5;
        float action_qim = Actions::qim[int(action)];
        if (action == Action::ByregotsBlessing) action_qim += Config::BASE_QUALITY_MULTIPLIER * effects[int(Effect::InnerQuiet)] * 0.20;
        return (unsigned)(condition_qim * effect_qim * action_qim);
    }

    bool can_use_action(const Action action) const {
        if (cp < get_cp_cost(action)) return false;
        if (durability <= 0) return false;

        if (Actions::combo_action[int(action)] != Action::Null && last_action != Actions::combo_action[int(action)]) return false;

        switch (action) {
            case Action::PreciseTouch:
            case Action::IntensiveSynthesis:
                return condition == Condition::Good || condition == Condition::Excellent;
            case Action::PrudentTouch:
            case Action::PrudentSynthesis:
                return effects[int(Effect::WasteNot)] == 0;
            case Action::Groundwork:
                return durability >= get_durability_cost(action);
            case Action::TrainedFinesse:
                return effects[int(Effect::InnerQuiet)] == 10;
            default:
                return true;
        }
    }

    void apply_effect(const Effect effect, const int stacks) {
        effects[int(effect)] = (condition == Condition::Primed ? stacks + 2 : stacks);
    }

    State use_action(const Action action) const {
        State new_state = *this;

        new_state.cp -= get_cp_cost(action);
        new_state.durability = std::max(0, new_state.durability - get_durability_cost(action));
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

            if (effects[int(Effect::Manipulation)] > 0)
                new_state.durability = std::min(Config::MAX_DURABILITY, new_state.durability + 5);
            if (action == Action::MasterMend)
                new_state.durability = std::min(Config::MAX_DURABILITY, new_state.durability + 30);

            if (action == Action::WasteNot) new_state.apply_effect(Effect::WasteNot, 4);
            else if (action == Action::WasteNot2) new_state.apply_effect(Effect::WasteNot, 8);
            else if (action == Action::Innovation) new_state.apply_effect(Effect::Innovation, 4);
            else if (action == Action::Veneration) new_state.apply_effect(Effect::Veneration, 4);
            else if (action == Action::GreatStrides) new_state.apply_effect(Effect::GreatStrides, 3);
            else if (action == Action::MuscleMemory) new_state.apply_effect(Effect::MuscleMemory, 5);
            else if (action == Action::Manipulation) new_state.apply_effect(Effect::Manipulation, 8);
        }

        return new_state;
    }
};

template<> struct std::hash<State> {
    std::size_t operator()(State const& state) const noexcept {
        static const size_t BASE = 1e9 + 7;
        std::size_t params_hash = (state.cp * 256 + state.durability) * 16 + int(state.condition);
        std::size_t action_hash = int(state.last_action);
        std::size_t effect_hash = 0;
        for (int x : state.effects) effect_hash = effect_hash * 16 + x;
        return (params_hash * BASE + action_hash) * BASE + effect_hash;
    }
};

bool operator == (const State lhs, const State rhs) {
    return lhs.cp == rhs.cp
        && lhs.durability == rhs.durability
        && lhs.effects == rhs.effects
        && lhs.condition == rhs.condition
        && lhs.last_action == rhs.last_action;
}

std::ostream &operator << (std::ostream &os, const State &state) { 
    const std::size_t hash = std::hash<State>()(state);
    os << std::hex << "([" << hash << "]" << std::dec;
    os << " CP: " << state.cp << " Dur: " << state.durability << ")";
    return os;
}