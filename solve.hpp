#pragma once

#include <vector>
#include <unordered_map>
#include <algorithm>
#include <iostream>
#include <cstring>

#include "enums.hpp"
#include "state.hpp"
#include "actions.hpp"
#include "pruning.hpp"
#include "config.hpp"

typedef std::vector<std::uint32_t> ParetoFront;

class Solver {
    static std::unordered_map<std::size_t, ParetoFront> sav;
    std::uint32_t n = 0, m = 0, buf[1 << 16], ind[32];

    void __merge_sort(const std::uint32_t sav_n, const std::uint32_t sav_m) {
        std::uint32_t *src = buf + sav_n, *dst = buf + n;
        for (std::uint32_t i; sav_m + 1 != m; m = sav_m + (i - sav_m) / 2) {
            ind[m] = n;
            for (i = sav_m; i < m; i += 2) { // iterate over segment indices
                if (i + 2 <= m) { // merge 2 segments
                    std::uint32_t l1 = ind[i] - sav_n, l2 = ind[i + 1] - sav_n, p = l1;
                    const std::uint32_t r1 = l2, r2 = ind[i + 2] - sav_n;
                    while (l1 != r1 && l2 != r2)
                        if (src[l1] > src[l2]) dst[p++] = src[l1++];
                        else dst[p++] = src[l2++];
                    if (l1 != r1) std::memcpy(dst + p, src + l1, (r1 - l1) * sizeof(std::uint32_t));
                    if (l2 != r2) std::memcpy(dst + p, src + l2, (r2 - l2) * sizeof(std::uint32_t));
                } else { // copy 1 segment
                    const std::uint32_t l = ind[i] - sav_n, r = ind[i + 1] - sav_n;
                    std::memcpy(dst + l, src + l, (r - l) * sizeof(std::uint32_t));
                }
                ind[sav_m + (i - sav_m) / 2] = ind[i]; // write back beginning index of merged segment
            }
            std::swap(src, dst);
        }
        if (src != buf + sav_n) // odd #iterations
            std::memcpy(buf + sav_n, buf + n, (n - sav_n) * sizeof(std::uint32_t));
    }

    void __build_pareto_front(const std::uint32_t sav_n) {
        std::uint32_t p = sav_n;
        for (std::uint32_t i = sav_n + 1; i != n; ++i)
            if ((buf[i] & 0xffff) > (buf[p] & 0xffff)) buf[++p] = buf[i];
        n = p + 1;
    }

    void __solve(const State &state, const std::uint32_t inc = 0) {
        const std::size_t hash = std::hash<State>()(state);
        if (m == 0 || ind[m - 1] != n) ind[m++] = n; // create new segment if starting position differs from prev segment

        // if already solved -> write to buf and return
        if (sav.count(hash) != 0) {
            for (const std::uint32_t x : sav.at(hash))
                buf[n++] = x + inc;
            return;
        }

        const std::uint32_t sav_n = n; // same as ind[sav_m]
        const std::uint32_t sav_m = m - 1;

        // solve all subtrees
        for (const Action action : ALL_ACTIONS) {
            if (!state.can_use_action(action) || !should_use_action(state, action)) continue;
            State new_state = state.use_action(action);
            const std::uint32_t prog = state.get_progress_potency(action);
            const std::uint32_t qual = state.get_quality_potency(action);
            if (new_state.durability != 0) __solve(new_state, prog << 16 | qual);
            else if (prog != 0) buf[n++] = prog << 16 | qual;
        }

        if (sav_m + 1 != m && ind[m - 1] == n) --m; // remove trailing segment if it is empty
        if (sav_m + 1 != m) {
            __merge_sort(sav_n, sav_m);
            __build_pareto_front(sav_n);
        }
        sav.emplace(hash, ParetoFront(buf + sav_n, buf + n));

        for (std::uint32_t i = sav_n; i != n; ++i) buf[i] += inc;
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
        __solve(state); n = 0; m = 0; // solve state and clear buffer
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