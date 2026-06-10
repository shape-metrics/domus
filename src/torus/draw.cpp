#include "draw.hpp"

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include "domus/drawing/polygon.hpp"
#include "domus/drawing/svg_drawer.hpp"

namespace domus::torus {
using namespace domus::graph;
using namespace domus::drawing;

const std::vector<Point2D>
get_hexagon_vertices(double h, double k, double r, bool flat_topped = false);

/*
    CONSTANTS
*/

const double PI = std::acos(-1.0);
constexpr double CENTER_X = 540.0;
constexpr double CENTER_Y = 360.0;
constexpr double HEXAGON_RADIUS = 200.0;
constexpr double SQUARE_SIDE = 300.0;
const std::array<Point2D, 4> SQUARE_VERTICES = {{
    {CENTER_X - SQUARE_SIDE/2.0, CENTER_Y + SQUARE_SIDE/2.0},
    {CENTER_X + SQUARE_SIDE/2.0, CENTER_Y + SQUARE_SIDE/2.0},
    {CENTER_X + SQUARE_SIDE/2.0, CENTER_Y - SQUARE_SIDE/2.0},
    {CENTER_X - SQUARE_SIDE/2.0, CENTER_Y - SQUARE_SIDE/2.0}
    
}};
const std::vector<Point2D> HEXAGON_VERTICES =
    get_hexagon_vertices(CENTER_X, CENTER_Y, HEXAGON_RADIUS, false);
constexpr std::array<std::pair<size_t, size_t>, 3> HEXAGON_REPEATED_PATH_ENDPOINTS_0{
    {{0, 1}, {2, 3}, {4, 5}}
};
constexpr std::array<std::pair<size_t, size_t>, 3> HEXAGON_REPEATED_PATH_ENDPOINTS_1{
    {{4, 3}, {0, 5}, {2, 1}}
};
constexpr std::array<std::pair<size_t, size_t>, 3> SQUARE_REPEATED_PATH_ENDPOINTS_0{
    {{0, 1}, {1, 2}}
};
constexpr std::array<std::pair<size_t, size_t>, 3> SQUARE_REPEATED_PATH_ENDPOINTS_1{
    {{3, 2}, {0, 3}}
};
constexpr std::array<const char*, 3> REPEATED_PATH_INDEX_TO_COLOR{{"red", "green", "blue"}};
constexpr const char* INNER_PATH_COLOR = "black";
constexpr const char* NODE_COLOR = "black";
constexpr double NODE_RADIUS = 10.0;
constexpr size_t SVG_WIDTH = 1080;
constexpr size_t SVG_HEIGHT = 720;

// Function to calculate hexagon vertices
// flat_topped = true: Flat top and bottom (vertices on Y-axis if centered)
// flat_topped = false: Pointy top and bottom (vertices on X-axis if centered)
const std::vector<Point2D> get_hexagon_vertices(double h, double k, double r, bool flat_topped) {
    std::vector<Point2D> vertices;
    vertices.reserve(6);

    // Starting angle changes based on orientation:
    // 0 radians (0 degrees) for pointy-topped
    // PI / 6 radians (30 degrees) for flat-topped
    double start_angle = flat_topped ? (PI / 6.0) : 0.0;

    for (int i = 0; i < 6; ++i) {
        double angle = start_angle - i * (PI / 3.0); // Vertices are separated by 60 degrees
        vertices.push_back(Point2D(h - r * std::cos(angle), k - r * std::sin(angle)));
    }

    return vertices;
}

void draw_nodes_of_path(
    SvgDrawer& drawer, Point2D first_position, Point2D last_position, const Path& path
) {
    const size_t num_nodes = path.number_of_nodes();
    for (size_t k = 0; k < num_nodes; ++k) {
        const double fraction = static_cast<double>(k) / static_cast<double>(num_nodes - 1);
        const double x = first_position.x + fraction * (last_position.x - first_position.x);
        const double y = first_position.y + fraction * (last_position.y - first_position.y);
        Circle2D circle(Point2D(x, y), NODE_RADIUS);

        drawer.add(circle, NODE_COLOR);
        drawer.add(std::to_string(path.node_id_at_position(k)), circle.get_center());
    }
}

void draw_the_hexagon_border(SvgDrawer& drawer, const Face& original_face) {
    for (size_t i = 0; i < 3; ++i) {
        drawer.add(
            Line2D(
                HEXAGON_VERTICES[HEXAGON_REPEATED_PATH_ENDPOINTS_0[i].first],
                HEXAGON_VERTICES[HEXAGON_REPEATED_PATH_ENDPOINTS_0[i].second]
            ),
            REPEATED_PATH_INDEX_TO_COLOR[i]
        );
        drawer.add(
            Line2D(
                HEXAGON_VERTICES[HEXAGON_REPEATED_PATH_ENDPOINTS_1[i].first],
                HEXAGON_VERTICES[HEXAGON_REPEATED_PATH_ENDPOINTS_1[i].second]
            ),
            REPEATED_PATH_INDEX_TO_COLOR[i]
        );
    }
    for (size_t i = 0; i < 3; ++i) {
        draw_nodes_of_path(
            drawer,
            HEXAGON_VERTICES[HEXAGON_REPEATED_PATH_ENDPOINTS_0[i].first],
            HEXAGON_VERTICES[HEXAGON_REPEATED_PATH_ENDPOINTS_0[i].second],
            original_face.repeated_paths()[i]
        );
        draw_nodes_of_path(
            drawer,
            HEXAGON_VERTICES[HEXAGON_REPEATED_PATH_ENDPOINTS_1[i].first],
            HEXAGON_VERTICES[HEXAGON_REPEATED_PATH_ENDPOINTS_1[i].second],
            original_face.repeated_paths()[i]
        );
    }
}

void draw_the_square_border(SvgDrawer& drawer, const Face& original_face) {
    for (size_t i = 0; i < original_face.repeated_paths().size(); ++i) {
        drawer.add(
            Line2D(
                SQUARE_VERTICES[SQUARE_REPEATED_PATH_ENDPOINTS_0[i].first],
                SQUARE_VERTICES[SQUARE_REPEATED_PATH_ENDPOINTS_0[i].second]
            ),
            REPEATED_PATH_INDEX_TO_COLOR[i]
        );
        drawer.add(
            Line2D(
                SQUARE_VERTICES[SQUARE_REPEATED_PATH_ENDPOINTS_1[i].first],
                SQUARE_VERTICES[SQUARE_REPEATED_PATH_ENDPOINTS_1[i].second]
            ),
            REPEATED_PATH_INDEX_TO_COLOR[i]
        );
    }
    for (size_t i = 0; i < original_face.repeated_paths().size(); ++i) {
        draw_nodes_of_path(
            drawer,
            SQUARE_VERTICES[SQUARE_REPEATED_PATH_ENDPOINTS_0[i].first],
            SQUARE_VERTICES[SQUARE_REPEATED_PATH_ENDPOINTS_0[i].second],
            original_face.repeated_paths()[i]
        );
        draw_nodes_of_path(
            drawer,
            SQUARE_VERTICES[SQUARE_REPEATED_PATH_ENDPOINTS_1[i].first],
            SQUARE_VERTICES[SQUARE_REPEATED_PATH_ENDPOINTS_1[i].second],
            original_face.repeated_paths()[i]
        );
    }
}

void draw_path_inside(
    SvgDrawer& drawer,
    const Path& path,
    const Embedding& embedding,
    const Face& original_face,
    const Point2D* positions,
    const std::pair<size_t, size_t>* indexes_0,
    const std::pair<size_t, size_t>* indexes_1
) {
    auto find_position = [&](const size_t node_id_to_find, const size_t corresponding_edge_id) {
        for (size_t j = 0; j < original_face.repeated_paths().size(); ++j) {
            const Path& repeated_path = original_face.repeated_paths()[j];

            const size_t first_node_id = repeated_path.get_first_node_id();
            const size_t first_edge_id = repeated_path.get_first_edge_id();
            if (first_node_id == node_id_to_find) {
                if (embedding
                        .next_in_adjacency_list(
                            first_node_id,
                            repeated_path.node_id_at_position(1),
                            first_edge_id
                        )
                        .id == corresponding_edge_id) {
                    const double x = positions[indexes_0[j].first].x;
                    const double y = positions[indexes_0[j].first].y;
                    return Point2D(x, y);
                }
                if (embedding
                        .prev_in_adjacency_list(
                            first_node_id,
                            repeated_path.node_id_at_position(1),
                            first_edge_id
                        )
                        .id == corresponding_edge_id) {
                    const double x = positions[indexes_1[j].first].x;
                    const double y = positions[indexes_1[j].first].y;
                    return Point2D(x, y);
                }
            }

            const size_t last_node_id = repeated_path.get_last_node_id();
            const size_t last_edge_id = repeated_path.get_last_edge_id();
            if (last_node_id == node_id_to_find) {
                if (embedding
                        .prev_in_adjacency_list(
                            last_node_id,
                            repeated_path.node_id_at_position(repeated_path.number_of_nodes() - 2),
                            last_edge_id
                        )
                        .id == corresponding_edge_id) {
                    const double x = positions[indexes_0[j].second].x;
                    const double y = positions[indexes_0[j].second].y;
                    return Point2D(x, y);
                }
                if (embedding
                        .next_in_adjacency_list(
                            last_node_id,
                            repeated_path.node_id_at_position(repeated_path.number_of_nodes() - 2),
                            last_edge_id
                        )
                        .id == corresponding_edge_id) {
                    const double x = positions[indexes_1[j].second].x;
                    const double y = positions[indexes_1[j].second].y;
                    return Point2D(x, y);
                }
            }

            for (size_t i = 1; i < repeated_path.number_of_edges(); i++) {
                const size_t node_id = repeated_path.node_id_at_position(i);
                if (node_id != node_id_to_find)
                    continue;
                const size_t next_edge_id = repeated_path.edge_id_at_position(i);
                if (embedding
                        .next_in_adjacency_list(
                            node_id,
                            repeated_path.node_id_at_position(i + 1),
                            next_edge_id
                        )
                        .id == corresponding_edge_id) {

                    const double min_x = positions[indexes_0[j].first].x;
                    const double max_x = positions[indexes_0[j].second].x;
                    const double min_y = positions[indexes_0[j].first].y;
                    const double max_y = positions[indexes_0[j].second].y;

                    const size_t num_nodes = repeated_path.number_of_nodes();
                    const double fraction =
                        static_cast<double>(i) / static_cast<double>(num_nodes - 1);
                    const double x = min_x + fraction * (max_x - min_x);
                    const double y = min_y + fraction * (max_y - min_y);
                    return Point2D(x, y);
                }
                if (embedding
                        .prev_in_adjacency_list(
                            node_id,
                            repeated_path.node_id_at_position(i + 1),
                            next_edge_id
                        )
                        .id == corresponding_edge_id) {
                    const double min_x = positions[indexes_1[j].first].x;
                    const double max_x = positions[indexes_1[j].second].x;
                    const double min_y = positions[indexes_1[j].first].y;
                    const double max_y = positions[indexes_1[j].second].y;

                    const size_t num_nodes = repeated_path.number_of_nodes();
                    const double fraction =
                        static_cast<double>(i) / static_cast<double>(num_nodes - 1);
                    const double x = min_x + fraction * (max_x - min_x);
                    const double y = min_y + fraction * (max_y - min_y);
                    return Point2D(x, y);
                }

                const size_t prev_edge_id = repeated_path.edge_id_at_position(i - 1);
                if (embedding
                        .prev_in_adjacency_list(
                            node_id,
                            repeated_path.node_id_at_position(i - 1),
                            prev_edge_id
                        )
                        .id == corresponding_edge_id) {

                    const double min_x = positions[indexes_0[j].first].x;
                    const double max_x = positions[indexes_0[j].second].x;
                    const double min_y = positions[indexes_0[j].first].y;
                    const double max_y = positions[indexes_0[j].second].y;

                    const size_t num_nodes = repeated_path.number_of_nodes();
                    const double fraction =
                        static_cast<double>(i) / static_cast<double>(num_nodes - 1);
                    const double x = min_x + fraction * (max_x - min_x);
                    const double y = min_y + fraction * (max_y - min_y);
                    return Point2D(x, y);
                }
                if (embedding
                        .next_in_adjacency_list(
                            node_id,
                            repeated_path.node_id_at_position(i - 1),
                            prev_edge_id
                        )
                        .id == corresponding_edge_id) {
                    const double min_x = positions[indexes_1[j].first].x;
                    const double max_x = positions[indexes_1[j].second].x;
                    const double min_y = positions[indexes_1[j].first].y;
                    const double max_y = positions[indexes_1[j].second].y;

                    const size_t num_nodes = repeated_path.number_of_nodes();
                    const double fraction =
                        static_cast<double>(i) / static_cast<double>(num_nodes - 1);
                    const double x = min_x + fraction * (max_x - min_x);
                    const double y = min_y + fraction * (max_y - min_y);
                    return Point2D(x, y);
                }
            }
        }
        DOMUS_ASSERT(false, "draw_path_inside: did not find the position of {}", node_id_to_find);
        return Point2D(0, 0);
    };

    const size_t first_node_id = path.get_first_node_id();
    const size_t first_edge_id = path.get_first_edge_id();
    const size_t last_node_id = path.get_last_node_id();
    const size_t last_edge_id = path.get_last_edge_id();

    Point2D first_position = find_position(first_node_id, first_edge_id);
    Point2D last_position = find_position(last_node_id, last_edge_id);
    drawer.add(Line2D(first_position, last_position), INNER_PATH_COLOR);
    draw_nodes_of_path(drawer, first_position, last_position, path);
}

void press_key_to_continue() {
    char c;
    std::cin >> c;
}

void draw_face_with_path(
    const graph::Embedding& embedding, const Face& original_face, const graph::Path& path
) {
    SvgDrawer drawer(SVG_WIDTH, SVG_HEIGHT);

    switch (original_face.type()) {

    case FaceType::TYPE_4:
        draw_path_inside(
            drawer,
            path,
            embedding,
            original_face,
            HEXAGON_VERTICES.data(),
            HEXAGON_REPEATED_PATH_ENDPOINTS_0.data(),
            HEXAGON_REPEATED_PATH_ENDPOINTS_1.data()
        );
        draw_the_hexagon_border(drawer, original_face);
        break;

    case FaceType::TYPE_3:
        draw_path_inside(
            drawer,
            path,
            embedding,
            original_face,
            SQUARE_VERTICES.data(),
            SQUARE_REPEATED_PATH_ENDPOINTS_0.data(),
            SQUARE_REPEATED_PATH_ENDPOINTS_1.data()
        );
        draw_the_square_border(drawer, original_face);
        break;

    default:
        DOMUS_ASSERT(false, "draw_face_with_path: unexpected face type");
    }

    drawer.save_to_file("test.svg").value();

    press_key_to_continue();
}

void draw_face_with_2_paths_path(
    const Embedding& embedding, const Face& original_face, const Path& path_1, const Path& path_2
) {
    SvgDrawer drawer(SVG_WIDTH, SVG_HEIGHT);

    draw_path_inside(
        drawer,
        path_1,
        embedding,
        original_face,
        HEXAGON_VERTICES.data(),
        HEXAGON_REPEATED_PATH_ENDPOINTS_0.data(),
        HEXAGON_REPEATED_PATH_ENDPOINTS_1.data()
    );
    draw_path_inside(
        drawer,
        path_2,
        embedding,
        original_face,
        HEXAGON_VERTICES.data(),
        HEXAGON_REPEATED_PATH_ENDPOINTS_0.data(),
        HEXAGON_REPEATED_PATH_ENDPOINTS_1.data()
    );
    draw_the_hexagon_border(drawer, original_face);

    drawer.save_to_file("test.svg").value();

    press_key_to_continue();
}

} // namespace domus::torus