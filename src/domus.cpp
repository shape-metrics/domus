#include <iostream>
#include <optional>
#include <string>

#include "domus/core/graph/file_loader.hpp"
#include "domus/orthogonal/drawing.hpp"
#include "domus/orthogonal/drawing_builder.hpp"
#include "domus/orthogonal/drawing_stats.hpp"
#include "domus/planarity/auslander_parter.hpp"
#include "domus/planarity/embedding.hpp"

using namespace std;

class UndirectedGraph;

int main() {
    string svg_filename = "drawing.svg";
    string input_graph_filename = "graph.txt";
    const auto graph = load_graph_from_txt_file(input_graph_filename);
    if (!graph) {
        cout << graph.error() << "\n";
        return 1;
    }
    const auto result = make_orthogonal_drawing(*graph);
    if (!result) {
        cout << result.error() << "\n";
        return 1;
    }
    make_svg(result->drawing.augmented_graph, result->drawing.attributes, svg_filename);
    const OrthogonalStats stats = compute_all_orthogonal_stats(result->drawing);
    print_orthogonal_stats(stats);
    cout << "Initial number of cycles: " << result->initial_number_of_cycles << "\n"
         << "Number of added cycles: " << result->number_of_added_cycles << "\n"
         << "Number of useless bends: " << result->number_of_useless_bends << "\n";
    return 0;
}

int main2() {
    const auto graph = load_graph_from_txt_file("graph.txt");
    if (!graph) {
        cout << graph.error() << "\n";
        return 1;
    }
    const optional<Embedding> embedding = embed_graph(*graph);
    if (embedding.has_value()) {
        cout << "Embedding found\n";
        embedding->print();
        cout << "number of faces: " << compute_number_of_faces_in_embedding(embedding.value())
             << "\n"
             << std::boolalpha << is_embedding_planar(embedding.value()) << "\n";
    } else
        cout << "Embedding not found\n";
    return 0;
}
