#define _CRT_SECURE_NO_WARNINGS  // Disable deprecation warnings for sprintf, fopen, etc.
#define GRID_SIZE 1000  // The range of the grid lines, you can adjust this value
#define GRID_STEP 1.0f  // Step size between lines
#define RAYGUI_IMPLEMENTATION

#include "raylib.h"
#include "raygui.h"
#include "raymath.h"

// Define program states
typedef enum { MAIN_MENU, GAME } GameState;

void DrawUnlimitedGrid(int gridSize, float gridStep) {
    // Draw the major grid lines (larger divisions)
    for (int i = -gridSize; i <= gridSize; i++) {
        if (i % 5 == 0) {  // Major grid lines (like Blender's)
            // Draw horizontal lines (X-axis)
            DrawLine3D((Vector3){i * gridStep, 0.0f, -gridSize * gridStep}, (Vector3){i * gridStep, 0.0f, gridSize * gridStep}, (Color){0xE1, 0xE0, 0xE0, 255});

            // Draw vertical lines (Z-axis)
            DrawLine3D((Vector3){-gridSize * gridStep, 0.0f, i * gridStep}, (Vector3){gridSize * gridStep, 0.0f, i * gridStep}, (Color){0xE1, 0xE0, 0xE0, 255});
        } else {  // Minor grid lines (smaller divisions)
            // Draw horizontal lines (X-axis)
            DrawLine3D((Vector3){i * gridStep, 0.0f, -gridSize * gridStep}, (Vector3){i * gridStep, 0.0f, gridSize * gridStep}, (Color){0xE1, 0xE0, 0xE0, 255});

            // Draw vertical lines (Z-axis)
            DrawLine3D((Vector3){-gridSize * gridStep, 0.0f, i * gridStep}, (Vector3){gridSize * gridStep, 0.0f, i * gridStep}, (Color){0xE1, 0xE0, 0xE0, 255});
        }
    }
}


int main()
{
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "Develop Game Engine");

    // Define the camera to look into our 3d world (position, target, up vector)
    Camera camera = {0};
    camera.position = (Vector3){ 8.0f, 6.0f, 8.0f };    // Camera position
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };      // Camera looking at point
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    camera.fovy = 60.0f;                                // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;             // Camera projection type

    SetTargetFPS(60);  // Set the game to run at 60 frames per second

    GameState currentState = MAIN_MENU;  // Start in the main menu state

    while (!WindowShouldClose())  // Detect window close button or ESC key
    {
        // Handle input and state transitions
        if (currentState == MAIN_MENU)
        {
            // Check if the "Start" button is clicked
            if (GuiButton(Rectangle{ 350, 200, 100, 30 }, "Start"))
            {
                currentState = GAME;  // Change state to GAME when Start is clicked
            }
        }
        else if (currentState == GAME)
        {
       if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
    Vector2 mouseDelta = GetMouseDelta();  // Get the mouse delta

    // Define sensitivity for rotation and pan
    float rotationSpeed = 0.5f;   // Adjust for orbiting speed
    float panSpeed = 0.01f;       // Adjust for panning speed

    // Check if Shift is held for panning; otherwise, orbit
    if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
        // Panning: Adjust camera position and target
        Vector3 right = Vector3Normalize(Vector3CrossProduct(camera.up, Vector3Subtract(camera.target, camera.position)));
        Vector3 up = camera.up;

        Vector3 panOffset = Vector3Add(
            Vector3Scale(right, -mouseDelta.x * panSpeed),
            Vector3Scale(up, mouseDelta.y * panSpeed)
        );

        camera.position = Vector3Add(camera.position, panOffset);
        camera.target = Vector3Add(camera.target, panOffset);
    } else {
        // Orbiting: Rotate camera around the target
        Vector3 direction = Vector3Subtract(camera.position, camera.target);

        // Calculate yaw (horizontal) and pitch (vertical) rotations
        float yaw = -mouseDelta.x * rotationSpeed * DEG2RAD;
        float pitch = -mouseDelta.y * rotationSpeed * DEG2RAD;

        // Apply rotations using spherical coordinates
        Matrix rotationMatrix = MatrixRotateXYZ((Vector3){ pitch, yaw, 0.0f });
        direction = Vector3Transform(direction, rotationMatrix);

        // Update camera position based on the rotated direction
        camera.position = Vector3Add(camera.target, direction);

        // Prevent excessive pitch (clamping vertical rotation)
        if (fabsf(Vector3Angle(direction, camera.up) - PI/2) > PI/3) {
            camera.position.y = camera.target.y; // Reset to avoid flipping
        }
    }
}

// Update camera projection for movement
UpdateCameraPro(&camera,
    (Vector3){
        (IsKeyDown(KEY_W)) * 0.1f - (IsKeyDown(KEY_S)) * 0.1f,  // Move forward-backward
        (IsKeyDown(KEY_D)) * 0.1f - (IsKeyDown(KEY_A)) * 0.1f,  // Move right-left
        0.0f  // Move up-down
    },
    (Vector3){ 0.0f, 0.0f, 0.0f },  // No rotation (set all to 0)
    GetMouseWheelMove() * 2.0f  // Adjust camera zoom based on mouse wheel movement
);

// Camera movement along the y-axis when Q and E are pressed (up and down)
if (IsKeyDown(KEY_Q)) {
    camera.position.y -= 0.1f;  // Move the camera downwards
    camera.target.y -= 0.1f;
}
if (IsKeyDown(KEY_E)) {
    camera.position.y += 0.1f;  // Move the camera upwards
    camera.target.y += 0.1f;
}




            // If you want to go back to the main menu, you can use Escape key
            if (IsKeyPressed(KEY_ESCAPE))
            {
                currentState = MAIN_MENU;
            }
        }

        // Drawing logic
        BeginDrawing();
        ClearBackground(RAYWHITE);  // Clear the screen with a white background

        if (currentState == MAIN_MENU)
        {
            // Draw the main menu text and button
            DrawText("Click 'Start' to Enter the Game Engine", 220, 170, 20, DARKGRAY);
            GuiButton(Rectangle{ 350, 200, 100, 30 }, "Start");
        }
        else if (currentState == GAME)
        {
            // Begin 3D mode for the blank canvas
            BeginMode3D(camera);

            // Draw grid in 3D space
             DrawUnlimitedGrid(GRID_SIZE, GRID_STEP);  // Drawing the "unlimited" grid

            // Optionally, you can add 3D objects like cubes or spheres to your scene:
            DrawCube((Vector3){ 0.0f, 1.0f, 0.0f }, 2.0f, 2.0f, 2.0f, BLUE);

            EndMode3D();
            // Draw the blank canvas (just a white background for now)
            DrawText("WASDQE, SHIFT AND WHEEL TO CONTROL CAMERA ", 250, 20, 20, DARKGRAY);
        }

        EndDrawing();
    }

    // De-initialization
    CloseWindow();  // Close window and OpenGL context

    return 0;
}
