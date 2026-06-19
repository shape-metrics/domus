#include "domus/core/color.hpp"
#include "domus/torus/mapping.hpp"

#include <GL/freeglut.h>
#include <cmath>
#include <iostream>
#include <string_view>
#include <sys/wait.h>

namespace domus::torus::mapper {
using namespace domus::drawing;
using color::ColorRGB;

struct TextLabel {
    std::string text;
    Point2D coordinates; // Screen coordinates (0-1 range)
    TextLabel(std::string_view text, Point2D coordinates) : text(text), coordinates(coordinates) {};
};

std::vector<TextLabel> text_labels;
bool draw_all_labels = false;

// Mouse parameters
int mouse_x = 0; // Current X mouse position
int mouse_y = 0; // Current Y mouse position
bool left_mouse_button_down = false;
int prev_mouse_x = 0; // Previous X mouse position
int prev_mouse_y = 0; // Previous Y mouse position

// Camera parameters
double camera_distance = 10.0f;
double camera_angle_x = 0.0f;
double camera_angle_y = 0.0f;
bool idle_camera_rotation = false;

void TorusMapping::precompute_polygons() {
    m_cached_polygon_meshes.clear();
    for (size_t index = 0; index < m_polygons.size(); index++) {
        PolygonMesh mesh(m_polygons[index].color);
        for (int i = 0; i < POLYGON_GRID_RES_U; i++) {
            for (int j = 0; j < POLYGON_GRID_RES_V; j++) {
                const double u1 = static_cast<double>(i) / POLYGON_GRID_RES_U;
                const double v1 = static_cast<double>(j) / POLYGON_GRID_RES_V;
                const double u2 = static_cast<double>(i + 1) / POLYGON_GRID_RES_U;
                const double v2 = static_cast<double>(j + 1) / POLYGON_GRID_RES_V;

                const Point2D center{(u1 + u2) / 2.0, (v1 + v2) / 2.0};
                if (m_polygons[index].is_inside(center)) {
                    mesh.quads_3d.push_back(map_rectangle_to_torus({u1, v1}));
                    mesh.quads_3d.push_back(map_rectangle_to_torus({u2, v1}));
                    mesh.quads_3d.push_back(map_rectangle_to_torus({u2, v2}));
                    mesh.quads_3d.push_back(map_rectangle_to_torus({u1, v2}));

                    mesh.quads_2d.push_back({u1, v1});
                    mesh.quads_2d.push_back({u2, v1});
                    mesh.quads_2d.push_back({u2, v2});
                    mesh.quads_2d.push_back({u1, v2});
                }
            }
        }
        m_cached_polygon_meshes.push_back(mesh);
    }
}

// Convert 3D world coordinates to screen coordinates
Point2D world_to_screen(const Point3D& world_coordinates) {
    GLdouble modelview[16];
    GLdouble projection[16];
    GLint viewport[4];

    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
    glGetDoublev(GL_PROJECTION_MATRIX, projection);
    glGetIntegerv(GL_VIEWPORT, viewport);

    GLdouble win_x, win_y, win_z;
    gluProject(
        world_coordinates.x,
        world_coordinates.y,
        world_coordinates.z,
        modelview,
        projection,
        viewport,
        &win_x,
        &win_y,
        &win_z
    );

    // Convert to 0-1 range
    return {(static_cast<double>(win_x) - viewport[0]) / viewport[2], (static_cast<double>(win_y) - viewport[1]) / viewport[3]};
}

void add_text_label(const std::string_view text, const Point2D& screen_coordinates) {
    text_labels.push_back(TextLabel(text, screen_coordinates));
}

// Function to add a 3D positioned label that will be drawn in screen space
void add_world_label(const std::string& text, const Point3D& world_coordinates) {
    Point2D screen_coordinates = world_to_screen(world_coordinates);

    // Only add if the point is in front of the camera (check win_z value)
    GLdouble modelview[16];
    GLdouble projection[16];
    GLint viewport[4];
    GLdouble win_x, win_y, win_z;

    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
    glGetDoublev(GL_PROJECTION_MATRIX, projection);
    glGetIntegerv(GL_VIEWPORT, viewport);

    gluProject(
        world_coordinates.x,
        world_coordinates.y,
        world_coordinates.z,
        modelview,
        projection,
        viewport,
        &win_x,
        &win_y,
        &win_z
    );

    // If point is in front of camera (Z between 0 and 1 in normalized device coordinates)
    if (win_z >= 0.0 && win_z <= 1.0)
        add_text_label(text, screen_coordinates);
}

// Function to render all the text labels in 2D screen space
void render_text_labels(const ColorRGB& background_color) {
    // Switch to orthographic projection for 2D rendering
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Disable 3D features
    glDisable(GL_DEPTH_TEST);

    // Draw all labels
    for (const auto& label : text_labels) {
        // Draw background
        float text_width = static_cast<float>(glutBitmapLength(
                               GLUT_BITMAP_HELVETICA_18,
                               reinterpret_cast<const unsigned char*>(label.text.c_str())
                           )) /
                           (static_cast<float>(glutGet(GLUT_WINDOW_WIDTH)) / 2.0f);
        float text_height = 0.03f;

        glColor4f(background_color.r, background_color.g, background_color.b, 0.7f);
        glBegin(GL_QUADS);
        glVertex2f(
            static_cast<float>(label.coordinates.x) - text_width / 2 - 0.01f,
            static_cast<float>(label.coordinates.y) - text_height / 2
        );
        glVertex2f(
            static_cast<float>(label.coordinates.x) + text_width / 2 + 0.01f,
            static_cast<float>(label.coordinates.y) - text_height / 2
        );
        glVertex2f(
            static_cast<float>(label.coordinates.x) + text_width / 2 + 0.01f,
            static_cast<float>(label.coordinates.y) + text_height / 2 + 0.01f
        );
        glVertex2f(
            static_cast<float>(label.coordinates.x) - text_width / 2 - 0.01f,
            static_cast<float>(label.coordinates.y) + text_height / 2 + 0.01f
        );
        glEnd();

        // Draw text
        glColor3f(LABELS_TEXT_COLOR.r, LABELS_TEXT_COLOR.g, LABELS_TEXT_COLOR.b);
        glRasterPos2f(
            static_cast<float>(label.coordinates.x) - text_width / 2,
            static_cast<float>(label.coordinates.y) - 0.005f
        );

        for (char c : label.text)
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
    }

    // Restore 3D settings
    glEnable(GL_DEPTH_TEST);

    // Restore matrices
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void draw_sphere(double radius, const Point3D& center, const ColorRGB& color) {
    glColor3f(color.r, color.g, color.b);
    glPushMatrix();
    glTranslatef(
        static_cast<float>(center.x),
        static_cast<float>(center.y),
        static_cast<float>(center.z)
    );
    glutSolidSphere(radius, SPHERE_SLICES, SPHERE_SLICES);
    glPopMatrix();
}

void draw_oriented_cylinder(
    const Point3D& p1, const Point3D& p2, double radius, const ColorRGB& color
) {
    glColor3f(color.r, color.g, color.b);
    // Create an orthonormal basis for the cylinder at each end
    // This ensures smooth transitions and proper lighting
    Point3D axis{p2.x - p1.x, p2.y - p1.y, p2.z - p1.z};
    double len = sqrt(axis.x * axis.x + axis.y * axis.y + axis.z * axis.z);
    axis.x /= len;
    axis.y /= len;
    axis.z /= len;

    // Calculate orthogonal vectors
    Point3D right{0, 0, 0};

    // Find the first orthogonal vector to axis
    if (fabs(axis.x) < fabs(axis.y) && fabs(axis.x) < fabs(axis.z))
        right.x = 1;
    else if (fabs(axis.y) < fabs(axis.z))
        right.y = 1;
    else
        right.z = 1;

    // Cross product to get perpendicular vector
    Point3D up{
        axis.y * right.z - axis.z * right.y,
        axis.z * right.x - axis.x * right.z,
        axis.x * right.y - axis.y * right.x
    };

    len = sqrt(up.x * up.x + up.y * up.y + up.z * up.z);
    up.x /= len;
    up.y /= len;
    up.z /= len;

    // Recalculate right to be truly orthogonal
    right.x = up.y * axis.z - up.z * axis.y;
    right.y = up.z * axis.x - up.x * axis.z;
    right.z = up.x * axis.y - up.y * axis.x;

    // Draw cylinder
    glBegin(GL_TRIANGLE_STRIP);

    for (size_t i = 0; i <= EDGE_SLICES; i++) {
        const double angle = static_cast<double>(i) * 2.0 * M_PI / EDGE_SLICES;
        const double ca = cos(angle);
        const double sa = sin(angle);

        // Calculate positions consistently using the orthonormal basis
        const float vx1 = static_cast<float>(p1.x + radius * (ca * right.x + sa * up.x));
        const float vy1 = static_cast<float>(p1.y + radius * (ca * right.y + sa * up.y));
        const float vz1 = static_cast<float>(p1.z + radius * (ca * right.z + sa * up.z));

        const float vx2 = static_cast<float>(p2.x + radius * (ca * right.x + sa * up.x));
        const float vy2 = static_cast<float>(p2.y + radius * (ca * right.y + sa * up.y));
        const float vz2 = static_cast<float>(p2.z + radius * (ca * right.z + sa * up.z));

        // Calculate normals accurately
        const float nx = static_cast<float>(ca * right.x + sa * up.x);
        const float ny = static_cast<float>(ca * right.y + sa * up.y);
        const float nz = static_cast<float>(ca * right.z + sa * up.z);

        // Use the same normal for both ends to maintain smoothness
        glNormal3f(nx, ny, nz);
        glVertex3f(vx1, vy1, vz1);

        glNormal3f(nx, ny, nz);
        glVertex3f(vx2, vy2, vz2);
    }
    glEnd();
}

// Draw a line between two points on the torus
void TorusMapping::draw_torus_line(
    const Point2D& start, const Point2D& end, const ColorRGB& color
) const {
    const double ds = (end.x - start.x) / static_cast<double>(EDGE_SLICES);
    const double dt = (end.y - start.y) / static_cast<double>(EDGE_SLICES);

    double current_s = start.x;
    double current_t = start.y;

    for (int i = 1; i <= EDGE_SLICES; i++) {
        const double next_s = current_s + ds;
        const double next_t = current_t + dt;

        const Point3D p1 = TorusMapping::map_rectangle_to_torus({current_s, current_t});
        const Point3D p2 = TorusMapping::map_rectangle_to_torus({next_s, next_t});

        draw_oriented_cylinder(p1, p2, EDGE_RADIUS, color);

        current_s = next_s;
        current_t = next_t;
    }
}

// Draw the wireframe torus
void draw_wireframe_torus(const ColorRGB& color) {
    glColor3f(color.r, color.g, color.b);
    // Draw circles around the major radius
    for (size_t i = 0; i < TORUS_SEGMENTS_MAJOR; i++) {
        const double u = 2.0f * M_PI * static_cast<double>(i) / TORUS_SEGMENTS_MAJOR;

        glBegin(GL_LINE_LOOP);
        for (int j = 0; j < TORUS_SEGMENTS_MINOR; j++) {
            const double v = 2.0f * M_PI * j / TORUS_SEGMENTS_MINOR;
            const float x = static_cast<float>((MAJOR_RADIUS + MINOR_RADIUS * cos(v)) * cos(u));
            const float y = static_cast<float>((MAJOR_RADIUS + MINOR_RADIUS * cos(v)) * sin(u));
            const float z = static_cast<float>(MINOR_RADIUS * sin(v));
            glVertex3f(x, y, z);
        }
        glEnd();
    }
    // Draw circles around the minor radius
    for (size_t j = 0; j < TORUS_SEGMENTS_MINOR; j++) {
        const double v = 2.0f * M_PI * static_cast<double>(j) / TORUS_SEGMENTS_MINOR;

        glBegin(GL_LINE_LOOP);
        for (size_t i = 0; i < TORUS_SEGMENTS_MAJOR; i++) {
            const double u = 2.0f * M_PI * static_cast<double>(i) / TORUS_SEGMENTS_MAJOR;
            const float x = static_cast<float>((MAJOR_RADIUS + MINOR_RADIUS * cos(v)) * cos(u));
            const float y = static_cast<float>((MAJOR_RADIUS + MINOR_RADIUS * cos(v)) * sin(u));
            const float z = static_cast<float>(MINOR_RADIUS * sin(v));
            glVertex3f(x, y, z);
        }
        glEnd();
    }
}

// Draw the rectangle with points and lines
void TorusMapping::draw_rectangle() const {
    // Save current transformation
    glPushMatrix();

    // Disable depth test for 2D drawing to prevent z-fighting
    // where items at z=0.0 hide subsequently drawn items at z=0.0
    glDisable(GL_DEPTH_TEST);

    // We are in an orthographic projection from 0 to 1, no transform needed

    // Draw rectangle outline
    glLineWidth(3.0f);           // Make the border thick and visible
    glColor3f(0.5f, 0.5f, 0.5f); // Medium gray color
    glBegin(GL_LINE_LOOP);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(1.0f, 0.0f, 0.0f);
    glVertex3f(1.0f, 1.0f, 0.0f);
    glVertex3f(0.0f, 1.0f, 0.0f);
    glEnd();
    glLineWidth(1.0f); // Restore default line width

    // Draw polygon faces
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for (size_t i = 0; i < m_cached_polygon_meshes.size(); i++) {
        const auto& mesh = m_cached_polygon_meshes[i];
        glColor4f(mesh.color.r, mesh.color.g, mesh.color.b, 0.5f);
        glBegin(GL_QUADS);
        for (const auto& p : mesh.quads_2d) {
            glVertex3f(static_cast<float>(p.x), static_cast<float>(p.y), 0.0f);
        }
        glEnd();
    }
    glDisable(GL_BLEND);

    // Draw polygon outlines
    for (const auto& polygon : m_polygons) {
        glColor3f(
            polygon.fill_color.value().r,
            polygon.fill_color.value().g,
            polygon.fill_color.value().b
        );
        glBegin(GL_LINE_LOOP);
        for (auto& point : polygon.points) {
            glVertex3f(static_cast<float>(point.x), static_cast<float>(point.y), 0.0f);
        }
        glEnd();
    }

    // Draw lines
    glLineWidth(RECTANGLE_EDGE_WIDTH);
    glBegin(GL_LINES);
    for (const auto& line : m_lines) {
        glColor3f(line.color.r, line.color.g, line.color.b);
        glVertex3f(static_cast<float>(line.p1.x), static_cast<float>(line.p1.y), 0.0f);
        glVertex3f(static_cast<float>(line.p2.x), static_cast<float>(line.p2.y), 0.0f);
    }
    glEnd();
    glLineWidth(1.0f);

    // Draw points (drawn last so they are on top)
    for (size_t i = 0; i < m_circles.size(); i++) {
        const auto& circle = m_circles[i];
        glPointSize(static_cast<float>(circle.radius));
        glBegin(GL_POINTS);
        glColor3f(circle.color.r, circle.color.g, circle.color.b);
        glVertex3f(static_cast<float>(circle.center.x), static_cast<float>(circle.center.y), 0.0f);
        glEnd();

        if (draw_all_labels) {
            char index_str[32];
            sprintf(index_str, "%zu", m_circle_to_node_id[i]);
            add_world_label(index_str, {circle.center.x, circle.center.y + 0.03f, 0.0f});
        }
    }

    // Restore 3D settings
    glEnable(GL_DEPTH_TEST);

    // Restore transformation
    glPopMatrix();
}

void TorusMapping::display() const {
    // Clear the screen and depth buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Clear any previous text labels
    text_labels.clear();

    const int width = glutGet(GLUT_WINDOW_WIDTH);
    const int height = glutGet(GLUT_WINDOW_HEIGHT);

    // --- LEFT VIEWPORT (3D Torus) ---
    glViewport(0, 0, width / 2, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, (static_cast<double>(width) / 2.0) / height, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Convert spherical to Cartesian coordinates
    const double x = camera_distance * sin(camera_angle_y) * cos(camera_angle_x);
    const double y = camera_distance * sin(camera_angle_x);
    const double z = camera_distance * cos(camera_angle_y) * cos(camera_angle_x);

    gluLookAt(
        x,
        y,
        z, // Camera position
        0.0f,
        0.0f,
        0.0f, // Look at center
        0.0f,
        1.0f,
        0.0f // Up vector
    );

    // Draw the 3D scene
    draw_wireframe_torus(TORUS_COLOR);

    // Draw points on the torus and add their labels
    for (size_t i = 0; i < m_circles.size(); i++) {
        const Point3D point = map_rectangle_to_torus(m_circles[i].center);

        draw_sphere(m_circles[i].radius / 150.0, point, m_circles[i].color);
        // Add label for all points if the option is enabled
        if (draw_all_labels) {
            char index_str[32];
            sprintf(index_str, "%zu", m_circle_to_node_id[i]);
            add_world_label(index_str, {point.x, point.y + 0.2f, point.z});
        }
    }
    // Draw lines on the torus

    for (const auto& line : m_lines)
        draw_torus_line(line.p1, line.p2, line.color);

    // Draw polygons on the torus
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    for (size_t i = 0; i < m_cached_polygon_meshes.size(); i++) {
        const auto& mesh = m_cached_polygon_meshes[i];
        glColor4f(mesh.color.r, mesh.color.g, mesh.color.b, 0.5f);
        glBegin(GL_QUADS);
        for (const auto& p : mesh.quads_3d) {
            glVertex3f(static_cast<float>(p.x), static_cast<float>(p.y), static_cast<float>(p.z));
        }
        glEnd();
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    glViewport(0, 0, width / 2, height);
    render_text_labels(LABELS_BACKGROUND_COLOR);
    text_labels.clear();

    // --- RIGHT VIEWPORT (2D Rectangle) ---
    glViewport(width / 2, 0, width / 2, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-0.125, 1.125, -0.125, 1.125, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    draw_rectangle(); // Draw the 2D rectangle representation

    glViewport(width / 2, 0, width / 2, height);
    render_text_labels(LABELS_BACKGROUND_COLOR);
    text_labels.clear();

    glutSwapBuffers();
}

// Reshape callback
void reshape(int, int) {
    // We handle viewports and projections in display() now
}

// Mouse button callback
void mouse_button(int button, int state, int x, int y) {
    int width = glutGet(GLUT_WINDOW_WIDTH);
    if (x < width / 2) {
        // Left viewport interaction
        if (button == GLUT_LEFT_BUTTON) {
            left_mouse_button_down = (state == GLUT_DOWN);
            prev_mouse_x = x;
            prev_mouse_y = y;
        }
    }
}

// Mouse motion callback
void mouse_motion(int x, int y) {
    int width = glutGet(GLUT_WINDOW_WIDTH);

    mouse_x = x;
    mouse_y = y;

    if (x < width / 2) {
        // Camera rotation logic
        if (left_mouse_button_down) {
            camera_angle_y -= static_cast<float>(x - prev_mouse_x) * 0.01f;
            camera_angle_x += static_cast<float>(y - prev_mouse_y) * 0.01f;

            if (camera_angle_x > M_PI / 2.0f - 0.01f)
                camera_angle_x = M_PI / 2.0f - 0.01f;
            if (camera_angle_x < -M_PI / 2.0f + 0.01f)
                camera_angle_x = -M_PI / 2.0f + 0.01f;

            prev_mouse_x = x;
            prev_mouse_y = y;
        }
    }
    glutPostRedisplay();
}

// Mouse wheel callback
void mouse_wheel(int, int dir, int, int) {
    if (dir > 0)
        camera_distance *= 0.9f;
    else
        camera_distance *= 1.1f;
    glutPostRedisplay();
}

// Keyboard callback
void keyboard(unsigned char key, int, int) {
    switch (key) {
    case 27: // ESC key
        glutLeaveMainLoop();
        break;
    case '0':
        // Reset camera
        camera_angle_x = 0.0f;
        camera_angle_y = 0.0f;
        camera_distance = 10.0f;
        glutPostRedisplay();
        break;
    case 'L':
    case 'l':
        // Toggle all labels
        draw_all_labels = !draw_all_labels;
        glutPostRedisplay();
        break;
    case 'H':
    case 'h':
        std::cout << "Torus Mapping Visualization" << std::endl;
        std::cout << "0: Reset camera" << std::endl;
        std::cout << "R: Toggle rotation" << std::endl;
        std::cout << "L: Toggle all labels" << std::endl;
        std::cout << "ESC: Reset mode / Quit" << std::endl;
        std::cout << "H: Print help" << std::endl;
        break;
    case 'R':
    case 'r':
        // Toggle idle camera rotation
        idle_camera_rotation = !idle_camera_rotation;
        break;
    }
}

void idle_rotate_camera() {
    if (!idle_camera_rotation)
        return;
    camera_angle_y += 0.015f;
    if (camera_angle_y > 360.0f)
        camera_angle_y -= 360.0f;
    glutPostRedisplay();
}

TorusMapping* g_current_mapping = nullptr;

void display_callback() {
    if (g_current_mapping)
        g_current_mapping->display();
}

void TorusMapping::visualize() {
    g_current_mapping = this;
    precompute_polygons();

    // Initialize GLUT
    int fake_argc = 1;
    char* fake_argv[] = {const_cast<char*>("domus"), nullptr};
    glutInit(&fake_argc, fake_argv);
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Torus Mapping Visualization");

    // Set callbacks
    glutDisplayFunc(display_callback);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse_button);
    glutMotionFunc(mouse_motion);
    glutMouseWheelFunc(mouse_wheel);
    glutKeyboardFunc(keyboard);
    glutIdleFunc(idle_rotate_camera);

    // Set up OpenGL
    glEnable(GL_DEPTH_TEST);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // white background
    // glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // black background --- IGNORE ---

    // Start main loop
    glutMainLoop();
}

} // namespace domus::torus::mapper