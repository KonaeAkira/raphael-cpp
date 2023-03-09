#pragma once

#include "actions.hpp"

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
};

bool operator == (const State lhs, const State rhs) {
    return lhs.cp == rhs.cp
        && lhs.durability == rhs.durability
        && lhs.effects == rhs.effects
        && lhs.condition == rhs.condition
        && lhs.last_action == rhs.last_action;
}

#include <unordered_set>
std::unordered_set<std::size_t> set;

template<> struct std::hash<State> {
    std::size_t operator()(State const& state) const noexcept {
        static const size_t BASE = 1e9 + 7;
        std::size_t params_hash = (state.cp * 256 + state.durability) * 16 + int(state.condition);
        std::size_t action_hash = int(state.last_action);
        std::size_t effect_hash = 0;
        for (int x : state.effects) effect_hash = effect_hash * 16 + x;
        // if (!set.count(effect_hash)) {
        //     set.insert(effect_hash);
        //     std::cout << '[' << set.size() << ']';
        //     for (int x : state.effects) std::cout << ' ' << x;
        //     std::cout << '\n';
        // }
        return (params_hash * BASE + action_hash) * BASE + effect_hash;
    }
};