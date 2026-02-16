#ifndef MY_DRAWING_STATS_H
#define MY_DRAWING_STATS_H

#include <string>

class OrthogonalDrawing;

int compute_total_edge_length(const OrthogonalDrawing& result);

int compute_max_edge_length(const OrthogonalDrawing& result);

double compute_edge_length_std_dev(const OrthogonalDrawing& result);

int compute_total_bends(const OrthogonalDrawing& result);

int compute_max_bends_per_edge(const OrthogonalDrawing& result);

double compute_bends_std_dev(const OrthogonalDrawing& result);

int compute_total_area(const OrthogonalDrawing& result);

int compute_total_crossings(const OrthogonalDrawing& result);

struct OrthogonalStats {
    int crossings;
    int bends;
    int area;
    int total_edge_length;
    int max_edge_length;
    double edge_length_stddev;
    int max_bends_per_edge;
    double bends_stddev;
};

OrthogonalStats compute_all_orthogonal_stats(const OrthogonalDrawing& result);

std::string orthogonal_stats_to_string(const OrthogonalStats& stats);

void print_orthogonal_stats(const OrthogonalStats& stats);

#endif
