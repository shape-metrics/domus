#include <expected>
#include <optional>
#include <print>
#include <string>

#include "domus/core/graph/attributes.hpp"
#include "domus/core/graph/embedding.hpp"
#include "domus/core/graph/file_loader.hpp"
#include "domus/core/graph/flow.hpp"
#include "domus/core/graph/graph.hpp"
#include "domus/core/graph/test.hpp"
#include "domus/drawing/polygon.hpp"
#include "domus/drawing/rgb_color.hpp"
#include "domus/orthogonal/drawing.hpp"
#include "domus/orthogonal/drawing_builder.hpp"
#include "domus/orthogonal/drawing_stats.hpp"
#include "domus/planarity/auslander_parter.hpp"
#include "domus/planarity/drawing.hpp"
#include "domus/planarity/tutte.hpp"
#include "domus/torus/embedder.hpp"
#include "domus/torus/faces.hpp"
#include "domus/torus/mapping.hpp"

using namespace domus;
using namespace domus::graph;
using namespace domus::planarity;
using namespace domus::orthogonal;
using namespace domus::torus;
using namespace domus::torus::mapper;
using namespace domus::drawing;

void planarity_test(const graph::Graph& graph) {
    const std::optional<Embedding> embedding = compute_planar_embedding(graph);
    if (embedding.has_value()) {
        std::println("Embedding found");
        embedding->print();
        std::println(
            "number of faces: {}",
            compute_number_of_faces_in_embedding(embedding.value())
        );
    } else
        std::println("Embedding not found");
}

void make_orthogonal(const Graph& graph) {
    static constexpr std::string svg_filename = "drawing.svg";
    const auto result = make_orthogonal_drawing(graph);
    make_svg(
        result.drawing.augmented_graph,
        result.drawing.attributes,
        result.drawing.shape,
        svg_filename
    )
        .value();
    stats::compute_all_orthogonal_stats(result.drawing).print();
    std::println("Initial number of cycles: {}", result.initial_number_of_cycles);
    std::println("Number of added cycles: {}", result.number_of_added_cycles);
    std::println("Number of useless bends: {}", result.number_of_useless_bends);
}

void toroidal_test(const graph::Graph& graph) {
    const std::optional<Embedding> embedding = torus::compute_toroidal_embedding(graph);
    if (embedding.has_value()) {
        std::println("Embedding found");
        embedding->print();
        std::println(
            "number of faces: {}",
            compute_number_of_faces_in_embedding(embedding.value())
        );
        std::println("genus: {}", compute_embedding_genus(embedding.value()));
    } else
        std::println("Embedding not found");
}

auto load_graph() {
    std::string input_graph_filename = "graph.txt";
    const auto graph = loader::load_graph_from_txt_file(input_graph_filename);
    if (!graph) {
        println("{}", graph.error());
        std::terminate();
    }
    return graph.value();
}

void test_tutte_layout() {
    Graph graph;
    for (int i = 0; i < 14; i++)
        graph.add_node();
    graph.add_edge(0, 1);
    graph.add_edge(0, 13);
    graph.add_edge(0, 2);
    graph.add_edge(0, 3);
    graph.add_edge(1, 9);
    graph.add_edge(1, 12);
    graph.add_edge(1, 13);
    graph.add_edge(2, 5);
    graph.add_edge(3, 4);
    graph.add_edge(3, 13);
    graph.add_edge(4, 5);
    graph.add_edge(4, 10);
    graph.add_edge(5, 6);
    graph.add_edge(5, 11);
    graph.add_edge(6, 7);
    graph.add_edge(6, 8);
    graph.add_edge(8, 9);
    graph.add_edge(9, 12);
    graph.add_edge(10, 11);
    graph.add_edge(10, 12);
    graph.add_edge(11, 12);
    graph.add_edge(8, 7);
    graph.add_edge(7, 2);
    graph.add_edge(9, 2);
    Attributes attributes;
    attributes.add_attribute(Attribute::NODES_POSITION);
    attributes.set_position(0, 0, 0);
    attributes.set_position(1, 0, 400);
    attributes.set_position(2, 600, 0);
    attributes.set_position(9, 600, 400);
    compute_nodes_positions(graph, attributes);
    auto res = make_svg(graph, attributes, "daje.svg");
    if (!res) {
        std::println("{}", res.error());
    }
    auto cycles = flow::max_vertex_disjoint_cycles(graph, 12);
    std::println("Number of vertex-disjoint cycles: {}", cycles.size());
    for (const auto& cycle : cycles) {
        cycle.print();
    }
    auto paths = flow::max_vertex_disjoint_paths(graph, 0, 9);
    std::println("Number of vertex-disjoint paths: {}", paths.size());
    for (const auto& path : paths) {
        path.print();
    }
}

