#define RAYGUI_IMPLEMENTATION
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE

#include "raylib.h"
#include "raymath.h"
#include "raygui.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX_MODELS 100


// Structure to store model data
typedef struct ModelData {
    Model model;
    Texture2D texture;
    Vector3 position;
    Vector3 rotation; // For rotation (in degrees)
    Vector3 scale = {1,1,1};    // For scaling
    BoundingBox bounds;
    bool selected;
} ModelData;

// Enum for axis types
typedef enum AxisType {
    AXIS_NONE,
    AXIS_X,
    AXIS_Y,
    AXIS_Z
} AxisType;

typedef enum TransformMode {
    MODE_POSITION = 0,
    MODE_ROTATION,
    MODE_SCALE
} TransformMode;

// Function declarations
void DrawAxisArrows(Vector3 position, float scale);
AxisType GetAxisCollision(Vector3 position, Vector3 rayOrigin, Vector3 rayDirection, float scale);
void RecalculateModelBounds(ModelData *model);
void UpdateModelTransform(ModelData *model);

int main(void) {
    // Initialization
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "raylib [models] example - models loading");

    Camera camera = {0};
    camera.position = (Vector3){50.0f, 50.0f, 50.0f};
    camera.target = (Vector3){0.0f, 10.0f, 0.0f};
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    ModelData models[MAX_MODELS] = {0};
    int modelCount = 0;
    int selectedModelIndex = -1;

    AxisType activeAxis = AXIS_NONE;
    int currentMode = MODE_POSITION;
    bool isDragging = false;

    char positionInputs[3][32] = {"0.0", "0.0", "0.0"};
    char rotationInputs[3][32] = {"0.0", "0.0", "0.0"};
    char scaleInputs[3][32] = {"1.0", "1.0", "1.0"};
    bool positionEdit[3] = {false, false, false};
    bool rotationEdit[3] = {false, false, false};
    bool scaleEdit[3] = {false, false, false};
    const char *modes[3] = {"Position", "Rotation", "Scale"};
    bool isdropDown = false;

    bool isMouseInGUI = false;

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        // Update
        Vector3 rayOrigin = GetMouseRay(GetMousePosition(), camera).position;
        Vector3 rayDirection = GetMouseRay(GetMousePosition(), camera).direction;

        isMouseInGUI = CheckCollisionPointRec(GetMousePosition(), (Rectangle){10, 10, 250, 300});

        // Handle file drop
        if (IsFileDropped()) {
            FilePathList droppedFiles = LoadDroppedFiles();

            if (modelCount < MAX_MODELS && droppedFiles.count == 1) {
                if (IsFileExtension(droppedFiles.paths[0], ".obj") ||
                    IsFileExtension(droppedFiles.paths[0], ".gltf") ||
                    IsFileExtension(droppedFiles.paths[0], ".glb") ||
                    IsFileExtension(droppedFiles.paths[0], ".vox") ||
                    IsFileExtension(droppedFiles.paths[0], ".iqm") ||
                    IsFileExtension(droppedFiles.paths[0], ".m3d")) {

                    models[modelCount].model = LoadModel(droppedFiles.paths[0]);
                    models[modelCount].position = (Vector3){0.0f, 0.0f, 0.0f};
                    models[modelCount].bounds = GetMeshBoundingBox(models[modelCount].model.meshes[0]);
                    models[modelCount].selected = false;

                    modelCount++;
                }
            }

            if (modelCount > 0 && droppedFiles.count == 1) {
                if (IsFileExtension(droppedFiles.paths[0], ".png") ||
                    IsFileExtension(droppedFiles.paths[0], ".jpg") ||
                    IsFileExtension(droppedFiles.paths[0], ".bmp") ||
                    IsFileExtension(droppedFiles.paths[0], ".tga")) {

                    // Assign texture to the selected model
                    for (int i = 0; i < modelCount; i++) {
                        if (models[i].selected) {
                        models[i].texture = LoadTexture(droppedFiles.paths[0]);
                        models[i].model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = models[i].texture;
                    }
                    }
                    
                }
            }

            UnloadDroppedFiles(droppedFiles);
        }

        // Handle model selection and dragging
        for (int i = 0; i < modelCount; i++) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (GetRayCollisionBox(GetMouseRay(GetMousePosition(), camera), models[i].bounds).hit) {
                    models[i].selected = true; //!models[i].selected;
                    selectedModelIndex = i;

                    snprintf(positionInputs[0], 32, "%.2f", models[i].position.x);
                    snprintf(positionInputs[1], 32, "%.2f", models[i].position.y);
                    snprintf(positionInputs[2], 32, "%.2f", models[i].position.z);

                    snprintf(rotationInputs[0], 32, "%.2f", models[i].rotation.x);
                    snprintf(rotationInputs[1], 32, "%.2f", models[i].rotation.y);
                    snprintf(rotationInputs[2], 32, "%.2f", models[i].rotation.z);

                    snprintf(scaleInputs[0], 32, "%.2f", models[i].scale.x);
                    snprintf(scaleInputs[1], 32, "%.2f", models[i].scale.y);
                    snprintf(scaleInputs[2], 32, "%.2f", models[i].scale.z);
                } 
                else if (models[i].selected && isMouseInGUI)
                {
                    selectedModelIndex = i;
                }                
                else {
                    models[i].selected = false;
                    selectedModelIndex = -1;
                }
            }

            if (models[i].selected) {
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    activeAxis = GetAxisCollision(models[i].position, rayOrigin, rayDirection, 2.0f);
                    if (activeAxis != AXIS_NONE) isDragging = true;
                }

                if (isDragging && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                    Vector2 mouseDelta = GetMouseDelta();

                    if (currentMode == MODE_POSITION) {
                        if (activeAxis == AXIS_X) models[i].position.x += mouseDelta.x * 0.1f;
                        if (activeAxis == AXIS_Y) models[i].position.y -= mouseDelta.y * 0.1f;
                        if (activeAxis == AXIS_Z) models[i].position.z -= mouseDelta.x * 0.1f;
                    } else if (currentMode == MODE_ROTATION) {
                        if (activeAxis == AXIS_X) models[i].rotation.x += mouseDelta.y * 0.1f;
                        if (activeAxis == AXIS_Y) models[i].rotation.y += mouseDelta.x * 0.1f;
                        if (activeAxis == AXIS_Z) models[i].rotation.z += mouseDelta.x * 0.1f;
                    } else if (currentMode == MODE_SCALE) {
                        if (activeAxis == AXIS_X) models[i].scale.x += mouseDelta.x * 0.01f;
                        if (activeAxis == AXIS_Y) models[i].scale.y += mouseDelta.y * 0.01f;
                        if (activeAxis == AXIS_Z) models[i].scale.z += mouseDelta.x * 0.01f;
                    }

                    snprintf(positionInputs[0], 32, "%.2f", models[i].position.x);
                    snprintf(positionInputs[1], 32, "%.2f", models[i].position.y);
                    snprintf(positionInputs[2], 32, "%.2f", models[i].position.z);

                    snprintf(rotationInputs[0], 32, "%.2f", models[i].rotation.x);
                    snprintf(rotationInputs[1], 32, "%.2f", models[i].rotation.y);
                    snprintf(rotationInputs[2], 32, "%.2f", models[i].rotation.z);

                    snprintf(scaleInputs[0], 32, "%.2f", models[i].scale.x);
                    snprintf(scaleInputs[1], 32, "%.2f", models[i].scale.y);
                    snprintf(scaleInputs[2], 32, "%.2f", models[i].scale.z);

                    UpdateModelTransform(&models[i]);                   
                }

                if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                    isDragging = false;
                    activeAxis = AXIS_NONE;
                }
            }
        }

        // Draw
        BeginDrawing();

        ClearBackground(RAYWHITE);

        BeginMode3D(camera);

        DrawGrid(20, 10.0f);

        for (int i = 0; i < modelCount; i++) {
            DrawModel(models[i].model, models[i].position, 1.0f, WHITE);

            if (models[i].selected) {
                DrawBoundingBox(models[i].bounds, GREEN);
                DrawAxisArrows(models[i].position, 100.0f);
            }
        }

        EndMode3D();

        if (selectedModelIndex != -1) {
            ModelData *selectedModel = &models[selectedModelIndex];

            GuiPanel((Rectangle){10, 10, 250, 300}, "Asset Management");
            GuiLabel((Rectangle){20, 20, 200, 20}, "Transform Settings");

            GuiLabel((Rectangle){20, 50, 100, 20}, "Position:");
            for (int i = 0; i < 3; i++) {
                if (GuiTextBox((Rectangle){100 + (i * 55), 50, 50, 20}, positionInputs[i], 32, positionEdit[i])) {
                    positionEdit[i] = !positionEdit[i];
                    switch (i)
                    {
                    case 0:
                        selectedModel->position.x = atof(positionInputs[i]);
                        break;
                    case 1:
                        selectedModel->position.y = atof(positionInputs[i]);
                        break;
                    case 2:
                        selectedModel->position.z = atof(positionInputs[i]);
                        break;
                    default:
                        break;
                    }
                }
            }

            GuiLabel((Rectangle){20, 80, 100, 20}, "Rotation:");
            for (int i = 0; i < 3; i++) {
                if (GuiTextBox((Rectangle){100 + (i * 55), 80, 50, 20}, rotationInputs[i], 32, rotationEdit[i])) {
                    rotationEdit[i] = !rotationEdit[i];
                    switch (i)
                    {
                    case 0:
                        selectedModel->rotation.x = atof(rotationInputs[i]);
                        break;
                    case 1:
                        selectedModel->rotation.y = atof(rotationInputs[i]);
                        break;
                    case 2:
                        selectedModel->rotation.z = atof(rotationInputs[i]);
                        break;
                    default:
                        break;
                    }
                }
            }

            GuiLabel((Rectangle){20, 110, 100, 20}, "Scale:");
            for (int i = 0; i < 3; i++) {
                if (GuiTextBox((Rectangle){100 + (i * 55), 110, 50, 20}, scaleInputs[i], 32, scaleEdit[i])) {
                    scaleEdit[i] = !scaleEdit[i];
                    switch (i)
                    {
                    case 0:
                        selectedModel->scale.x = atof(scaleInputs[i]);
                        break;
                    case 1:
                        selectedModel->scale.y = atof(scaleInputs[i]);
                        break;
                    case 2:
                        selectedModel->scale.z = atof(scaleInputs[i]);
                        break;
                    default:
                        break;
                    }
                }
             }

            GuiLabel((Rectangle){20, 150, 200, 20}, TextFormat("Current Mode: %s", modes[currentMode]));
            if (GuiDropdownBox((Rectangle){20, 180, 100, 20}, "Position;Rotation;Scale", &currentMode, isdropDown)) {
                isdropDown = !isdropDown;
            }           

            UpdateModelTransform(selectedModel);

            //Delete button
            if (GuiButton((Rectangle){20, 270, 100, 30}, "Delete")) {
                UnloadModel(selectedModel->model);
                if (models[selectedModelIndex].texture.id != 0) {
                    UnloadTexture(models[selectedModelIndex].texture);
                }

                for (int i = selectedModelIndex; i < modelCount - 1; i++) {
                    models[i] = models[i + 1];
                }
                models[modelCount - 1] = (ModelData){0};
                modelCount--;
                selectedModelIndex = -1;
            }
        }

        DrawText("Drag & drop models to load them.", 10, screenHeight - 20, 10, DARKGRAY);
        DrawFPS(10, 10);

        EndDrawing();
    }

    // De-Initialization
    for (int i = 0; i < modelCount; i++) {
        UnloadModel(models[i].model);
    }

    CloseWindow();

    return 0;
}

