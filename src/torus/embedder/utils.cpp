#include "utils.hpp"

#include <fstream>

namespace domus::torus {
using namespace domus::graph;
using namespace domus::graph::utilities;

static std::ofstream log_final_configurations("log_final_configurations.txt");

void add_log_final_configuration(const std::vector<Face>& faces) {
    if (log_final_configurations.is_open()) {
        std::vector<size_t> face_types;
        face_types.reserve(faces.size());

        for (const Face& face : faces)
            face_types.push_back(static_cast<size_t>(face.type()));
        std::sort(face_types.begin(), face_types.end());

        for (size_t type : face_types)
            log_final_configurations << type << " ";
        log_final_configurations << std::endl;
    }
}

} // namespace domus::torus
