#include "domus/torus/mapping.hpp"

#include <fstream>

#include "../../nlohmann/json.hpp"

namespace domus::torus::mapper {
using json = nlohmann::json;

// std::expected<TorusMapping, std::string> TorusMapping::load_from_file(std::filesystem::path path)
// {
//     std::ifstream file(path);
//     if (!file.is_open()) {
//         return std::unexpected(
//             std::format("TorusMapping::load_from_file: could not open file {}", path.string())
//         );
//     }
//     json data;
//     try {
//         file >> data;
//     } catch (const json::parse_error& e) {
//         return std::unexpected(std::format("JSON parse error: {}", e.what()));
//     }

//     TorusMapping mapping;

//     try {
//         if (data.contains("points") && data["points"].is_array()) {
//             for (const auto& pt : data["points"]) {
//                 mapping.m_rectangle_points.push_back(
//                     {pt["positions"][0].get<double>(), pt["positions"][1].get<double>()}
//                 );
//                 mapping.m_rectangle_points_color.push_back(
//                     {pt["color"][0].get<float>(),
//                      pt["color"][1].get<float>(),
//                      pt["color"][2].get<float>()}
//                 );
//                 mapping.m_is_rectangle_point_hidden.push_back(pt["is_hidden"].get<bool>());
//             }
//         }

//         if (data.contains("lines") && data["lines"].is_array()) {
//             for (const auto& line : data["lines"]) {
//                 mapping.m_rectangle_lines.push_back(
//                     {line["endpoints"][0].get<size_t>(), line["endpoints"][1].get<size_t>()}
//                 );
//                 mapping.m_rectangle_lines_color.push_back(
//                     {line["color"][0].get<float>(),
//                      line["color"][1].get<float>(),
//                      line["color"][2].get<float>()}
//                 );
//             }
//         }

//         if (data.contains("polygons") && data["polygons"].is_array()) {
//             for (const auto& poly : data["polygons"]) {
//                 std::vector<size_t> indices;
//                 for (const auto& idx : poly["indexes"]) {
//                     indices.push_back(idx.get<size_t>());
//                 }
//                 mapping.m_polygons.push_back(indices);
//                 mapping.m_rectangle_polygons_color.push_back(
//                     {poly["color"][0].get<float>(),
//                      poly["color"][1].get<float>(),
//                      poly["color"][2].get<float>()}
//                 );
//             }
//         }

//         mapping.precompute_polygons();
//     } catch (const json::exception& e) {
//         return std::unexpected(std::format("JSON data error: {}", e.what()));
//     }

//     return mapping;
// }

// std::expected<void, std::string> TorusMapping::save_to_file(std::filesystem::path path) {
//     json data;

//     data["points"] = json::array();
//     for (size_t i = 0; i < m_rectangle_points.size(); ++i) {
//         data["points"].push_back(
//             {{"positions", {m_rectangle_points[i].x, m_rectangle_points[i].y}},
//              {"color",
//               {m_rectangle_points_color[i].r,
//                m_rectangle_points_color[i].g,
//                m_rectangle_points_color[i].b}},
//              {"is_hidden", static_cast<bool>(m_is_rectangle_point_hidden[i])}}
//         );
//     }

//     data["lines"] = json::array();
//     for (size_t i = 0; i < m_rectangle_lines.size(); ++i) {
//         data["lines"].push_back(
//             {{"endpoints", {m_rectangle_lines[i].first, m_rectangle_lines[i].second}},
//              {"color",
//               {m_rectangle_lines_color[i].r,
//                m_rectangle_lines_color[i].g,
//                m_rectangle_lines_color[i].b}}}
//         );
//     }

//     data["polygons"] = json::array();
//     for (size_t i = 0; i < m_polygons.size(); ++i) {
//         data["polygons"].push_back(
//             {{"indexes", m_polygons[i]},
//              {"color",
//               {m_rectangle_polygons_color[i].r,
//                m_rectangle_polygons_color[i].g,
//                m_rectangle_polygons_color[i].b}}}
//         );
//     }

//     std::ofstream file(path);
//     if (file.is_open()) {
//         file << data.dump(4);
//         return {};
//     }
//     return std::unexpected(
//         std::format("TorusMapping::save_to_file: could not open file {}", path.string())
//     );
// }

} // namespace domus::torus::mapper