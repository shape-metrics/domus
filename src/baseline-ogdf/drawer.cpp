#include "baseline-ogdf/drawer.hpp"

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/basic/GraphList.h>
#include <ogdf/basic/LayoutStatistics.h>
#include <ogdf/fileformats/GraphIO.h>
#include <ogdf/orthogonal/OrthoLayout.h>
#include <ogdf/planarity/EmbedderMinDepthMaxFaceLayers.h>
#include <ogdf/planarity/PlanarSubgraphFast.h>
#include <ogdf/planarity/PlanarizationLayout.h>
#include <ogdf/planarity/RemoveReinsertType.h>
#include <ogdf/planarity/SubgraphPlanarizer.h>
#include <ogdf/planarity/VariableEmbeddingInserter.h>

#include <cmath>
#include <format>
#include <fstream>
#include <iostream>
#include <set>
#include <string>

#include "core/graph/graph.hpp"
#include "core/graph/graphs_algorithms.hpp"
#include "orthogonal/drawing_builder.hpp"

class OGDFDrawing {
 private:
  std::unordered_map<int, std::tuple<int, int>> id_to_coords;
  std::unordered_map<int, std::vector<int>> edge_to_bend_ids;

 public:
  void set_edge_to_bend_ids(int key, int bend_id) {
    edge_to_bend_ids[key].push_back(bend_id);
  }
  std::vector<int> get_edge_to_bend_ids(int key) {
    if (edge_to_bend_ids.find(key) != edge_to_bend_ids.end())
      return edge_to_bend_ids[key];
    return std::vector<int>();
  }
  auto is_key_in_edge_to_bend_ids(int key) {
    return edge_to_bend_ids.find(key) != edge_to_bend_ids.end();
  }
  void set_id_to_coords(int id, int x, int y) {
    id_to_coords[id] = std::make_tuple(x, y);
  }
  auto get_id_to_coords(int id) { return id_to_coords[id]; }
  auto get_id_to_coords() { return id_to_coords; }
};

int make_key(int x, int y) {
  return (x << 16) ^ y;  // bit shift + XOR
}

std::vector<int> count_length_per_edge(const ogdf::GraphAttributes &GA,
                                       const ogdf::Graph &G,
                                       OGDFDrawing &ogdf_drawing) {
  std::vector<int> lengths_per_edge;
  for (ogdf::edge e : G.edges) {
    int source = e->source()->index();
    int target = e->target()->index();
    if (ogdf_drawing.is_key_in_edge_to_bend_ids(make_key(source, target))) {
      std::vector<int> bend_ids =
          ogdf_drawing.get_edge_to_bend_ids(make_key(source, target));
      bend_ids.insert(bend_ids.begin(), source);
      bend_ids.push_back(target);
      int x = 0;
      int y = 0;
      for (int i = 0; i < bend_ids.size() - 1; ++i) {
        x += std::abs(
            std::get<0>(ogdf_drawing.get_id_to_coords(bend_ids[i])) -
            std::get<0>(ogdf_drawing.get_id_to_coords(bend_ids[i + 1])));
        y += std::abs(
            std::get<1>(ogdf_drawing.get_id_to_coords(bend_ids[i])) -
            std::get<1>(ogdf_drawing.get_id_to_coords(bend_ids[i + 1])));
      }
      lengths_per_edge.push_back(x + y);
    } else {
      int x = std::abs(std::get<0>(ogdf_drawing.get_id_to_coords(source)) -
                       std::get<0>(ogdf_drawing.get_id_to_coords(target)));
      int y = std::abs(std::get<1>(ogdf_drawing.get_id_to_coords(source)) -
                       std::get<1>(ogdf_drawing.get_id_to_coords(target)));
      lengths_per_edge.push_back(x + y);
    }
  }
  return lengths_per_edge;
}

int count_total_edge_length(const std::vector<int> &lengths_per_edge) {
  int total_length = 0;
  for (int length : lengths_per_edge) total_length += length;
  return total_length;
}

int count_max_edge_length(const std::vector<int> &lengths_per_edge) {
  int max_length = 0;
  for (int length : lengths_per_edge)
    if (length > max_length) max_length = length;
  return max_length;
}

