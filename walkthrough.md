# Interactive Torus Mapping Editor - Implementation Complete

I've successfully rewritten `TorusMapping` to function as an interactive split-screen editor!

## What was changed

### 1. Viewport Splitting
The single window is now split neatly into two halves using `glViewport`:
- **Left Half**: Displays the 3D perspective projection of your torus with the mapped points, edges, and polygons. Camera controls (mouse dragging, zooming) still apply here if you click and drag on the left half.
- **Right Half**: Displays the strict 2D orthographic projection of the canonical `[0.0, 1.0]` rectangle. This makes it trivial to interact with it directly.

### 2. Interaction Modes (Keyboard Shortcuts)
The application now runs on a simple state machine. A text indicator at the top left of the 2D canvas displays your current mode.
- Press `V`: **Add Vertex Mode**. Clicking anywhere on the right canvas will drop a new point in the `[0, 1]` space, which instantly maps onto the 3D torus.
- Press `E`: **Add Edge Mode**. Click one existing vertex (it will highlight yellow), then click a second vertex to establish an edge between them. The edge is drawn on both the 2D rectangle and the 3D torus.
- Press `M`: **Move Vertex Mode**. Click and drag any existing vertex to freely move it around the rectangle space. The 3D torus updates in real-time.
- Press `ESC`: Exits the current active mode back to the default `VIEW` mode. Pressing `ESC` again quits the application.

### 3. Removed `fork()`
`TorusMapping::visualize()` no longer forks a detached process. The function now blocks the main thread, allowing the application memory to be directly modified by the GLUT mouse callbacks. When you close the window, your C++ program resumes naturally.

## How to Verify
You can test this right now by running the executable!
```bash
cd build
./domus
```
> [!TIP]
> Try pressing **`V`** and clicking around the right half of the screen to add points, then press **`M`** to drag them around!

### Files Modified
- [mapping.hpp](file:///home/giordy/Documents/domus/include/domus/torus/mapping.hpp): Added methods to access/modify point coordinates and colors.
- [mapping.cpp](file:///home/giordy/Documents/domus/src/torus/mapping.cpp): Implemented the 2D mouse intersection logic, split `glViewport`, and mode state-machine.
