#pragma once

#include "enums.hpp"
#include "config.hpp"

namespace Actions {
    std::string display_name[int(Action::COUNT)];
    int cp_cost[int(Action::COUNT)], dur_cost[int(Action::COUNT)];
    unsigned pim[int(Action::COUNT)], qim[int(Action::COUNT)];
    Action combo_action[int(Action::COUNT)];

    void set_parameters(const Action action, std::string _display_name, int _cp_cost, int _dur_cost, float _pim, float _qim, Action _combo_action) {
        display_name[int(action)] = _display_name;
        cp_cost[int(action)] = _cp_cost;
        dur_cost[int(action)] = _dur_cost;
        pim[int(action)] = _pim * Config::BASE_PROGRESS_MULTIPLIER;
        qim[int(action)] = _qim * Config::BASE_QUALITY_MULTIPLIER;
        combo_action[int(action)] = _combo_action;
    }

    void init() {
        set_parameters(Action::Null, "Null", 0, 0, 0.00, 0.00, Action::Null);
        set_parameters(Action::None, "None", 0, 0, 0.00, 0.00, Action::Null);
        set_parameters(Action::BasicSynthesis, "Basic Synthesis", 0, 10, 1.20, 0.00, Action::Null);
        set_parameters(Action::BasicTouch, "Basic Touch", 18, 10, 0.00, 1.00, Action::Null);
        set_parameters(Action::MasterMend, "Master's Mend", 88, 0, 0.00, 0.00, Action::Null);
        // Hasty Touch
        // Rapid Synthesis
        set_parameters(Action::Observe, "Observe", 7, 0, 0.00, 0.00, Action::Null);
        // Tricks of the Trade
        set_parameters(Action::WasteNot, "Waste Not", 56, 0, 0.00, 0.00, Action::Null);
        set_parameters(Action::Veneration, "Veneration", 18, 0, 0.00, 0.00, Action::Null);
        set_parameters(Action::StandardTouch, "Standard Touch", 18, 10, 0.00, 1.25, Action::BasicTouch);
        set_parameters(Action::GreatStrides, "Great Strides", 32, 0, 0.00, 0.00, Action::Null);
        set_parameters(Action::Innovation, "Innovation", 18, 0, 0.00, 0.00, Action::Null);
        // Final Appraisal
        set_parameters(Action::WasteNot2, "Waste Not II", 98, 0, 0.00, 0.00, Action::Null);
        set_parameters(Action::ByregotsBlessing, "Byregot's Blessing", 24, 10, 0.00, 1.00, Action::Null);
        set_parameters(Action::PreciseTouch, "Precise Touch", 18, 10, 0.00, 1.50, Action::Null);
        set_parameters(Action::MuscleMemory, "Muscle Memory", 6, 10, 3.00, 0.00, Action::None);
        set_parameters(Action::CarefulSynthesis, "Careful Synthesis", 7, 10, 1.80, 0.00, Action::Null);
        set_parameters(Action::Manipulation, "Manipulation", 96, 0, 0.00, 0.00, Action::Null);
        set_parameters(Action::PrudentTouch, "Prudent Touch", 25, 5, 0.00, 1.00, Action::Null);
        set_parameters(Action::FocusedSynthesis, "Focused Synthesis", 5, 10, 2.00, 0.00, Action::Observe);
        set_parameters(Action::FocusedTouch, "Focused Touch", 18, 10, 0.00, 1.50, Action::Observe);
        set_parameters(Action::Reflect, "Reflect", 6, 10, 0.00, 1.00, Action::None);
        set_parameters(Action::PreparatoryTouch, "Preparatory Touch", 40, 20, 0.00, 2.00, Action::Null);
        set_parameters(Action::Groundwork, "Groundwork", 18, 20, 3.60, 0.00, Action::Null);
        set_parameters(Action::DelicateSynthesis, "Delicate Synthesis", 32, 10, 1.00, 1.00, Action::Null);
        set_parameters(Action::IntensiveSynthesis, "Intensive Synthesis", 6, 10, 4.00, 1.00, Action::Null);
        // Trained Eye
        set_parameters(Action::AdvancedTouch, "Advanced Touch", 18, 10, 0.00, 1.50, Action::StandardTouch);
        set_parameters(Action::PrudentSynthesis, "Prudent Synthesis", 18, 5, 1.80, 0.00, Action::Null);
        set_parameters(Action::TrainedFinesse, "Trained Finesse", 32, 0, 0.00, 1.00, Action::Null);
    }
}

const Action ALL_ACTIONS[] = {
        Action::BasicSynthesis,
        Action::BasicTouch,
        Action::MasterMend,
        Action::Observe,
        Action::WasteNot,
        Action::Veneration,
        Action::StandardTouch,
        Action::GreatStrides,
        Action::Innovation,
        Action::WasteNot2,
        Action::ByregotsBlessing,
        Action::PreciseTouch,
        Action::MuscleMemory,
        Action::CarefulSynthesis,
        Action::Manipulation,
        Action::PrudentTouch,
        Action::FocusedSynthesis,
        Action::FocusedTouch,
        Action::Reflect,
        Action::PreparatoryTouch,
        Action::Groundwork,
        Action::DelicateSynthesis,
        Action::IntensiveSynthesis,
        Action::AdvancedTouch,
        Action::PrudentSynthesis,
        Action::TrainedFinesse
    };

bool is_combo_action(const Action action) {
    return action == Action::Observe || action == Action::BasicTouch || action == Action::StandardTouch;
}