#define _CRT_SECURE_NO_WARNINGS  // Disable deprecation warnings for sprintf, fopen, etc.
#define GRID_SIZE 1000  // The range of the grid lines, you can adjust this value
#define GRID_STEP 1.0f  // Step size between lines
#define MAX_AUDIO_FILES 5 //
#define RAYGUI_IMPLEMENTATION

#include "raylib.h"
#include "raygui.h"
#include "raymath.h"
#include <stdlib.h> 
#include <string.h>
#include <iostream>

using namespace std;

const int screenWidth = 1720;
const int screenHeight = 880;

// Audio handling structures
typedef struct {
    Sound sound;
    char name[256];
    bool loaded;
} SoundFile;

typedef struct {
    Music music;
    char name[256];
    bool loaded;
} MusicFile;

//Shape Struct
typedef struct {
    Vector3 position;
    Vector3 size;
    Color color;
} Cube;
typedef struct {
    Vector3 position;
    float radius;
    Color color;
} Sphere;
typedef struct {
    Vector3 position;
    float radiusTop;
    float radiusBottom;
    float height;
    int slices;
    Color color;
} Cylinder;
typedef struct {
    Vector3 startPos; // Starting position of the capsule
    Vector3 endPos;   // Ending position of the capsule
    float radius;     // Radius of the capsule
    int slices;       // Number of slices (longitudinal divisions)
    int rings;        // Number of rings (latitudinal divisions)
    Color color;      // Color of the capsule
} Capsule;
typedef struct {
    Vector3 position;
    Vector2 size;
    Color color;
} Plane;

Cube* cubes = NULL;
int cubeCount = 0;
Sphere* spheres = NULL;
int sphereCount = 0;
Cylinder* cylinders = NULL;
int cylinderCount = 0;
Capsule* capsules = NULL;
int capsuleCount = 0;
Plane* planes = NULL;
int planeCount = 0;

