#include "raylib.h"
#include "raymath.h"
#include <stdlib.h>

#define MAX_MODELS 100

// Structure to store model data
typedef struct ModelData {
    Model model;
    Texture2D texture;
    Vector3 position;
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

    ModelData models[MAX_MODELS];
    int modelCount = 0;
    AxisType activeAxis = AXIS_NONE;
    bool isDragging = false;

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        // Update
        Vector3 rayOrigin = GetMouseRay(GetMousePosition(), camera).position;
        Vector3 rayDirection = GetMouseRay(GetMousePosition(), camera).direction;

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
                } else {
                    models[i].selected = false;
                }
            }

            if (models[i].selected) {
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    activeAxis = GetAxisCollision(models[i].position, rayOrigin, rayDirection, 2.0f);
                    if (activeAxis != AXIS_NONE) isDragging = true;
                }

                if (isDragging && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                    Vector2 mouseDelta = GetMouseDelta();

                    if (activeAxis == AXIS_X) models[i].position.x += mouseDelta.x * 0.1f;
                    if (activeAxis == AXIS_Y) models[i].position.y -= mouseDelta.y * 0.1f;
                    if (activeAxis == AXIS_Z) models[i].position.z -= mouseDelta.x * 0.1f;

                    RecalculateModelBounds(&models[i]);
                   
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
