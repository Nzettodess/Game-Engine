#define _CRT_SECURE_NO_WARNINGS  // Disable deprecation warnings for sprintf, fopen, etc.
#define GRID_SIZE 1000  // The range of the grid lines, you can adjust this value
#define GRID_STEP 1.0f  // Step size between lines
#define RAYGUI_IMPLEMENTATION

#include "raylib.h"
#include "raygui.h"
#include "raymath.h"

const int screenWidth = 1720;
const int screenHeight = 880;
// Remove GameState enum and related logic as we go directly into the game scene

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
void DrawInfoPane(bool isCameraMode, float* rotationSpeed, float* panSpeed) {
    int panelWidth = 400;  // Width of the right panel
    int panelX = screenWidth - panelWidth;
    DrawRectangle(panelX, 0, panelWidth, screenHeight, BLACK);

    if (!isCameraMode) {
        DrawText("Information Pane", panelX + 10, 10, 20, WHITE);
        DrawText("Press RMB to Enter CAMERA Mode", panelX + 10, 50, 20, WHITE);
    } else {
        DrawText("Camera Settings", panelX + 10, 10, 20, WHITE);

        // Adjust rotation speed
        DrawText("Rotation Speed", panelX + 10, 50, 16, WHITE);
        GuiSliderBar((Rectangle){ (float)(panelX + 10), 70.0f, 380.0f, 20.0f }, NULL, NULL, rotationSpeed, 0.1f, 2.0f);

        // Adjust pan speed
        DrawText("Pan Speed", panelX + 10, 110, 16, WHITE);
        GuiSliderBar((Rectangle){ (float)(panelX + 10), 130.0f, 380.0f, 20.0f }, NULL, NULL, panSpeed, 0.001f, 0.1f);

        // Display instructions
        DrawText("WASD To Move, QE To Up/Down,", panelX + 10, 200, 20, WHITE);
        DrawText("MouseWheel To Zoom In/Out", panelX + 10, 230, 20, WHITE);
        DrawText("Zero to Exit CAMERA Mode", panelX + 10, 260, 20, WHITE);
    }
}

int main()
{
    InitWindow(screenWidth, screenHeight, "Game Engine by Stellar Blade");

    // Define the camera to look into our 3d world (position, target, up vector)
    bool isCameraMode = false;
    Camera camera = {0};
    camera.position = (Vector3){ 8.0f, 6.0f, 8.0f };    // Camera position
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };      // Camera looking at point
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    camera.fovy = 60.0f;                                // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;             // Camera projection type
    float rotationSpeed = 0.4f;   // Adjust for orbiting speed
    float panSpeed = 0.01f;       // Adjust for panning speed


    SetTargetFPS(60);  // Set the game to run at 60 frames per second

    while (!WindowShouldClose())  // Detect window close button or ESC key
    {
        if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)){
            isCameraMode = true;
        }
        else if (IsKeyPressed(KEY_ZERO)){
             isCameraMode = false;
         }

        if(isCameraMode){

        Vector2 mouseDelta = GetMouseDelta();  // Get the mouse delta

            // Define sensitivity for rotation and pan
            // float rotationSpeed = 0.4f;   // Adjust for orbiting speed
            // float panSpeed = 0.01f;       // Adjust for panning speed

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
        }
        
        // Drawing logic
        BeginDrawing();
        ClearBackground(RAYWHITE);  // Clear the screen with a white background

        // Begin 3D mode for the game scene
        BeginMode3D(camera);

        // Draw grid in 3D space
        DrawUnlimitedGrid(GRID_SIZE, GRID_STEP);  // Drawing the "unlimited" grid

        // Optionally, you can add 3D objects like cubes or spheres to your scene:
        DrawCube((Vector3){ 0.0f, 1.0f, 0.0f }, 2.0f, 2.0f, 2.0f, BLUE);

        EndMode3D();

        DrawInfoPane(isCameraMode, &rotationSpeed, &panSpeed);

        if(isCameraMode){
            DrawInfoPane(isCameraMode, &rotationSpeed, &panSpeed);
        }
        // Draw the blank canvas (just a white background for now)
        // DrawText("Hold RMB to Enter CAMERA Mode", 250, 20, 20, DARKGRAY);

        EndDrawing();
    
}
    // De-initialization
    CloseWindow();  // Close window and OpenGL context

    return 0;
}