// Function to add a cube
void AddCube(Vector3 position, Vector3 size, Color color) {
    cubeCount++;
    cubes = (Cube*)realloc(cubes, cubeCount * sizeof(Cube));
    cubes[cubeCount - 1] = (Cube){ position, size, color };
}
void AddSphere(Vector3 position, float radius, Color color) {
    sphereCount++;
    spheres = (Sphere*)realloc(spheres, sphereCount * sizeof(Sphere));
    spheres[sphereCount - 1] = (Sphere){ position, radius, color };
}
void AddCylinder(Vector3 position, float radiusTop, float radiusBottom, float height, int slices, Color color) {
    cylinderCount++;
    cylinders = (Cylinder*)realloc(cylinders, cylinderCount * sizeof(Cylinder));
    cylinders[cylinderCount - 1] = (Cylinder){ position, radiusTop, radiusBottom, height, slices, color };
}
void AddCapsule(Vector3 startPos, Vector3 endPos, float radius, int slices, int rings, Color color) {
    capsuleCount++;
    capsules = (Capsule*)realloc(capsules, capsuleCount * sizeof(Capsule)); // Reallocate memory for the new capsule
    capsules[capsuleCount - 1] = (Capsule){ startPos, endPos, radius, slices, rings, color };
}
void AddPlane(Vector3 position, Vector2 size, Color color) {
    planeCount++;
    planes = (Plane*)realloc(planes, planeCount * sizeof(Plane));
    planes[planeCount - 1] = (Plane){ position, size, color };
}

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
void DrawInfoPane(bool isNotInAnyMode, bool isCameraMode, bool isShapeCreationMode,bool isAudioMode, bool& isfileunsupported, bool isCollisionMode,
    bool isAssetManagementMode, float* rotationSpeed, float* panSpeed, float* fov, int* projection) 
    {

    //draw pane
    int panelWidth = 400;  // Width of the right panel
    int panelX = screenWidth - panelWidth;
    DrawRectangle(panelX, 0, panelWidth, screenHeight, BLACK);
    //mode switch
    if (isNotInAnyMode) {
        DrawText("Information Pane", panelX + 10, 10, 20, WHITE);

        DrawText("Press RMB to Enter CAMERA Mode", panelX + 10, 50, 20, WHITE);

        DrawText("Press A to Enter  ", panelX + 10, 100, 20, WHITE);
        DrawText("SHAPE CREATION Mode", panelX + 10, 120, 20, WHITE);

        DrawText("Press M to Enter ", panelX + 10, 170, 20, WHITE);
        DrawText("AUDIO Mode", panelX + 10, 190, 20, WHITE);

        DrawText("Press XX to Enter ", panelX + 10, 240, 20, WHITE);
        DrawText("COLLISION Mode", panelX + 10, 260, 20, WHITE);

        DrawText("Press XX to Enter ", panelX + 10, 310, 20, WHITE);
        DrawText("ASSET MANAGEMENT Mode", panelX + 10, 330, 20, WHITE);

        // DrawText("Press XX to Enter Shape CREATION Mode", panelX + 10, 80, 20, WHITE);
    } else if (isCameraMode){
        DrawText("Camera Settings", panelX + 10, 10, 20, WHITE);

        // Adjust rotation speed
        DrawText("Rotation Speed", panelX + 10, 50, 16, WHITE);
        GuiSliderBar((Rectangle){ (float)(panelX + 10), 70.0f, 380.0f, 20.0f }, NULL, NULL, rotationSpeed, 0.1f, 2.0f);

        // Adjust pan speed
        DrawText("Pan Speed", panelX + 10, 110, 16, WHITE);
        GuiSliderBar((Rectangle){ (float)(panelX + 10), 130.0f, 380.0f, 20.0f }, NULL, NULL, panSpeed, 0.001f, 0.1f);

         DrawText("Camera FOV", panelX + 10, 170, 16, WHITE);
        GuiSliderBar((Rectangle){ (float)(panelX + 10), 190.0f, 380.0f, 20.0f }, NULL, NULL, fov, 30.0f, 120.0f);

        // Camera projection mode selection
        DrawText("Projection Mode", panelX + 10, 230, 16, WHITE);
        if (GuiButton((Rectangle){ panelX + 10, 250.0f, 180.0f, 30.0f }, "Perspective")) {
            *projection = CAMERA_PERSPECTIVE;
        }
        if (GuiButton((Rectangle){ panelX + 210, 250.0f, 180.0f, 30.0f }, "Orthographic")) {
            *projection = CAMERA_ORTHOGRAPHIC;
        }
        // Display instructions
        DrawText("WASD To Move, QE To Up/Down,", panelX + 10, 300, 20, WHITE);
        DrawText("MouseWheel To Zoom In/Out", panelX + 10, 330, 20, WHITE);
        DrawText("Zero to Exit CAMERA Mode", panelX + 10, 360, 20, WHITE);
    }else if(isShapeCreationMode){
        DrawText("Shape Creation Mode", panelX + 10, 10, 20, WHITE);
        GuiSetStyle(DEFAULT, TEXT_SIZE, 20);
        if (GuiButton((Rectangle){ 1345, 50, 350, 50 }, "Cube")) { //x coordinate, y coordinate, length. wide
            DrawText("Cube Created", 100, 200, 20, RED);
            AddCube((Vector3){  0.0f, 1.0f, 0.0f }, {2.0f, 2.0f, 2.0f}, BLUE);
            //Debugging AddCube
            // AddCube((Vector3){ GetRandomValue(-5, 5), 1.0f, GetRandomValue(-5, 5) }, (Vector3){ 2.0f, 2.0f, 2.0f }, BLUE);

        }
        if (GuiButton((Rectangle){ 1345, 150, 350, 50 }, "Sphere")) { //x coordinate, y coordinate, length. wide
            DrawText("Sphere Created", 100, 200, 20, RED);

            AddSphere((Vector3){ 0.0f, 1.0f, 0.0f }, 1.5f, RED);
            //Debugging Add
            //AddSphere((Vector3){ GetRandomValue(-5, 5), 1.0f, GetRandomValue(-5, 5) }, 1.5f, RED);
        }
        if(GuiButton((Rectangle){ 1345, 250, 350, 50 }, "Cylinder")) { //x coordinate, y coordinate, length. wide
            
            DrawText("Cylinder Created", 100, 200, 20, RED);
            AddCylinder((Vector3){ 0.0f, 1.0f, 0.0f }, 1.0f, 1.0f, 3.0f, 16, GREEN);
            //Debugging Add
            //AddCylinder((Vector3){ GetRandomValue(-5, 5), 1.0f, GetRandomValue(-5, 5) }, 1.0f, 1.0f, 3.0f, 16, GREEN);
        }
        if (GuiButton((Rectangle){ 1345, 350, 350, 50 }, "Capsule")) { //x coordinate, y coordinate, length. wide
            
            DrawText("Capsule Created", 100, 200, 20, RED);
            AddCapsule((Vector3){ 0.0f, 1.0f, 0.0f }, (Vector3){ 0.0f, -1.0f, 0.0f }, 0.5f, 16, 8, GREEN);
            

            //Debugging Add
            // AddCapsule((Vector3){ GetRandomValue(-5, 5), 1.0f, GetRandomValue(-5, 5) }, (Vector3){ GetRandomValue(-5, 5), 1.0f, GetRandomValue(-5, 5) }, 0.5f, 16, 8, GREEN);
        }
        if (GuiButton((Rectangle){ 1345, 450, 350, 50 }, "Plane")) { //x coordinate, y coordinate, length. wide
            
            DrawText("Plane Created", 100, 200, 20, RED);
            AddPlane((Vector3){ 0.0f, 1.0f, 0.0f }, (Vector2){ 3.0f, 3.0f }, DARKGRAY);
            //Debugging Add
            //AddPlane((Vector3){ GetRandomValue(-5, 5), 0.0f, GetRandomValue(-5, 5) }, (Vector2){ 3.0f, 3.0f }, DARKGRAY);


        }
        DrawText("Zero to Exit SHAPE CREATION Mode", panelX + 10, 550, 20, WHITE);
    }
    else if(isAudioMode){
        DrawText("Audio Mode", panelX + 10, 10, 20, WHITE);
        DrawText("Sound Effects", panelX + 10, 30, 20, WHITE);
        DrawText("Music", panelX + 10, 110, 20, WHITE);
        DrawText("Zero to Exit Audio Mode", panelX + 10, 360, 20, WHITE);

        if(isfileunsupported){

            int rectWidth = 500;
            int rectHeight = 150;
            int rectX = (screenWidth - rectWidth) / 2;
            int rectY = (screenHeight - rectHeight) / 2;
            
            DrawRectangle(rectX, rectY, rectWidth, rectHeight, DARKGRAY);
            DrawText("Unsupported File Format", rectX+120, rectY+10, 20, WHITE);
            DrawText("The file format is not supported.", rectX+70, rectY+40, 20, WHITE);
            DrawText("Accepted Format: .wav, .ogg, .flac, .mp3", rectX+50, rectY+60, 20, WHITE);
                if (GuiButton((Rectangle){ rectX+150, rectY+100, 180, 30 }, "OK")) {
                    isfileunsupported = false;  // Close the message box
                }
        }

        
        
    }
    else if(isCollisionMode){
        DrawText("Collision Mode", panelX + 10, 10, 20, WHITE);
        DrawText("Zero to Exit Collision Mode", panelX + 10, 360, 20, WHITE);

    }else if(isAssetManagementMode){
        DrawText("Asset Management Mode", panelX + 10, 10, 20, WHITE);
        DrawText("Zero to Exit Asset Management Mode", panelX + 10, 360, 20, WHITE);
        
    }
}

