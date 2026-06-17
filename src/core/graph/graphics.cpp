#include "domus/core/graph/attributes.hpp"

#include "domus/core/color.hpp"

#include <GL/freeglut.h>
#include <algorithm>
#include <limits>
#include <sys/wait.h>

namespace domus::graph {
using namespace domus::drawing;
using color::ColorRGB;

const Attributes* g_current_attributes = nullptr;
const Graph* g_current_graph = nullptr;

constexpr float VERTEX_HALF_SIDE = 0.025f;
constexpr ColorRGB VERTEX_COLOR = NAVY_RGB;
constexpr float EDGE_WIDTH = 5.0f;

// Draw the rectangle with points and lines
void draw_rectangle(float ortho_width, float ortho_height) {
    // Save current transformation
    glPushMatrix();

    // Disable depth test for 2D drawing to prevent z-fighting
    // where items at z=0.0 hide subsequently drawn items at z=0.0
    glDisable(GL_DEPTH_TEST);

    // We are in an orthographic projection from 0 to 1, no transform needed

    glLineWidth(1.0f); // Restore default line width

    // Draw lines
    glLineWidth(EDGE_WIDTH);
    glBegin(GL_LINES);
    for (const auto& edge : g_current_graph->get_all_edges()) {
        const auto& p1 = g_current_attributes->get_position(edge.edge.from_id);
        const auto& p2 = g_current_attributes->get_position(edge.edge.to_id);
        const auto& color = g_current_attributes->get_edge_color(edge.id);
        glColor3f(color.r, color.g, color.b);
        glVertex3f(static_cast<float>(p1.x), static_cast<float>(p1.y), 0.0f);
        glVertex3f(static_cast<float>(p2.x), static_cast<float>(p2.y), 0.0f);
    }
    glEnd();
    glLineWidth(1.0f);

    const int width = glutGet(GLUT_WINDOW_WIDTH);
    const int height = glutGet(GLUT_WINDOW_HEIGHT);
    const float pixel_x = ortho_width / static_cast<float>(width);
    const float pixel_y = ortho_height / static_cast<float>(height);

    // Draw points (drawn last so they are on top)
    for (const auto& node_id : g_current_graph->get_nodes_ids()) {
        float px = static_cast<float>(g_current_attributes->get_position_x(node_id));
        float py = static_cast<float>(g_current_attributes->get_position_y(node_id));

        // Draw square for vertex
        glColor3f(VERTEX_COLOR.r, VERTEX_COLOR.g, VERTEX_COLOR.b);
        glBegin(GL_QUADS);
        glVertex3f(px - VERTEX_HALF_SIDE, py - VERTEX_HALF_SIDE, 0.0f);
        glVertex3f(px + VERTEX_HALF_SIDE, py - VERTEX_HALF_SIDE, 0.0f);
        glVertex3f(px + VERTEX_HALF_SIDE, py + VERTEX_HALF_SIDE, 0.0f);
        glVertex3f(px - VERTEX_HALF_SIDE, py + VERTEX_HALF_SIDE, 0.0f);
        glEnd();

        // Draw node ID text centered
        std::string label = (g_current_attributes->has_attribute(Attribute::NODES_LABELS))
                                ? std::string{g_current_attributes->get_node_label(node_id)}
                                : std::to_string(node_id);
        int text_width_px = glutBitmapLength(
            GLUT_BITMAP_HELVETICA_18,
            reinterpret_cast<const unsigned char*>(label.c_str())
        );

        float offset_x = (static_cast<float>(text_width_px) / 2.0f) * pixel_x;
        float offset_y = 6.0f * pixel_y; // Roughly half the visual height of Helvetica 18

        glColor3f(1.0f, 1.0f, 1.0f); // White text
        glRasterPos2f(px - offset_x, py - offset_y);
        for (char c : label) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
        }
    }

    // Restore 3D settings
    glEnable(GL_DEPTH_TEST);

    // Restore transformation
    glPopMatrix();
}

void display() {
    // Clear the screen and depth buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float min_x = std::numeric_limits<float>::max();
    float max_x = std::numeric_limits<float>::lowest();
    float min_y = std::numeric_limits<float>::max();
    float max_y = std::numeric_limits<float>::lowest();

    bool has_nodes = false;
    for (const auto& node_id : g_current_graph->get_nodes_ids()) {
        has_nodes = true;
        float px = static_cast<float>(g_current_attributes->get_position_x(node_id));
        float py = static_cast<float>(g_current_attributes->get_position_y(node_id));
        min_x = std::min(min_x, px);
        max_x = std::max(max_x, px);
        min_y = std::min(min_y, py);
        max_y = std::max(max_y, py);
    }

    if (!has_nodes) {
        min_x = 0.0f;
        max_x = 1.0f;
        min_y = 0.0f;
        max_y = 1.0f;
    }

    float width_val = max_x - min_x;
    float height_val = max_y - min_y;

    if (width_val == 0.0f) {
        width_val = 1.0f;
        min_x -= 0.5f;
        max_x += 0.5f;
    }
    if (height_val == 0.0f) {
        height_val = 1.0f;
        min_y -= 0.5f;
        max_y += 0.5f;
    }

    float pad_x = std::max(width_val * 0.125f, VERTEX_HALF_SIDE * 2.0f);
    float pad_y = std::max(height_val * 0.125f, VERTEX_HALF_SIDE * 2.0f);

    float left = min_x - pad_x;
    float right = max_x + pad_x;
    float bottom = min_y - pad_y;
    float top = max_y + pad_y;

    const int width = glutGet(GLUT_WINDOW_WIDTH);
    const int height = glutGet(GLUT_WINDOW_HEIGHT);

    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(left, right, bottom, top, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    draw_rectangle(right - left, top - bottom);

    glutSwapBuffers();
}

void Attributes::visualize(const Graph& graph) const {
    g_current_attributes = this;
    g_current_graph = &graph;

    int fake_argc = 1;
    char* fake_argv[] = {const_cast<char*>("domus"), nullptr};
    glutInit(&fake_argc, fake_argv);
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Graph Visualization");

    glutDisplayFunc(display);

    glEnable(GL_DEPTH_TEST);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // white background

    glutMainLoop();
}

} // namespace domus::graph