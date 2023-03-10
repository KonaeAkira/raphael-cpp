#pragma once

#include <vector>
#include <algorithm>

struct entry {
    std::uint16_t prog, qual;
    entry(): prog(), qual() {}
    entry(const unsigned prog, const unsigned qual): prog(prog), qual(qual) {}
};

bool operator < (const entry lhs, const entry rhs) {
    return ((std::uint32_t)lhs.prog << 16 | (std::uint32_t)lhs.qual)
         < ((std::uint32_t)rhs.prog << 16 | (std::uint32_t)rhs.qual);
}

bool operator > (const entry lhs, const entry rhs) {
    return ((std::uint32_t)lhs.prog << 16 | (std::uint32_t)lhs.qual)
         > ((std::uint32_t)rhs.prog << 16 | (std::uint32_t)rhs.qual);
}

class ParetoFrontBuilder {
    std::vector<entry> entries;
    std::vector<std::pair<unsigned, unsigned>> segments;

public:
    void insert(const unsigned prog, const unsigned qual) {
        segments.emplace_back(entries.size(), entries.size() + 1);
        entries.emplace_back(prog, qual);
    }

    void insert(const std::vector<entry> &vec, const unsigned prog, const unsigned qual) {
        if (!vec.empty()) {
            segments.emplace_back(entries.size(), entries.size() + vec.size());
            for (const entry &e : vec) entries.emplace_back(e.prog + prog, e.qual + qual);
        }
    }
    
    std::vector<entry> finalize() {
        if (segments.size() <= 1) return entries;
        if (entries.size() < segments.size() * 3) { // std::sort
            std::sort(entries.begin(), entries.end(), std::greater<entry>());
        } else { // merge sort
            std::vector<entry> tmp(entries);
            for (std::size_t i = 0; i + 1 < segments.size(); i += 2) {
                unsigned l1 = segments[i].first, l2 = segments[i + 1].first, j = l1;
                const unsigned r1 = segments[i].second, r2 = segments[i + 1].second;
                if (r1 == l2) {
                    segments.emplace_back(l1, r2);
                    while (j != r2) {
                        if (l1 == r1 || (l2 != r2 && entries[l1] < entries[l2])) tmp[j++] = entries[l2++];
                        else tmp[j++] = entries[l1++];
                    }
                    if (j == entries.size()) std::swap(entries, tmp);
                } else {
                    segments.push_back(segments[i--]);
                    std::swap(entries, tmp);
                }
            }
        }
        std::vector<entry> pareto_front;
        if (!entries.empty()) {
            pareto_front.push_back(entries.front());
            for (std::size_t i = 1; i != entries.size(); ++i)
                if (entries[i].qual > pareto_front.back().qual)
                    pareto_front.push_back(entries[i]);
        }
        return pareto_front;
    }
};