// Function to draw axis arrows for model movement
void DrawAxisArrows(Vector3 position, float scale) {
    // Draw a thick X-axis as a red rectangle (cube)
    Vector3 xStart = position;
    //Vector3 xEnd = Vector3Add(position, (Vector3){scale * 1.0f, 0.0f, 0.0f});
    Vector3 xSize = (Vector3){scale * 1.0f, 2.0f, 2.0f}; // Thick X-axis rectangle
    DrawCubeV(xStart, xSize, RED); // X-axis

    // Draw a thick Y-axis as a green rectangle (cube)
    Vector3 yStart = position;
    //Vector3 yEnd = Vector3Add(position, (Vector3){0.0f, scale * 1.0f, 0.0f});
    Vector3 ySize = (Vector3){2.0f, scale * 1.0f, 2.0f}; // Thick Y-axis rectangle
    DrawCubeV(yStart, ySize, GREEN); // Y-axis

    // Draw a thick Z-axis as a blue rectangle (cube)
    Vector3 zStart = position;
    //Vector3 zEnd = Vector3Add(position, (Vector3){0.0f, 0.0f, scale * 1.0f});
    Vector3 zSize = (Vector3){2.0f, 2.0f, scale * 1.0f}; // Thick Z-axis rectangle
    DrawCubeV(zStart, zSize, BLUE); // Z-axis
    
}

