#pragma once

#include <vector>
#include <unordered_map>
#include <algorithm>
#include <iostream>
#include <cstring>

#include "enums.hpp"
#include "state.hpp"
#include "actions.hpp"
#include "config.hpp"

typedef std::vector<std::uint32_t> ParetoFront;

class Solver {
    static std::unordered_map<std::size_t, ParetoFront> sav;
    std::uint32_t n = 0, buf[1 << 16];

    static bool should_use_action(const State &state, const Action action) {
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

    void push_root(std::vector<std::pair<std::uint32_t, std::uint32_t>> &heap) const {
        std::uint32_t i = 0, max = 0;
    push_down:
        if (i * 2 + 1 < heap.size() && buf[heap[max].first] < buf[heap[i * 2 + 1].first])
            max = i * 2 + 1;
        if (i * 2 + 2 < heap.size() && buf[heap[max].first] < buf[heap[i * 2 + 2].first])
            max = i * 2 + 2;
        if (max != i) {
            std::swap(heap[i], heap[max]);
            i = max;
            goto push_down;
        }
    }

    void push_leaf(std::vector<std::pair<std::uint32_t, std::uint32_t>> &heap) const {
        std::uint32_t i = heap.size() - 1;
        while (i != 0 && buf[heap[(i - 1) >> 1].first] < buf[heap[i].first]) {
            std::swap(heap[(i - 1) >> 1], heap[i]);
            i = (i - 1) >> 1;
        }
    }

    ParetoFront __use_builtin_sort(const std::uint32_t s) {
        std::sort(buf + s, buf + n, std::greater<std::uint32_t>());
        std::uint32_t p = s;
        for (std::uint32_t i = s + 1; i != n; ++i)
            if ((buf[i] & 0xffff) > (buf[p] & 0xffff)) buf[++p] = buf[i];
        n = p + 1;
        return ParetoFront(buf + s, buf + n);
    }

    ParetoFront __use_custom_sort(const std::uint32_t s, std::vector<std::pair<std::uint32_t, std::uint32_t>> &heap) {
        ParetoFront pareto_front;
        pareto_front.push_back(buf[heap.front().first]);
        do {
            if ((buf[heap.front().first] & 0xffff) > (pareto_front.back() & 0xffff)) {
                pareto_front.push_back(buf[heap.front().first++]);
                if (heap.front().first == heap.front().second) {
                    heap.front() = heap.back();
                    heap.pop_back();
                }
            } else {
                do {
                    heap.front().first += 1;
                    if (heap.front().first == heap.front().second) {
                        heap.front() = heap.back();
                        heap.pop_back();
                        break;
                    }
                } while ((buf[heap.front().first] & 0xffff) <= (pareto_front.back() & 0xffff));
            }
            push_root(heap);
        } while (!heap.empty());
        n = s + pareto_front.size();
        std::memcpy(buf + s, &pareto_front.front(), pareto_front.size() * sizeof(std::uint32_t));
        return pareto_front;
    }

    void solve(const State &state, const std::uint32_t inc = 0) {
        const std::size_t hash = std::hash<State>()(state);
        if (sav.count(hash) != 0) {
            for (const std::uint32_t x : sav.at(hash))
                buf[n++] = x + inc;
            return;
        }

        const std::uint32_t s = n;
        std::vector<std::pair<std::uint32_t, std::uint32_t>> heap;

        for (const Action action : ALL_ACTIONS) {
            if (!state.can_use_action(action) || !should_use_action(state, action)) continue;
            State new_state = state.use_action(action);
            const std::uint32_t prog = state.get_progress_potency(action);
            const std::uint32_t qual = state.get_quality_potency(action);
            if (new_state.durability != 0) {
                const std::uint32_t t = n;
                solve(new_state, prog << 16 | qual);
                if (t != n) {
                    heap.emplace_back(t, n);
                    push_leaf(heap);
                }
            } else if (prog != 0) {
                buf[n++] = prog << 16 | qual;
                heap.emplace_back(n - 1, n);
                push_leaf(heap);
            }
        }

        if (s == n) sav.emplace(hash, ParetoFront());
        else if (n - s < heap.size() * 3) sav.emplace(hash, __use_builtin_sort(s));
        else sav.emplace(hash, __use_custom_sort(s, heap));

        for (std::uint32_t i = s; i != n; ++i) buf[i] += inc;
    }

public:
    static std::uint32_t get_max_quality(const State state, const std::uint32_t min_prog) {
        if (state.durability != 0) {
            const ParetoFront &entries = sav[std::hash<State>()(state)];
            auto iter = std::lower_bound(entries.rbegin(), entries.rend(), min_prog << 16);
            if (iter != entries.rend()) return *iter & 0xffff;
        }
        return 0;
    }

    Action get_best_action(const State state, const std::uint32_t min_prog) {
        if (state.durability == 0) return Action::Null;
        solve(state); n = 0; // solve state and clear buffer
        std::uint32_t best_qual = 0;
        Action best_action = Action::Null;
        for (const Action action : ALL_ACTIONS) {
            if (!state.can_use_action(action) || !should_use_action(state, action)) continue;
            std::uint32_t prog = state.get_progress_potency(action);
            std::uint32_t qual = state.get_quality_potency(action);
            State new_state = state.use_action(action);
            if (new_state.durability != 0) qual += get_max_quality(new_state, min_prog - prog);
            else if (prog < min_prog) continue;
            if (qual > best_qual) { best_qual = qual; best_action = action; }
        }
        return best_action;
    }

    void print_debug_info(const State init) {
        std::cout << "Unique states: " << sav.size() << ' ';
        std::cout << "Initial state size: " << sav.at(std::hash<State>()(init)).size() << '\n';
        std::cout << get_max_quality(init, Config::MAX_PROGRESS);

        State cur_state = init;
        std::uint32_t min_prog = Config::MAX_PROGRESS;
        while (cur_state.durability > 0) {
            Action best_action = get_best_action(cur_state, min_prog);
            const std::uint32_t prog = cur_state.get_progress_potency(best_action);
            min_prog = prog > min_prog ? 0 : min_prog - prog;
            cur_state = cur_state.use_action(best_action);
            std::cout << " >> " << Actions::display_name[int(best_action)];
            if (best_action == Action::Null) {
                std::cout << "\nUnexpected error occured";
                break;
            }
        }
        std::cout << '\n';
    }
};

std::unordered_map<std::size_t, ParetoFront> Solver::sav = std::unordered_map<std::size_t, ParetoFront>();