#define _CRT_SECURE_NO_WARNINGS  // Disable deprecation warnings for sprintf, fopen, etc.
#include "raylib.h"

#define RAYGUI_IMPLEMENTATION

#include "raygui.h"

int main()
{
    InitWindow(800, 450, "Raylib + RayGUI Example");

    SetTargetFPS(60);  // Set the game to run at 60 frames per second

    bool showMessageBox = false;  // Track if message box should be shown
    int messageBoxResult = -1;    // Store message box result

    while (!WindowShouldClose())  // Detect window close button or ESC key
    {
        // Check if the button is clicked, and show the message box if true
        if (GuiButton(Rectangle{ 350, 200, 100, 30 }, "Show Message"))
        {
            showMessageBox = true;
        }

        // Show message box and handle response if it’s displayed
        if (showMessageBox)
        {
            messageBoxResult = GuiMessageBox(Rectangle{ 250, 350, 300, 100 }, "Message Box", "Hello! This is RayGUI in action!", "OK;Cancel");

            // If OK or Cancel is clicked, close the message box
            if (messageBoxResult >= 0)
            {
                showMessageBox = false;
                messageBoxResult = -1;  // Reset message box result
            }
        }

        // Drawing logic
        BeginDrawing();
        ClearBackground(RAYWHITE);  // Clear the screen with a white background

        // Draw some text and the button
        DrawText("Click the button to show a message box!", 220, 170, 20, DARKGRAY);
        GuiButton(Rectangle{ 350, 200, 100, 30 }, "Show Message");

        EndDrawing();
    }

    // De-initialization
    CloseWindow();  // Close window and OpenGL context

    return 0;
}
