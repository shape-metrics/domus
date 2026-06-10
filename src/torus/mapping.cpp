#include "domus/torus/mapping.hpp"

#include <GL/freeglut.h>
#include <cmath>
#include <iostream>
#include <string_view>
#include <sys/wait.h>

#include "domus/drawing/rgb_color.hpp"

namespace domus::torus::mapper {
using namespace domus::drawing;

struct TextLabel {
    std::string text;
    Point2D coordinates; // Screen coordinates (0-1 range)
    ColorRGB color;
    TextLabel(std::string_view text, Point2D coordinates, ColorRGB color)
        : text(text), coordinates(coordinates), color(color) {};
};

// Torus parameters
const double MAJOR_RADIUS = 3.0;      // R - distance from center of tube to center of torus
const double MINOR_RADIUS = 1.0;      // r - radius of the tube itself
const int TORUS_SEGMENTS_MAJOR = 128; // Resolution of the torus
const int TORUS_SEGMENTS_MINOR = 64;  // Resolution of the torus
const ColorRGB TORUS_COLOR = GRAY_RGB;

// Sphere (drawn vertices on the torus) parameters
const double SPHERE_RADIUS = 0.1;
const int SPHERE_SLICES = 8; // Detail level of spheres
const ColorRGB SPHERE_COLOR = RED_RGB;
const ColorRGB HIGHLIGHT_SPHERE_COLOR = YELLOW_RGB;

// Cylinder (drawn edges on the torus) parameters
const ColorRGB EDGE_COLOR = NAVY_RGB;
const double EDGE_RADIUS = 0.02;
const int EDGE_SLICES = 50; // Detail level of cylinders

// Labels parameters
std::optional<size_t> hovered_point_index;
std::vector<TextLabel> text_labels;
bool draw_all_labels = false;
const ColorRGB LABELS_TEXT_COLOR = WHITE_RGB;
const ColorRGB LABELS_BACKGROUND_COLOR = {0.2f, 0.2f, 0.2f};

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

// Points variables
std::vector<Point2D> rectangle_points_g;
std::vector<std::pair<size_t, size_t>> rectangle_lines_g;
std::vector<Point3D> torus_points;

// Rectangle variables
bool show_rectangle = true;

// Polygons variables
std::vector<drawing::Polygon2D> rectangle_polygons_g;
const int POLYGON_GRID_RES_U = 512;
const int POLYGON_GRID_RES_V = 256;
const ColorRGB POLYGON_COLOR = TEAL_RGB;
struct PolygonMesh {
    std::vector<Point3D> quads_3d;
    std::vector<Point2D> quads_2d;
};
std::vector<PolygonMesh> cached_polygon_meshes_g;

void add_text_label(
    const std::string_view text, const Point2D& screen_coordinates, const ColorRGB& color
) {
    text_labels.push_back(TextLabel(text, screen_coordinates, color));
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
    return {static_cast<double>(win_x) / viewport[2], static_cast<double>(win_y) / viewport[3]};
}

// Function to add a 3D positioned label that will be drawn in screen space
void add_world_label(
    const std::string& text, const Point3D& world_coordinates, const ColorRGB& color
) {
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
        add_text_label(text, screen_coordinates, color);
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
                           static_cast<float>(glutGet(GLUT_WINDOW_WIDTH));
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
        glColor3f(label.color.r, label.color.g, label.color.b);
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

// Convert 2D rectangle coordinates (x,y) to 3D torus coordinates (x,y,z)
Point3D map_rectangle_to_torus(const Point2D& point) {
    // Map x,y (in range 0-1) to u,v parameters (in range 0-2π)
    double u = 2.0f * M_PI * point.x;
    double v = 2.0f * M_PI * point.y;

    // Parametric equations of the torus
    double x = (MAJOR_RADIUS + MINOR_RADIUS * cos(v)) * cos(u);
    double y = (MAJOR_RADIUS + MINOR_RADIUS * cos(v)) * sin(u);
    double z = MINOR_RADIUS * sin(v);

    return {x, y, z};
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
        double angle = static_cast<double>(i) * 2.0 * M_PI / EDGE_SLICES;
        double ca = cos(angle);
        double sa = sin(angle);

        // Calculate positions consistently using the orthonormal basis
        float vx1 = static_cast<float>(p1.x + radius * (ca * right.x + sa * up.x));
        float vy1 = static_cast<float>(p1.y + radius * (ca * right.y + sa * up.y));
        float vz1 = static_cast<float>(p1.z + radius * (ca * right.z + sa * up.z));

        float vx2 = static_cast<float>(p2.x + radius * (ca * right.x + sa * up.x));
        float vy2 = static_cast<float>(p2.y + radius * (ca * right.y + sa * up.y));
        float vz2 = static_cast<float>(p2.z + radius * (ca * right.z + sa * up.z));

        // Calculate normals accurately
        float nx = static_cast<float>(ca * right.x + sa * up.x);
        float ny = static_cast<float>(ca * right.y + sa * up.y);
        float nz = static_cast<float>(ca * right.z + sa * up.z);

        // Use the same normal for both ends to maintain smoothness
        glNormal3f(nx, ny, nz);
        glVertex3f(vx1, vy1, vz1);

        glNormal3f(nx, ny, nz);
        glVertex3f(vx2, vy2, vz2);
    }
    glEnd();
}

// Draw a line between two points on the torus
void draw_torus_line(const Point2D& start, const Point2D& end, const ColorRGB& color) {
    double ds = (end.x - start.x) / static_cast<double>(EDGE_SLICES);
    double dt = (end.y - start.y) / static_cast<double>(EDGE_SLICES);

    double current_s = start.x;
    double current_t = start.y;

    for (int i = 1; i <= EDGE_SLICES; i++) {
        double next_s = current_s + ds;
        double next_t = current_t + dt;

        Point3D p1 = map_rectangle_to_torus({current_s, current_t});
        Point3D p2 = map_rectangle_to_torus({next_s, next_t});

        draw_oriented_cylinder(p1, p2, EDGE_RADIUS, color);

        current_s = next_s;
        current_t = next_t;
    }
}

// Check if mouse is hovering over a point
void check_hovered_point() {
    // Projection matrix to convert 3D points to screen coordinates
    GLdouble modelview[16];
    GLdouble projection[16];
    GLint viewport[4];

    // Get the matrices
    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
    glGetDoublev(GL_PROJECTION_MATRIX, projection);
    glGetIntegerv(GL_VIEWPORT, viewport);

    // Initialize the hovered point to none
    hovered_point_index = std::nullopt;

    // Check each point
    for (size_t i = 0; i < torus_points.size(); i++) {
        const Point3D& point = torus_points[i];

        // Project 3D point to screen coordinates
        GLdouble screen_x, screen_y, screen_z;
        gluProject(
            point.x,
            point.y,
            point.z,
            modelview,
            projection,
            viewport,
            &screen_x,
            &screen_y,
            &screen_z
        );

        // Convert OpenGL screen Y to window Y (OpenGL has origin at bottom-left)
        screen_y = viewport[3] - screen_y;

        // Check if mouse is within a certain distance of the point
        double dx = mouse_x - screen_x;
        double dy = mouse_y - screen_y;
        double dist_squared = dx * dx + dy * dy;

        // If mouse is close enough to the point (adjust radius as needed)
        if (dist_squared < 100) { // 10 pixel radius squared
            hovered_point_index = i;
            break;
        }
    }

    // Request redisplay if hovering state changed
    glutPostRedisplay();
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
void draw_rectangle() {
    // Save current transformation
    glPushMatrix();

    // Position the rectangle to the right of the torus
    glTranslatef(MAJOR_RADIUS * 2.5f, 0.0f, 0.0f);
    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
    glScalef(MAJOR_RADIUS, MAJOR_RADIUS, 1.0f);

    // Draw rectangle outline
    glColor3f(0.8f, 0.8f, 0.8f);
    glBegin(GL_LINE_LOOP);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(1.0f, 0.0f, 0.0f);
    glVertex3f(1.0f, 1.0f, 0.0f);
    glVertex3f(0.0f, 1.0f, 0.0f);
    glEnd();

    // Draw polygon faces
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for (const auto& mesh : cached_polygon_meshes_g) {
        glColor4f(POLYGON_COLOR.r, POLYGON_COLOR.g, POLYGON_COLOR.b, 0.5f);
        glBegin(GL_QUADS);
        for (const auto& p : mesh.quads_2d) {
            glVertex3f(static_cast<float>(p.x), static_cast<float>(p.y), 0.0f);
        }
        glEnd();
    }
    glDisable(GL_BLEND);

    // Draw polygon outlines
    for (const auto& poly : rectangle_polygons_g) {
        glColor3f(POLYGON_COLOR.r, POLYGON_COLOR.g, POLYGON_COLOR.b);
        glBegin(GL_LINE_LOOP);
        for (const auto& p : poly.get_points()) {
            glVertex3f(static_cast<float>(p.x), static_cast<float>(p.y), 0.0f);
        }
        glEnd();
    }

    // Draw points
    glColor3f(1.0f, 0.0f, 0.0f);
    glPointSize(5.0f);
    glBegin(GL_POINTS);
    for (const Point2D& point : rectangle_points_g)
        glVertex3f(static_cast<float>(point.x), static_cast<float>(point.y), 0.0f);
    glEnd();

    // Draw lines
    glColor3f(0.0f, 1.0f, 0.0f);
    glBegin(GL_LINES);
    for (const auto& line : rectangle_lines_g) {
        const Point2D& p1 = rectangle_points_g[line.first];
        const Point2D& p2 = rectangle_points_g[line.second];
        glVertex3f(static_cast<float>(p1.x), static_cast<float>(p1.y), 0.0f);
        glVertex3f(static_cast<float>(p2.x), static_cast<float>(p2.y), 0.0f);
    }
    glEnd();

    // Restore transformation
    glPopMatrix();
}

void display() {
    // Clear the screen and depth buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Clear any previous text labels
    text_labels.clear();

    // Set up camera using spherical coordinates
    glLoadIdentity();

    // Convert spherical to Cartesian coordinates
    double x = camera_distance * sin(camera_angle_y) * cos(camera_angle_x);
    double y = camera_distance * sin(camera_angle_x);
    double z = camera_distance * cos(camera_angle_y) * cos(camera_angle_x);

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
    for (size_t i = 0; i < torus_points.size(); i++) {
        const auto& point = torus_points[i];
        const auto& rectangle_point = rectangle_points_g[i];
        if (rectangle_point.x == 1.0f || rectangle_point.x == 0.0f)
            continue;
        if (rectangle_point.y == 1.0f || rectangle_point.y == 0.0f)
            continue;
        // If this is the hovered point, draw it bigger and in yellow
        if (hovered_point_index.has_value() && i == *hovered_point_index) {
            draw_sphere(SPHERE_RADIUS * 1.5f, point, HIGHLIGHT_SPHERE_COLOR);
            // Add the label for the hovered point
            char index_str[32];
            sprintf(index_str, "Point %zu", i);
            add_world_label(index_str, {point.x, point.y + 0.3f, point.z}, LABELS_TEXT_COLOR);
        } else {
            draw_sphere(SPHERE_RADIUS, point, SPHERE_COLOR);
            // Add label for all points if the option is enabled
            if (draw_all_labels) {
                char index_str[32];
                sprintf(index_str, "%zu", i);
                add_world_label(index_str, {point.x, point.y + 0.2f, point.z}, LABELS_TEXT_COLOR);
            }
        }
    }
    // Draw lines on the torus
    for (const auto& line : rectangle_lines_g) {
        const Point2D& p1 = rectangle_points_g[line.first];
        const Point2D& p2 = rectangle_points_g[line.second];
        draw_torus_line(p1, p2, EDGE_COLOR);
    }

    // Draw polygons on the torus
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    for (const auto& mesh : cached_polygon_meshes_g) {
        glColor4f(POLYGON_COLOR.r, POLYGON_COLOR.g, POLYGON_COLOR.b, 0.5f);
        glBegin(GL_QUADS);
        for (const auto& p : mesh.quads_3d) {
            glVertex3f(static_cast<float>(p.x), static_cast<float>(p.y), static_cast<float>(p.z));
        }
        glEnd();
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    if (show_rectangle)
        draw_rectangle();                        // Draw the 2D rectangle representation
    render_text_labels(LABELS_BACKGROUND_COLOR); // Render all text labels as 2D overlay
    glutSwapBuffers();
}

// Reshape callback
void reshape(int width, int height) {
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, static_cast<double>(width) / height, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
}

// Mouse button callback
void mouse_button(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) {
        left_mouse_button_down = (state == GLUT_DOWN);
        prev_mouse_x = x;
        prev_mouse_y = y;
    }
}

// Mouse motion callback
void mouse_motion(int x, int y) {
    // Store mouse position for hover checks
    mouse_x = x;
    mouse_y = y;

    // Camera rotation logic
    if (left_mouse_button_down) {
        camera_angle_y -= static_cast<float>(x - prev_mouse_x) * 0.01f;
        camera_angle_x += static_cast<float>(y - prev_mouse_y) * 0.01f;

        // Clamp vertical angle to avoid gimbal lock
        if (camera_angle_x > M_PI / 2.0f - 0.01f)
            camera_angle_x = M_PI / 2.0f - 0.01f;
        if (camera_angle_x < -M_PI / 2.0f + 0.01f)
            camera_angle_x = -M_PI / 2.0f + 0.01f;

        prev_mouse_x = x;
        prev_mouse_y = y;
    }

    check_hovered_point();

    glutPostRedisplay();
}

// Passive mouse motion function for tracking when not clicking
void passive_mouse_motion(int x, int y) {
    mouse_x = x;
    mouse_y = y;
    check_hovered_point();
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
        exit(0);
        break;
    case '0':
        // Reset camera
        camera_angle_x = 0.0f;
        camera_angle_y = 0.0f;
        camera_distance = 10.0f;
        glutPostRedisplay();
        break;
    case 'L':
        // Toggle all labels
        draw_all_labels = !draw_all_labels;
        glutPostRedisplay();
        break;
    case 'S':
        // Toggle rectangle visibility
        show_rectangle = !show_rectangle;
        glutPostRedisplay();
        break;
    case 'H':
        // Print help
        std::cout << "Torus Mapping Visualization" << std::endl;
        std::cout << "0: Reset camera" << std::endl;
        std::cout << "R: Toggle rotation" << std::endl;
        std::cout << "L: Toggle all labels" << std::endl;
        std::cout << "S: Toggle rectangle visibility" << std::endl;
        std::cout << "H: Print help" << std::endl;
        break;
    case 'R':
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

void precompute_polygons() {
    cached_polygon_meshes_g.clear();
    for (const auto& poly : rectangle_polygons_g) {
        PolygonMesh mesh;
        for (int i = 0; i < POLYGON_GRID_RES_U; i++) {
            for (int j = 0; j < POLYGON_GRID_RES_V; j++) {
                double u1 = static_cast<double>(i) / POLYGON_GRID_RES_U;
                double v1 = static_cast<double>(j) / POLYGON_GRID_RES_V;
                double u2 = static_cast<double>(i + 1) / POLYGON_GRID_RES_U;
                double v2 = static_cast<double>(j + 1) / POLYGON_GRID_RES_V;

                Point2D center{(u1 + u2) / 2.0, (v1 + v2) / 2.0};
                if (poly.is_inside(center)) {
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
        cached_polygon_meshes_g.push_back(mesh);
    }
}

void visualize_torus() {
    precompute_polygons();
    // Map all points to the torus
    torus_points.clear();
    for (const auto& point : rectangle_points_g)
        torus_points.push_back(map_rectangle_to_torus(point));

    // Initialize GLUT
    int fake_argc = 0;
    glutInit(&fake_argc, 0);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Torus Mapping Visualization");

    // Set callbacks
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse_button);
    glutMotionFunc(mouse_motion);
    glutMouseWheelFunc(mouse_wheel);
    glutKeyboardFunc(keyboard);
    glutIdleFunc(idle_rotate_camera);
    glutPassiveMotionFunc(passive_mouse_motion);

    // Set up OpenGL
    glEnable(GL_DEPTH_TEST);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // white background
    // glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // black background --- IGNORE ---

    // Start main loop
    glutMainLoop();
}

void TorusMapping::add_point(drawing::Point2D point) { m_rectangle_points.push_back(point); }

void TorusMapping::add_line(size_t start_index, size_t end_index) {
    m_rectangle_lines.emplace_back(start_index, end_index);
}

void TorusMapping::add_polygon(drawing::Polygon2D polygon) {
    m_rectangle_polygons.push_back(polygon);
}

void TorusMapping::visualize() const {
    for (Point2D point : m_rectangle_points) {
        if (point.x > 1.0f || point.x < 0)
            throw std::runtime_error("Point x coordinate out of bounds");
        if (point.y > 1.0f || point.y < 0)
            throw std::runtime_error("Point y coordinate out of bounds");
    }
    rectangle_points_g = m_rectangle_points;
    for (auto [i_0, i_1] : m_rectangle_lines) {
        if (i_0 >= m_rectangle_points.size())
            throw std::runtime_error("Line start index out of bounds");
        if (i_1 >= m_rectangle_points.size())
            throw std::runtime_error("Line end index out of bounds");
    }
    rectangle_lines_g = m_rectangle_lines;

    for (const auto& poly : m_rectangle_polygons) {
        for (Point2D point : poly.get_points()) {
            if (point.x > 1.0f || point.x < 0)
                throw std::runtime_error("Polygon point x coordinate out of bounds");
            if (point.y > 1.0f || point.y < 0)
                throw std::runtime_error("Polygon point y coordinate out of bounds");
        }
    }
    rectangle_polygons_g = m_rectangle_polygons;

    pid_t child_pid = fork();
    if (child_pid < 0) {
        perror("Parent: fork");
        return;
    }
    if (child_pid == 0) {
        // Child process
        visualize_torus();
        return;
    }
    // Parent process
    // int status;
    // waitpid(child_pid, &status, 0); // Not actually necessary
}

} // namespace domus::torus::mapper