void test_all_possible_embeddings(const Graph& graph) {
    std::vector<Embedding> embeddings = compute_all_possible_embeddings(graph);
    std::println("Number of embeddings: {}", embeddings.size());
    std::vector<size_t> genuses;
    std::vector<size_t> max_face_types;
    for (const Embedding& embedding : embeddings) {
        const std::vector<Path> faces = compute_faces_in_embedding(graph, embedding);
        const size_t g = compute_embedding_genus(
            graph.get_number_of_nodes(),
            graph.get_number_of_edges(),
            faces.size(),
            1
        );
        if (genuses.size() <= g)
            genuses.resize(g + 1);
        ++genuses[g];
        if (g == 1) {
            size_t max_face_type = 0;
            for (FaceType type : std::views::transform(faces, [&](const Path& path) {
                     return compute_face_from_path(Path(path), graph).type();
                 })) {
                if (static_cast<size_t>(type) > max_face_type)
                    max_face_type = static_cast<size_t>(type);
            }

            if (max_face_types.size() <= static_cast<size_t>(max_face_type))
                max_face_types.resize(static_cast<size_t>(max_face_type) + 1);
            ++max_face_types[static_cast<size_t>(max_face_type)];
        }
    }

    for (size_t i = 1; i < genuses.size(); ++i) {
        std::println("Genus: [{:>2}] Quantity: [{:>4}]", i, genuses[i]);
    }

    for (size_t i = 1; i < max_face_types.size(); ++i) {
        std::println("Case: [{:>2}] Quantity: [{:>4}]", i, max_face_types[i]);
    }
}

void visualize_torus() {
    TorusMapping mapping;

    mapping.add_point({0.0f, 0.0f});
    mapping.add_point({0.4f, 0.0f});
    mapping.add_point({0.6f, 0.0f});
    mapping.add_point({1.0f, 0.0f});

    mapping.add_point({0.0f, 1.0f});
    mapping.add_point({0.4f, 1.0f});
    mapping.add_point({0.6f, 1.0f});
    mapping.add_point({1.0f, 1.0f});

    mapping.add_line(0, 1);
    mapping.add_line(1, 2);
    mapping.add_line(2, 3);
    mapping.add_line(4, 5);
    mapping.add_line(5, 6);
    mapping.add_line(6, 7);

    mapping.add_line(1, 6);

    mapping.set_line_color(0, BLUE_RGB);
    mapping.set_line_color(1, GREEN_RGB);
    mapping.set_line_color(2, BLUE_RGB);
    mapping.set_line_color(3, BLUE_RGB);
    mapping.set_line_color(4, GREEN_RGB);
    mapping.set_line_color(5, BLUE_RGB);

    mapping.set_line_color(6, RED_RGB);

    std::vector<Point2D> polygon_points;
    polygon_points.emplace_back(0.2, 0.2);
    polygon_points.emplace_back(0.2, 0.8);
    polygon_points.emplace_back(0.8, 0.8);
    polygon_points.emplace_back(0.8, 0.2);
    mapping.add_polygon(Polygon2D(polygon_points));
    mapping.visualize();
}

int main() {
    // test_tutte_layout();
    // graph->print(true);
    // std::println("{}", generators::code_to_generate_graph(*graph));

    // planarity_test(*graph);
    // for (const auto& forbidden_minor : test::forbidden_minors)
    //     planarity_test(forbidden_minor);

    // make_orthogonal(*graph);
    // toroidal_test(test::two_cycle_graphs[2]);

    // std::println("k5");
    // test::subdivided_k_5.print(true);

    // toroidal_test(test::subdivided_k_5);

    // std::println("CASE ----- K_5 -------");
    // test_all_possible_embeddings(test::subdivided_k_5);

    // std::println("\n\nCASE ----- K_3_3 -----");
    // test_all_possible_embeddings(test::subdivided_k_3_3);

    visualize_torus();

    return 0;
}