int main()
{
    InitWindow(screenWidth, screenHeight, "Game Engine by Stellar Blade");

    //Mode Switching
    bool isNotInAnyMode = true;
    bool isCameraMode = false;
    bool isShapeCreationMode = false;
    bool isAudioMode = false;
        bool isfileunsupported = false;
    bool isCollisionMode = false;
    bool isAssetManagementMode = false;
    
    // Camera Param
    Camera camera = {0};
    camera.position = (Vector3){ 8.0f, 6.0f, 8.0f };    // Camera position
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };      // Camera looking at point
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    camera.fovy = 60.0f;                                // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;             // Camera projection type
    float rotationSpeed = 0.4f;   // Adjust for orbiting speed
    float panSpeed = 0.01f;       // Adjust for panning speed
    float fov = 60.0f;            // Field of view
    int projection = CAMERA_PERSPECTIVE; // Projection type

    static SoundFile soundFiles[MAX_AUDIO_FILES] = { 0 };
    static MusicFile musicFile = { 0 };
    static int selectedSound = -1;
    static bool musicPlaying = false;

    static float soundVolume = 1.0f;
    static float soundPitch = 1.0f;
    static float soundPan = 0.0f;

    static float musicVolume = 1.0f;

    SetTargetFPS(60);  // Set the game to run at 60 frames per second

    while (!WindowShouldClose())  // Detect window close button or ESC key
    {
        //CameraMode Trigger
        if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)){
            isCameraMode = true;
        }
        else if (IsKeyPressed(KEY_ZERO)){
             isCameraMode = false;
             isNotInAnyMode = true;
         }

        //Shape Creation Mode Trigger
        if(IsKeyPressed(KEY_A)){
            isShapeCreationMode = true;
        }
        else if (IsKeyPressed(KEY_ZERO)){
             isShapeCreationMode = false;
             isNotInAnyMode = true;
         }
         //Audio Mode
        if(IsKeyPressed(KEY_M)){
            isAudioMode = true;
        }
        else if (IsKeyPressed(KEY_ZERO)){
             isAudioMode = false;
             isNotInAnyMode = true;
         }
        //Camera Mode Logic
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
        //Shape Creation Mode
        if(isShapeCreationMode){

        }
        //Audio Mode
        if(isAudioMode){

            if (IsFileDropped()) {
                // Load dropped files using FilePathList
                FilePathList droppedFiles = LoadDroppedFiles();

                    for (unsigned int i = 0; i < droppedFiles.count; i++) {
                        const char *filePath = droppedFiles.paths[i];
                        
                        if (IsFileExtension(filePath, ".wav")||IsFileExtension(filePath, ".ogg")||IsFileExtension(filePath, ".flac")||IsFileExtension(filePath, ".mp3")) 
                        {
                            cout << "Loaded Sound";
                            if (IsFileExtension(filePath, ".wav") || IsFileExtension(filePath, ".ogg") || IsFileExtension(filePath, ".flac")) {
                            // Load as sound effect
                                for (int j = 0; j < MAX_AUDIO_FILES; j++) {
                                    if (!soundFiles[j].loaded) {
                                        soundFiles[j].sound = LoadSound(filePath);
                                        strncpy(soundFiles[j].name, filePath, 255);
                                        soundFiles[j].loaded = true;
                                        cout << "Loaded Sound";
                                        break;
                                    }
                                }
                            } else if (IsFileExtension(filePath, ".mp3")) {
                                // Load as music
                                if (!musicFile.loaded) {
                                    musicFile.music = LoadMusicStream(filePath);
                                    strncpy(musicFile.name, filePath, 255);
                                    musicFile.loaded = true;
                                    cout << "Loaded Music";
                                }
                            }
                        } else {
                            // Unsupported file format
                            char errorMsg[512];
                            snprintf(errorMsg, sizeof(errorMsg), "Unsupported file format: %s", GetFileName(filePath));
                            cout << "Unsupported file format";
                            isfileunsupported = true;
                        }
                    }

                    // Free memory used by dropped files
                UnloadDroppedFiles(droppedFiles);
            }

        }
        //Collision Mode
        if(isCollisionMode){

        }
        //Assets Management Mode
        if(isAssetManagementMode){

        }
        //GUI Updater
        // Update FOV and projection mode based on GUI
        camera.fovy = fov;
        camera.projection = projection;

        // Drawing logic
        BeginDrawing();
        ClearBackground(RAYWHITE);  // Clear the screen with a white background

        // Begin 3D mode for the game scene
        BeginMode3D(camera);

        // Draw shapes based on the state set by button clicks
        for (int i = 0; i < cubeCount; i++) {
            DrawCube(cubes[i].position, cubes[i].size.x, cubes[i].size.y, cubes[i].size.z, cubes[i].color);
        }
        for (int i = 0; i < sphereCount; i++) {
            DrawSphere(spheres[i].position, spheres[i].radius, spheres[i].color);
        }
        for (int i = 0; i < cylinderCount; i++) {
            DrawCylinder(cylinders[i].position, cylinders[i].radiusTop, cylinders[i].radiusBottom, cylinders[i].height, cylinders[i].slices, cylinders[i].color);
        }
        for (int i = 0; i < capsuleCount; i++) {
            DrawCapsule(capsules[i].startPos, capsules[i].endPos, capsules[i].radius, capsules[i].slices, capsules[i].rings, capsules[i].color);
        }
        for (int i = 0; i < planeCount; i++) {
            DrawPlane(planes[i].position, planes[i].size, planes[i].color);
        }
        
        // Draw grid in 3D space
        DrawUnlimitedGrid(GRID_SIZE, GRID_STEP);  // Drawing the "unlimited" grid

        // Optionally, you can add 3D objects like cubes or spheres to your scene:
        //DrawCube((Vector3){ 0.0f, 1.0f, 0.0f }, 2.0f, 2.0f, 2.0f, BLUE);

        EndMode3D();

        //Draw GUI
        DrawInfoPane(isNotInAnyMode, isCameraMode, isShapeCreationMode, isAudioMode,  isfileunsupported, isCollisionMode,
     isAssetManagementMode, &rotationSpeed, &panSpeed, &fov, &projection);
        //Draw Mode GUI
        if(isCameraMode){
            isNotInAnyMode = false;
        DrawInfoPane(isNotInAnyMode, isCameraMode, isShapeCreationMode, isAudioMode,  isfileunsupported, isCollisionMode,
     isAssetManagementMode, &rotationSpeed, &panSpeed, &fov, &projection);
        }
        if(isShapeCreationMode){
            isNotInAnyMode = false;
        DrawInfoPane(isNotInAnyMode, isCameraMode, isShapeCreationMode, isAudioMode,  isfileunsupported, isCollisionMode,
     isAssetManagementMode, &rotationSpeed, &panSpeed, &fov, &projection);
        }
        if(isAudioMode){
            isNotInAnyMode = false;

        DrawInfoPane(isNotInAnyMode, isCameraMode, isShapeCreationMode, isAudioMode,  isfileunsupported, isCollisionMode,
     isAssetManagementMode, &rotationSpeed, &panSpeed, &fov, &projection);
        }
        if(isCollisionMode){
            isNotInAnyMode = false;
        DrawInfoPane(isNotInAnyMode, isCameraMode, isShapeCreationMode, isAudioMode,  isfileunsupported, isCollisionMode,
     isAssetManagementMode, &rotationSpeed, &panSpeed, &fov, &projection);
        }
        if(isAssetManagementMode){
            isNotInAnyMode = false;
        DrawInfoPane(isNotInAnyMode, isCameraMode, isShapeCreationMode, isAudioMode,  isfileunsupported, isCollisionMode,
     isAssetManagementMode, &rotationSpeed, &panSpeed, &fov, &projection);
        }
        // Draw the blank canvas (just a white background for now)
        // DrawText("Hold RMB to Enter CAMERA Mode", 250, 20, 20, DARKGRAY);

        EndDrawing();
    
}
    // Clean up the allocated memory
    free(cubes);
    free(spheres);
    free(cylinders);
    free(capsules);
    free(planes);
    // De-initialization
    CloseWindow();  // Close window and OpenGL context

    return 0;
}