// Function to detect collision with axis arrows
AxisType GetAxisCollision(Vector3 position, Vector3 rayOrigin, Vector3 rayDirection, float scale) {
    Ray ray = {rayOrigin, rayDirection};
    
    // Increase the size of the bounding boxes for more accurate collision detection
    BoundingBox xBox = {
        Vector3Add(position, (Vector3){-scale * 20, -2.0f, -2.0f}),
        Vector3Add(position, (Vector3){scale * 20, 2.0f, 2.0f})
    };
    BoundingBox yBox = {
        Vector3Add(position, (Vector3){-2.0f, -scale * 20, -2.0f}),
        Vector3Add(position, (Vector3){2.0f, scale * 20, 2.0f})
    };
    BoundingBox zBox = {
        Vector3Add(position, (Vector3){-2.0f, -2.0f, -scale * 20}),
        Vector3Add(position, (Vector3){2.0f, 2.0f, scale * 20})
    };

    if (GetRayCollisionBox(ray, xBox).hit) return AXIS_X;
    if (GetRayCollisionBox(ray, yBox).hit) return AXIS_Y;
    if (GetRayCollisionBox(ray, zBox).hit) return AXIS_Z;

    return AXIS_NONE;
}

void RecalculateModelBounds(ModelData *model) {
    // Update the model's bounding box based on the new position
    model->bounds = GetMeshBoundingBox(model->model.meshes[0]);
    model->bounds.min = Vector3Add(model->bounds.min, model->position);
    model->bounds.max = Vector3Add(model->bounds.max, model->position);
}

void UpdateModelTransform(ModelData *model) {

    // Create a scaling matrix
    Matrix scaleMatrix = MatrixScale(model->scale.x, model->scale.y, model->scale.z);
    
    // Create a rotation matrix (rotation order: XYZ)
    Matrix rotationMatrix = MatrixRotateXYZ((Vector3){
        DEG2RAD * model->rotation.x,
        DEG2RAD * model->rotation.y,
        DEG2RAD * model->rotation.z
    });
    
    // Create a translation matrix
    //Matrix translationMatrix = MatrixTranslate(model->position.x, model->position.y, model->position.z);
    
    // Combine the transformations: Scale -> Rotate -> Translate
    //model->model.transform = MatrixMultiply(MatrixMultiply(scaleMatrix, rotationMatrix), translationMatrix);
    model->model.transform = MatrixMultiply(scaleMatrix, rotationMatrix);
    RecalculateModelBounds(model);
}
