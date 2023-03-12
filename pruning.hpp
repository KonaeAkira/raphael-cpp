#pragma once

#include "state.hpp"
#include "actions.hpp"

bool should_use_action(const State &state, const Action action) {
    if (state.last_action == Action::Observe) {
        if (action != Action::FocusedSynthesis && action != Action::FocusedTouch) return false;
    } else if (state.last_action == Action::None) {
        if (action != Action::MuscleMemory && action != Action::Reflect) return false;
    }

    if (state.effects[int(Effect::MuscleMemory)] != 0)
        if (Actions::pim[int(action)] == 0 && Actions::qim[int(action)] != 0) return false;
    if (state.effects[int(Effect::GreatStrides)] != 0)
        if (action != Action::ByregotsBlessing) return false;
    if (state.effects[int(Effect::InnerQuiet)] != 0)
        if (Actions::pim[int(action)] != 0 && Actions::qim[int(action)] == 0) return false;
        
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