// after normalization, node's coordinates are in range [1, x] and [1, y]
// in this way, the area of a path is 1*n
int compute_normalized_area(const ogdf::GraphAttributes &GA,
                            const ogdf::Graph &G, OGDFDrawing &ogdf_drawing) {
  std::set<double> x_coords, y_coords;
  std::unordered_map<double, std::vector<int>> x_coor_to_ids, y_coor_to_ids;
  for (ogdf::node v : G.nodes) {
    double x = GA.x(v);
    double y = GA.y(v);
    int index = v->index();
    x_coords.insert(x);
    y_coords.insert(y);
    x_coor_to_ids[x].push_back(index);
    y_coor_to_ids[y].push_back(index);
  }
  int bend_id = G.numberOfNodes();
  for (ogdf::edge e : G.edges) {
    if (GA.bends(e).size() <= 2) continue;
    std::vector<ogdf::DPoint> bend_vec;
    for (auto &elem : GA.bends(e)) bend_vec.push_back(elem);
    for (int j = 1; j < bend_vec.size() - 1; ++j) {
      double x_source = bend_vec[j - 1].m_x;
      double y_source = bend_vec[j - 1].m_y;
      double x_bend = bend_vec[j].m_x;
      double y_bend = bend_vec[j].m_y;
      double x_target = bend_vec[bend_vec.size() - 1].m_x;
      double y_target = bend_vec[bend_vec.size() - 1].m_y;
      if (x_source != x_bend && x_target != x_bend) {
        x_coords.insert(x_bend);
        y_coords.insert(y_bend);
        x_coor_to_ids[x_bend].push_back(bend_id);
        y_coor_to_ids[y_bend].push_back(bend_id);
        ogdf_drawing.set_edge_to_bend_ids(
            make_key(e->source()->index(), e->target()->index()), bend_id);
        bend_id++;
      }
      if (y_source != y_bend && y_target != y_bend) {
        x_coords.insert(x_bend);
        y_coords.insert(y_bend);
        x_coor_to_ids[x_bend].push_back(bend_id);
        y_coor_to_ids[y_bend].push_back(bend_id);
        ogdf_drawing.set_edge_to_bend_ids(
            make_key(e->source()->index(), e->target()->index()), bend_id);
        bend_id++;
      }
    }
  }
  int x = 0;
  std::vector<double> queue;
  for (auto it = x_coords.begin(); std::next(it) != x_coords.end(); ++it) {
    if ((*std::next(it) - *it) > 10) {
      x++;
      queue.push_back(*it);
      for (auto elem : queue)
        for (auto id : x_coor_to_ids[elem])
          ogdf_drawing.set_id_to_coords(id, x, 0);
      queue.clear();
    } else
      queue.push_back(*it);
  }
  x++;
  for (auto elem : queue)
    for (auto id : x_coor_to_ids[elem]) ogdf_drawing.set_id_to_coords(id, x, 0);
  queue.clear();
  auto last_it = std::prev(x_coords.end());
  for (auto id : x_coor_to_ids[*last_it])
    ogdf_drawing.set_id_to_coords(id, x, 0);
  int y = 0;
  for (auto it = y_coords.begin(); std::next(it) != y_coords.end(); ++it) {
    if ((*std::next(it) - *it) > 10) {
      y++;
      queue.push_back(*it);
      for (auto elem : queue)
        for (auto id : y_coor_to_ids[elem])
          ogdf_drawing.set_id_to_coords(
              id, std::get<0>(ogdf_drawing.get_id_to_coords(id)), y);
      queue.clear();
    } else
      queue.push_back(*it);
  }
  y++;
  for (auto elem : queue)
    for (auto id : y_coor_to_ids[elem])
      ogdf_drawing.set_id_to_coords(
          id, std::get<0>(ogdf_drawing.get_id_to_coords(id)), y);
  queue.clear();
  last_it = std::prev(y_coords.end());
  for (auto id : y_coor_to_ids[*last_it])
    ogdf_drawing.set_id_to_coords(
        id, std::get<0>(ogdf_drawing.get_id_to_coords(id)), y);
  // std::cout << "from ids to normalized coordinates: " << std::endl;
  // for (auto it : ogdf_drawing.get_id_to_coords())
  //     std::cout << "id: " << it.first << " coords: (" <<
  //     std::get<0>(it.second) << ", " << std::get<1>(it.second) << ")" <<
  //     std::endl;
  return x * y;
}

double compute_standard_deviation(std::vector<int> metric_per_edge) {
  int n = metric_per_edge.size();
  if (n == 0) return 0.0;
  double sum = 0.0;
  for (int elem : metric_per_edge) sum += elem;
  double mean = sum / n;
  double variance = 0.0;
  for (int elem : metric_per_edge) variance += (elem - mean) * (elem - mean);
  variance /= n;
  return std::sqrt(variance);
}

