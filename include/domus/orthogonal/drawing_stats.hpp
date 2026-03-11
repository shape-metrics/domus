#pragma once

#include <string>

struct OrthogonalDrawing;

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
};

OrthogonalStats compute_all_orthogonal_stats(const OrthogonalDrawing& result);

std::string orthogonal_stats_to_string(const OrthogonalStats& stats);

void print_orthogonal_stats(const OrthogonalStats& stats);