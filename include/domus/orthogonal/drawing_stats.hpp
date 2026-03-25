#pragma once

#include <string>

namespace domus::orthogonal {
struct OrthogonalDrawing;
}

namespace domus::orthogonal::stats {

size_t compute_total_edge_length(const OrthogonalDrawing& result);

size_t compute_max_edge_length(const OrthogonalDrawing& result);

double compute_edge_length_std_dev(const OrthogonalDrawing& result);

size_t compute_total_bends(const OrthogonalDrawing& result);

size_t compute_max_bends_per_edge(const OrthogonalDrawing& result);

double compute_bends_std_dev(const OrthogonalDrawing& result);

size_t compute_total_area(const OrthogonalDrawing& result);

size_t compute_total_crossings(const OrthogonalDrawing& result);

struct OrthogonalStats {
    size_t crossings;
    size_t bends;
    size_t area;
    size_t total_edge_length;
    size_t max_edge_length;
    double edge_length_stddev;
    size_t max_bends_per_edge;
    double bends_stddev;
    std::string to_string() const;
    void print() const;
};

OrthogonalStats compute_all_orthogonal_stats(const OrthogonalDrawing& result);

} // namespace domus::orthogonal::stats