int count_bends(const ogdf::GraphAttributes &GA, const ogdf::Graph &G) {
  int bends = 0;
  for (ogdf::edge e : G.edges) {
    int bend_size = GA.bends(e).size();
    if (bend_size > 2) {
      int i = bend_size - 2;
      bends += i;
    }
  }
  return bends;
}

int count_max_number_of_bends(const ogdf::GraphAttributes &GA,
                              const ogdf::Graph &G) {
  int max_bends = 0;
  for (ogdf::edge e : G.edges) {
    int bend_size = GA.bends(e).size();
    if (bend_size > 2) {
      int i = bend_size - 2;
      if (i > max_bends) max_bends = i;
    }
  }
  return max_bends;
}

int count_crossings(const ogdf::GraphAttributes &GA,
                    const ogdf::LayoutStatistics &stats) {
  int crossings = 0;
  for (auto &elem : stats.numberOfCrossings(GA)) crossings += elem;
  return crossings / 2;
}

OGDFResult create_drawing(const Graph &graph,
                          const std::string svg_output_filename,
                          const std::string gml_output_filename) {
  ogdf::Graph G;
  ogdf::GraphAttributes GA(G, ogdf::GraphAttributes::nodeGraphics |
                                  ogdf::GraphAttributes::nodeType |
                                  ogdf::GraphAttributes::edgeGraphics |
                                  ogdf::GraphAttributes::edgeType |
                                  ogdf::GraphAttributes::nodeLabel |
                                  ogdf::GraphAttributes::nodeStyle |
                                  ogdf::GraphAttributes::nodeTemplate);
  std::unordered_map<int, ogdf::node> nodes;
  for (auto &node : graph.get_nodes())
    nodes[node.get_id()] = G.newNode(node.get_id());
  for (auto &node : graph.get_nodes()) {
    int i = node.get_id();
    for (const auto &edge : node.get_edges()) {
      int j = edge.get_to().get_id();
      if (i < j) G.newEdge(nodes[i], nodes[j]);
    }
  }

  for (ogdf::node v : G.nodes) {
    GA.width(v) /= 2;
    GA.height(v) /= 2;
    GA.label(v) = std::to_string(v->index());
  }

  ogdf::PlanarizationLayout pl;

  ogdf::SubgraphPlanarizer *crossMin = new ogdf::SubgraphPlanarizer;
  ogdf::PlanarSubgraphFast<int> *ps = new ogdf::PlanarSubgraphFast<int>;
  ps->runs(100);
  ogdf::VariableEmbeddingInserter *ves = new ogdf::VariableEmbeddingInserter;
  ves->removeReinsert(ogdf::RemoveReinsertType::All);

  crossMin->setSubgraph(ps);
  crossMin->setInserter(ves);
  pl.setCrossMin(crossMin);

  ogdf::EmbedderMinDepthMaxFaceLayers *emb =
      new ogdf::EmbedderMinDepthMaxFaceLayers;
  pl.setEmbedder(emb);

  ogdf::OrthoLayout *ol = new ogdf::OrthoLayout;
  ol->separation(20.0);
  ol->cOverhang(0.4);
  pl.setPlanarLayouter(ol);

  ogdf::setSeed(0);
  pl.call(GA);
  ogdf::LayoutStatistics stats;
  if (svg_output_filename != "")
    ogdf::GraphIO::write(GA, svg_output_filename, ogdf::GraphIO::drawSVG);
  if (gml_output_filename != "")
    ogdf::GraphIO::write(GA, gml_output_filename, ogdf::GraphIO::writeGML);
  OGDFDrawing ogdf_drawing = OGDFDrawing();
  int normalized_area = compute_normalized_area(GA, G, ogdf_drawing);
  std::vector<int> lengths_per_edge =
      count_length_per_edge(GA, G, ogdf_drawing);
  int total_edge_length = count_total_edge_length(lengths_per_edge);
  int max_edge_length = count_max_edge_length(lengths_per_edge);
  int crossings = count_crossings(GA, stats);
  int bends = count_bends(GA, G);
  int max_bends_per_edge = count_max_number_of_bends(GA, G);
  double edge_length_stddev = compute_standard_deviation(lengths_per_edge);
  std::vector<int> bends_per_edge;
  for (ogdf::edge e : G.edges) {
    int bend_count = GA.bends(e).size() - 2;
    bends_per_edge.push_back(bend_count);
  }
  double bends_stddev = compute_standard_deviation(bends_per_edge);

  return {crossings,          bends,           normalized_area,
          total_edge_length,  max_edge_length, edge_length_stddev,
          max_bends_per_edge, bends_stddev};
}