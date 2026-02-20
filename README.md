# **DOMUS: Drawing Orthogonal Metrics Using (the) Shape**

<!-- ## Update
- [09/2026] Added ... -->
 
## Overview

This code implements the methodology described in "**A Walk on the Wild Side: a Shape-First Methodology for Orthogonal Drawings**" (accepted at [GD2025](https://drops.dagstuhl.de/entities/document/10.4230/LIPIcs.GD.2025.35), [arXiv](https://arxiv.org/abs/2508.19416)) to generate orthogonal drawings of small-to-medium size graphs. It works by repeatedly subdividing edges until a rectilinear drawing is found. Therefore, the approach instead of minimizating the number of crossings opts to minimize the number of bends.

## Key Features

- **Utilizes state-of-the-art SAT solvers.** The methodology works by constructing SAT formulas to obtain rectilinear drawings of the (subdivisions of the) graph. In **domus** we included two solvers
  - **Glucose**, a C++ solver based on [Minisat 2.2](http://minisat.se/MiniSat.html) ([repository](https://github.com/audemard/glucose)).
  - **Kissat**, a reimplementation of [CaDiCaL](https://fmv.jku.at/cadical/) in C ([page link](https://fmv.jku.at/kissat/), [repository](https://github.com/arminbiere/kissat)).
- **Stand alone implementation.** The library is entirely self contained, everything needed is already provided inside the repository and the program is ready to be built and run.

## Building

This project uses CMake. The file `CMakeLists.txt` contains all the rules to compile the library. Domus is intended to be used as a library, however the compilation also builds an executable `domus` that computes the orthogonal drawing of an input graph and saves it as an `.svg` file.

## Usage of the executable

The `domus` executable expects a `graph.txt` files as input, located in the same directory containing the executable itself (you can check in the `/example-graphs/` directory for examples of the used format). It then computes an orthogonal drawing, and saves it as an svg image, `drawing.svg`, again in the same directory of the executable.

## Usage as a library

The library contains plenty of functions, some of the most notable ones are:

``` cpp
// in domus/core/graph/file_loader.hpp
UndirectedGraph load_graph_from_txt_file(std::string filename);

// in domus/orthogonal/drawing_builder.hpp
ShapeMetricsDrawing make_orthogonal_drawing(const UndirectedGraph& graph);
void save_shape_metrics_drawing_to_file(const ShapeMetricsDrawing& result,
  const std::string& path);

// in domus/orthogonal/drawing_stats.hpp
OrthogonalStats compute_all_orthogonal_stats(const OrthogonalDrawing& result);

// in domus/orthogonal/drawing.hpp
void make_svg(
  const UndirectedGraph& graph, const GraphAttributes& attributes, 
  const std::string& filename);
```

## Experiments

In the paper [A Walk on the Wild Side: a Shape-First Methodology for Orthogonal Drawings](www.https://arxiv.org/abs/2508.19416) we described extensive experiments we performed to evaluate our approach against the state-of-the-art approaches, in particular we compared the **shape-metrics** implementation in DOMUS against the **topology-shape-metrics** implementation in OGDF, using widely adopted graph drawing metrics.

The two sets of experiments are:
  - **in vitro**: dataset of unformily at random generated connected graphs with max degree 4;
  - **in the wild**: rome graphs dataset (without disconnected graphs).

