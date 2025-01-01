#define _CRT_SECURE_NO_WARNINGS  // Disable deprecation warnings for sprintf, fopen, etc.
#define _CRT_SECURE_NO_DEPRECATE
#define GRID_SIZE 1000  // The range of the grid lines, you can adjust this value
#define GRID_STEP 1.0f  // Step size between lines
#define MAX_AUDIO_FILES 2 //
#define RAYGUI_IMPLEMENTATION
#define MAX_MODELS 100

#include "raylib.h"
#include "raygui.h"
#include "raymath.h"
#include <stdlib.h> 
#include <string.h>
#include <iostream>


using namespace std;

const int screenWidth = 1720;
const int screenHeight = 880;

typedef enum Mode { NONE = 0, CAMERA, SHAPE_CREATETION, AUDIO, COLLISION, ASSET_MANAGEMENT, TEST } Mode;

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

// Function declarations
void DrawAxisArrows(Vector3 position, float scale);
AxisType GetAxisCollision(Vector3 position, Vector3 rayOrigin, Vector3 rayDirection, float scale);
void RecalculateModelBounds(ModelData *model);
void UpdateModelTransform(ModelData *model);

//Asset Management
    ModelData models[MAX_MODELS] = {0};
    int modelCount = 0;
    int selectedModelIndex = -1;

    AxisType activeAxis = AXIS_NONE;
    int currentAssetMode = MODE_POSITION;
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

// Audio handling structures
typedef struct {
    Sound sound;
    char name[256];
    bool loaded;
    float volume;
    float pitch;
    float pan;
    bool isPlaying;
    bool wasPlaying;  // New field to track previous playing state
} SoundFile;

typedef struct {
    Music music;
    char name[256];
    bool loaded;
    float volume;
    float pitch;
    float pan;
    bool isPlaying;
    bool wasPlaying;  // New field to track previous playing state
} MusicFile;

static SoundFile soundFiles[MAX_AUDIO_FILES] = { 0 };
static MusicFile musicFiles[MAX_AUDIO_FILES] = { 0 };
static float masterVolume = 1.0f;
static float masterSoundVolume = 1.0f;
static float masterMusicVolume = 1.0f;

void UpdateSoundParameters(SoundFile* soundFile) {
    if (soundFile->loaded) {
        float effectiveVolume = soundFile->volume * masterSoundVolume * masterVolume;
        SetSoundVolume(soundFile->sound, effectiveVolume);
        SetSoundPitch(soundFile->sound, soundFile->pitch);
        SetSoundPan(soundFile->sound, soundFile->pan);
    }
}

// Add this function to update music parameters
void UpdateMusicParameters(MusicFile* musicFile) {
    if (musicFile->loaded) {
        float effectiveVolume = musicFile->volume * masterMusicVolume * masterVolume;
        SetMusicVolume(musicFile->music, effectiveVolume);
        // Note: SetMusicPitch and SetMusicPan are not available in raylib
    }
}

bool removedSound = false;  // Flag to track if a sound was removed
int soundToRemove = -1;     // Index of sound to remove
bool removedMusic = false;  // Flag to track if a sound was removed
int musicToRemove = -1;     // Index of sound to remove

typedef enum {
    SHAPE_CUBE,
    SHAPE_SPHERE,
    SHAPE_CYLINDER,
    SHAPE_CAPSULE,
    SHAPE_PLANE
} ShapeType;


//Shape Struct
typedef struct {
    Vector3 position;
    Vector3 size;
    Color color;
    bool collisionActive;
    BoundingBox boundingBox;
    ShapeType type; // Add this field to store the shape type
} Cube;

typedef struct {
    Vector3 position;
    float radius;
    Color color;
    bool collisionActive;
    BoundingBox boundingBox;
    ShapeType type; // Add this field to store the shape type
} Sphere;

typedef struct {
    Vector3 position;
    float radiusTop;
    float radiusBottom;
    float height;
    int slices;
    Color color;
    bool collisionActive;
    BoundingBox boundingBox;
    ShapeType type; // Add this field to store the shape type
} Cylinder;

typedef struct {
    Vector3 startPos;
    Vector3 endPos;
    float radius;
    int slices;
    int rings;
    Color color;
    bool collisionActive;
    BoundingBox boundingBox;
    ShapeType type; // Add this field to store the shape type
} Capsule;

typedef struct {
    Vector3 position; // Position of the plane
    Vector2 size;     // Size of the plane (width and length, as a Vector2)
    Color color;
    bool collisionActive;
    BoundingBox boundingBox; // You may already have this in your plane struct
} Plane;



// Initialize cube collision activation state
static bool cubeCollisionActive = false; // Default: collision is disabled
static bool sphereCollisionActive = false; // Default: collision is disabled
static bool cylinderCollisionActive = false; // Default: collision is disabled
static bool capsuleCollisionActive = false; // Default: collision is disabled
static bool planeCollisionActive = false; // Default: collision is disabled

bool CheckBoundingBoxCollision(BoundingBox box1, BoundingBox box2) {
    return (box1.min.x <= box2.max.x && box1.max.x >= box2.min.x) &&
           (box1.min.y <= box2.max.y && box1.max.y >= box2.min.y) &&
           (box1.min.z <= box2.max.z && box1.max.z >= box2.min.z);
}

//global data 
float saturation = 1.0f;      // Saturation: 0-1
float lightness = 0.5f;       // Lightness: 0-1

//cubedata
Cube* cubes = NULL;
int cubeCount = 0;
float cubeposx = 0.0;
float cubeposy = 1.0;
float cubeposz = 0.0;
float cubesizex = 2.0;
float cubesizey = 2.0;
float cubesizez = 2.0;
float cubehue = 150.0;
char cubepositionInputs[3][32] = {"0.0", "1.0", "0.0"};
bool cubepositionEdit[3] = {false, false, false};
char cubesizeInputs[3][32] = {"2.0", "2.0", "2.0"};
bool cubesizeEdit[3] = {false, false, false};

// Sphere
Sphere* spheres = NULL;
int sphereCount = 0;
float sphereposx = 0.0;
float sphereposy = 1.0;
float sphereposz = 0.0;
float sphererad = 1.5;
float spherehue = 150.0;
bool radiusEdit = false;
char radiusInput[32] = "2.0";
char spherepositionInputs[3][32] = {"0.0", "1.0", "0.0"};
bool spherepositionEdit[3] = {false, false, false};

//cylinder
Cylinder* cylinders = NULL;
int cylinderCount = 0;
float cylinderposx = 0.0;
float cylinderposy = 1.0;
float cylinderposz = 0.0;
float cylinderradtop = 1.0;
float cylinderradbottom = 1.0;
float cylinderheight = 3.0;
int cylinderslices = 16;
float cylinderhue = 150.0;
char cylinderpositionInputs[3][32] = {"0.0", "1.0", "0.0"};
bool cylinderpositionEdit[3] = {false, false, false};
bool cylinderradiusEdit[2] = {false, false};
char cylinderradiusInput[2][32] = {"2.0", "2.0"};
bool cylinderheightEdit = false;
char cylinderheightInput[32] = "3";
bool cylinderslicesEdit = false;
char cylinderslicesInput[32] = "16";

//capsule
Capsule* capsules = NULL;
int capsuleCount = 0;
float capsulestartposx = 0.0;
float capsulestartposy = 1.0;
float capsulestartposz = 0.0;
float capsuleendposx = 0.0;
float capsuleendposy = -1.0;
float capsuleendposz = 0.0;
float capsulerad = 0.5f;
int capsuleslices = 16;
int capsulering = 8;
float capsulehue = 150.0;
char capsulestartpositionInputs[3][32] = {"0.0", "1.0", "0.0"};
bool capsulestartpositionEdit[3] = {false, false, false};
char capsuleendpositionInputs[3][32] = {"0.0", "-1.0", "0.0"};
bool capsuleendpositionEdit[3] = {false, false, false};
bool capsuleradiusEdit = false;
char capsuleradiusInput[32] = "0.5";
bool capsuleslicesEdit = false;
char capsuleslicesInput[32] = "16";
bool capsuleringEdit = false;
char capsuleringInput[32] = "8";

//plane
Plane* planes = NULL;
int planeCount = 0;
float planeposx = 0.0;
float planeposy = 1.0;
float planeposz = 0.0;
float planesizex = 2.0;
float planesizey = 2.0;
float planehue = 150.0;
char planepositionInputs[3][32] = {"0.0", "1.0", "0.0"};
bool planepositionEdit[3] = {false, false, false};
char planesizeInputs[2][32] = {"2.0", "2.0"};
bool planesizeEdit[2] = {false, false};


Color HSLToRGB(float hue, float saturation, float lightness) {
    float c = (1 - fabs(2 * lightness - 1)) * saturation;
    float x = c * (1 - fabs(fmod(hue / 60.0, 2) - 1));
    float m = lightness - c / 2;

    float r, g, b;
    if (hue >= 0 && hue < 60)      { r = c; g = x; b = 0; }
    else if (hue >= 60 && hue < 120)  { r = x; g = c; b = 0; }
    else if (hue >= 120 && hue < 180) { r = 0; g = c; b = x; }
    else if (hue >= 180 && hue < 240) { r = 0; g = x; b = c; }
    else if (hue >= 240 && hue < 300) { r = x; g = 0; b = c; }
    else                              { r = c; g = 0; b = x; }

    return (Color){
        (unsigned char)((r + m) * 255),
        (unsigned char)((g + m) * 255),
        (unsigned char)((b + m) * 255),
        255
    };
}

//Function to draw boundingbox
BoundingBox ComputeBoundingBoxCube(Vector3 position, Vector3 size) {
    Vector3 min = Vector3Subtract(position, Vector3Scale(size, 0.5f)); // Calculate min corner
    Vector3 max = Vector3Add(position, Vector3Scale(size, 0.5f));      // Calculate max corner
    return (BoundingBox){ min, max };
}

BoundingBox ComputeBoundingBoxSphere(Vector3 position, float radius) {
    Vector3 min = (Vector3){ position.x - radius, position.y - radius, position.z - radius };
    Vector3 max = (Vector3){ position.x + radius, position.y + radius, position.z + radius };
    return (BoundingBox){ min, max };
}

BoundingBox ComputeBoundingBoxCylinder(Vector3 position, float radiusBottom, float radiusTop, float height) {
    // Use the largest radius between the top and bottom
    float maxRadius = fmaxf(radiusBottom, radiusTop);

    // Adjust Y to align the bottom of the bounding box with the cylinder's position
    Vector3 min = {
        position.x - maxRadius,                 // Min X
        position.y - height / 2.0f,            // Min Y (aligns bottom of cylinder to bounding box)
        position.z - maxRadius                 // Min Z
    };
    Vector3 max = {
        position.x + maxRadius,                 // Max X
        position.y + height / 2.0f,             // Max Y (aligns top of cylinder to bounding box)
        position.z + maxRadius                  // Max Z
    };

    return (BoundingBox){ min, max };
}

BoundingBox ComputeBoundingBoxCapsule(Vector3 startPos, Vector3 endPos, float radius) {
    // Find the minimum and maximum corners of the capsule's bounding box
    Vector3 min = Vector3Min(startPos, endPos);
    Vector3 max = Vector3Max(startPos, endPos);

    // Expand the bounding box by the radius in all directions
    min = Vector3Subtract(min, (Vector3){ radius, radius, radius });
    max = Vector3Add(max, (Vector3){ radius, radius, radius });

    return (BoundingBox){ min, max };
}

BoundingBox ComputeBoundingBoxPlane(Vector3 position, Vector2 size) {
    // Create a bounding box based on the plane position and size (width and height)
    Vector3 min = Vector3Subtract(position, (Vector3){size.x * 0.5f, 0, size.y * 0.5f});
    Vector3 max = Vector3Add(position, (Vector3){size.x * 0.5f, 0, size.y * 0.5f});

    return (BoundingBox){min, max};
}



bool CheckCollisionBetweenShapes(void* shape1, void* shape2, ShapeType type1, ShapeType type2) {
    BoundingBox box1, box2;

    // Get bounding box for shape 1
    switch (type1) {
        case SHAPE_CUBE:
            box1 = ComputeBoundingBoxCube(((Cube*)shape1)->position, ((Cube*)shape1)->size);
            break;
        case SHAPE_SPHERE:
            box1 = ComputeBoundingBoxSphere(((Sphere*)shape1)->position, ((Sphere*)shape1)->radius);
            break;
        case SHAPE_CYLINDER:
            box1 = ComputeBoundingBoxCylinder(((Cylinder*)shape1)->position, ((Cylinder*)shape1)->radiusTop, ((Cylinder*)shape1)->radiusBottom, ((Cylinder*)shape1)->height);
            break;
        case SHAPE_CAPSULE:
            // Use startPos and endPos for Capsule
            box1 = ComputeBoundingBoxCapsule(((Capsule*)shape1)->startPos, ((Capsule*)shape1)->endPos, ((Capsule*)shape1)->radius);
            break;
        case SHAPE_PLANE:
            // Pass the full Vector2 size to ComputeBoundingBoxPlane
            box1 = ComputeBoundingBoxPlane(((Plane*)shape1)->position, ((Plane*)shape1)->size); 
            break;
    }

    // Get bounding box for shape 2
    switch (type2) {
        case SHAPE_CUBE:
            box2 = ComputeBoundingBoxCube(((Cube*)shape2)->position, ((Cube*)shape2)->size);
            break;
        case SHAPE_SPHERE:
            box2 = ComputeBoundingBoxSphere(((Sphere*)shape2)->position, ((Sphere*)shape2)->radius);
            break;
        case SHAPE_CYLINDER:
            box2 = ComputeBoundingBoxCylinder(((Cylinder*)shape2)->position, ((Cylinder*)shape2)->radiusTop, ((Cylinder*)shape2)->radiusBottom, ((Cylinder*)shape2)->height);
            break;
        case SHAPE_CAPSULE:
            // Use startPos and endPos for Capsule
            box2 = ComputeBoundingBoxCapsule(((Capsule*)shape2)->startPos, ((Capsule*)shape2)->endPos, ((Capsule*)shape2)->radius);
            break;
        case SHAPE_PLANE:
            // Pass the full Vector2 size to ComputeBoundingBoxPlane
            box2 = ComputeBoundingBoxPlane(((Plane*)shape2)->position, ((Plane*)shape2)->size); 
            break;
    }

    // Return the result of bounding box collision check
    return CheckBoundingBoxCollision(box1, box2);  // Pass by value, not by reference
}





// Function to add a cube
void AddCube(Vector3 position, Vector3 size, Color color) {
    cubeCount++;
    cubes = (Cube*)realloc(cubes, cubeCount * sizeof(Cube));
    
    // Set the shape type and bounding box
    cubes[cubeCount - 1] = (Cube){
        position,
        size,
        color,
        false, // collisionActive is initially false
        SHAPE_CUBE // Set shape type
    };
    
    // Compute and store the bounding box
    cubes[cubeCount - 1].boundingBox = ComputeBoundingBoxCube(position, size);
}

void AddSphere(Vector3 position, float radius, Color color) {
    sphereCount++;
    spheres = (Sphere*)realloc(spheres, sphereCount * sizeof(Sphere));
    
    // Set the shape type and bounding box
    spheres[sphereCount - 1] = (Sphere){
        position,
        radius,
        color,
        false, // collisionActive is initially false
        SHAPE_SPHERE // Set shape type
    };
    
    // Compute and store the bounding box
    spheres[sphereCount - 1].boundingBox = ComputeBoundingBoxSphere(position, radius);
}

void AddCylinder(Vector3 position, float radiusTop, float radiusBottom, float height, int slices, Color color) {
    cylinderCount++;
    cylinders = (Cylinder*)realloc(cylinders, cylinderCount * sizeof(Cylinder));

    // Set the shape type and bounding box
    cylinders[cylinderCount - 1] = (Cylinder){
        position,
        radiusTop,
        radiusBottom,
        height,
        slices,
        color,
        false, // collisionActive is initially false
        SHAPE_CYLINDER // Set shape type
    };

    // Compute and store the bounding box
    cylinders[cylinderCount - 1].boundingBox = ComputeBoundingBoxCylinder(position, radiusBottom, radiusTop, height);
}

void AddCapsule(Vector3 startPos, Vector3 endPos, float radius, int slices, int rings, Color color) {
    capsuleCount++;
    capsules = (Capsule*)realloc(capsules, capsuleCount * sizeof(Capsule));

    // Set the shape type and bounding box
    capsules[capsuleCount - 1] = (Capsule){
        startPos,
        endPos,
        radius,
        slices,
        rings,
        color,
        false, // collisionActive is initially false
        SHAPE_CAPSULE // Set shape type
    };

    // Compute and store the bounding box
    capsules[capsuleCount - 1].boundingBox = ComputeBoundingBoxCapsule(startPos, endPos, radius);
}

void AddPlane(Vector3 position, Vector2 size, Color color) {
    planeCount++;
    planes = (Plane*)realloc(planes, planeCount * sizeof(Plane));

    // Set the shape type and bounding box
    planes[planeCount - 1] = (Plane){
        position,
        size,
        color,
        false, // collisionActive is initially false
        SHAPE_PLANE // Set shape type
    };

    // Compute and store the bounding box
    planes[planeCount - 1].boundingBox = ComputeBoundingBoxPlane(position, size);
}

//Update boundingbox
void UpdateCubeBoundingBox(Cube* cube) {
    cube->boundingBox = ComputeBoundingBoxCube(cube->position, cube->size);
}

void UpdateSphereBoundingBox(Sphere* sphere) {
    sphere->boundingBox = ComputeBoundingBoxSphere(sphere->position, sphere->radius);
}

void UpdateCylinderBoundingBox(Cylinder* cylinder) {
    // Use the largest radius between the top and bottom
    float maxRadius = fmaxf(cylinder->radiusTop, cylinder->radiusBottom);

    // Adjust Y to align the bottom of the bounding box with the cylinder's position
    Vector3 min = {
        cylinder->position.x - maxRadius, 
        cylinder->position.y - cylinder->height / 28.0f, // Min Y (aligns bottom of cylinder to bounding box)
        cylinder->position.z - maxRadius
    };
    Vector3 max = {
        cylinder->position.x + maxRadius, 
        cylinder->position.y + cylinder->height, // 2.0f, // Max Y (aligns top of cylinder to bounding box)
        cylinder->position.z + maxRadius
    };

    cylinder->boundingBox = (BoundingBox){ min, max };
}

void UpdateCapsuleBoundingBox(Capsule* capsule) {
    Vector3 min = {
        fmin(capsule->startPos.x, capsule->endPos.x) - capsule->radius,
        fmin(capsule->startPos.y, capsule->endPos.y) - capsule->radius,
        fmin(capsule->startPos.z, capsule->endPos.z) - capsule->radius
    };
    Vector3 max = {
        fmax(capsule->startPos.x, capsule->endPos.x) + capsule->radius,
        fmax(capsule->startPos.y, capsule->endPos.y) + capsule->radius,
        fmax(capsule->startPos.z, capsule->endPos.z) + capsule->radius
    };
    capsule->boundingBox = (BoundingBox){ min, max };
}

void UpdatePlaneBoundingBox(Plane* plane) {
    Vector3 min = {
        plane->position.x - plane->size.x / 2.0f,
        plane->position.y,
        plane->position.z - plane->size.y / 2.0f
    };
    Vector3 max = {
        plane->position.x + plane->size.x / 2.0f,
        plane->position.y,
        plane->position.z + plane->size.y / 2.0f
    };
    plane->boundingBox = (BoundingBox){ min, max };
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

void DrawInfoPane(Mode currentMode, bool& isfileunsupported, float* rotationSpeed, float* panSpeed, 
                 float* fov, int* projection, 
                 SoundFile* soundFiles, MusicFile* musicFiles,  // Add these parameters
                 float& masterVolume, float& masterSoundVolume, float& masterMusicVolume)  // Add these parameters
    {
    //draw pane
    int panelWidth = 400;  // Width of the right panel
    int panelX = screenWidth - panelWidth;
    DrawRectangle(panelX, 0, panelWidth, screenHeight, BLACK);
    // GuiSetStyle()    
    GuiSetStyle(DEFAULT, TEXT_SIZE, 18);
    //mode switch
    switch(currentMode)
        {
            case NONE:
            {
                DrawText("Information Pane", panelX + 10, 10, 20, WHITE);

                DrawText("Press RMB to Enter CAMERA Mode", panelX + 10, 50, 20, WHITE);

                DrawText("Press A to Enter  ", panelX + 10, 100, 20, WHITE);
                DrawText("SHAPE CREATION Mode", panelX + 10, 120, 20, WHITE);

                DrawText("Press M to Enter ", panelX + 10, 170, 20, WHITE);
                DrawText("AUDIO Mode", panelX + 10, 190, 20, WHITE);

                DrawText("Press C to Enter ", panelX + 10, 240, 20, WHITE);
                DrawText("COLLISION Mode", panelX + 10, 260, 20, WHITE);

                DrawText("Press X to Enter ", panelX + 10, 310, 20, WHITE);
                DrawText("ASSET MANAGEMENT Mode", panelX + 10, 330, 20, WHITE);

                // DrawText("Press XX to Enter Shape CREATION Mode", panelX + 10, 80, 20, WHITE);
            } break;
            case CAMERA:
            {
                DrawText("Camera Settings", panelX + 10, 10, 20, WHITE);

                // Adjust rotation speed
                DrawText("Rotation Speed", panelX + 10, 50, 20, WHITE);
                GuiSliderBar((Rectangle){ (float)(panelX + 10), 70.0f, 380.0f, 20.0f }, NULL, NULL, rotationSpeed, 0.1f, 2.0f);

                // Adjust pan speed
                DrawText("Pan Speed", panelX + 10, 110, 20, WHITE);
                GuiSliderBar((Rectangle){ (float)(panelX + 10), 130.0f, 380.0f, 20.0f }, NULL, NULL, panSpeed, 0.001f, 0.1f);

                DrawText("Camera FOV", panelX + 10, 170, 20, WHITE);
                GuiSliderBar((Rectangle){ (float)(panelX + 10), 190.0f, 380.0f, 20.0f }, NULL, NULL, fov, 30.0f, 120.0f);

                // Camera projection mode selection
                DrawText("Projection Mode", panelX + 10, 230, 20, WHITE);
                if (GuiButton((Rectangle){ panelX + 10, 250.0f, 180.0f, 30.0f }, "Perspective")) {
                    *projection = CAMERA_PERSPECTIVE;
                }
                if (GuiButton((Rectangle){ panelX + 210, 250.0f, 180.0f, 30.0f }, "Orthographic")) {
                    *projection = CAMERA_ORTHOGRAPHIC;
                }

                // // Define the dropdown box
                // if (GuiDropdownBox((Rectangle){panelX + 10, 250.0f, 180.0f, 30.0f}, "Perspective;Orthographic;First Person;Third Person;Orbital", &activeOption, dropdownEditMode)) {
                //     dropdownEditMode = !dropdownEditMode;  // Toggle dropdown open/close state
                // }

                // // Update the projection based on the selected option
                // if (!dropdownEditMode) {
                //     if (activeOption == 0) *projection = CAMERA_PERSPECTIVE;
                //     else if (activeOption == 1) *projection = CAMERA_ORTHOGRAPHIC;
                //     else if (activeOption == 2) *projection = CAMERA_FIRST_PERSON;
                //     else if (activeOption == 3) *projection = CAMERA_THIRD_PERSON;
                //     else if (activeOption == 4) *projection = CAMERA_ORBITAL;
                // }

                // Display instructions
                DrawText("WASD To Move, QE To Up/Down,", panelX + 10, 300, 20, WHITE);
                DrawText("MouseWheel To Zoom In/Out", panelX + 10, 330, 20, WHITE);
                DrawText("Hold shift and move Mouse to pan", panelX + 10, 360, 20, WHITE);
                DrawText("F1 to Exit CAMERA Mode", panelX + 10, 390, 20, WHITE);

            } break;
            case SHAPE_CREATETION:
            {
                //DrawText("Shape Creation Mode", panelX + 10, 10, 20, WHITE);
                GuiPanel((Rectangle){panelX + 10, 10, 380, 750}, "Shape Creation Mode");

                    //Cube
                    GuiLabel((Rectangle){panelX + 130, 50, 100, 20}, "Position:");
                    for (int i = 0; i < 3; i++) {
                        if (GuiTextBox((Rectangle){panelX + 200 + (i * 55), 50, 50, 20}, cubepositionInputs[i], 32, cubepositionEdit[i])) {
                            cubepositionEdit[i] = !cubepositionEdit[i]; // Toggle edit state
                            if (!cubepositionEdit[i]) { // If edit finished, update value
                                switch (i) {
                                    case 0: cubeposx = atof(cubepositionInputs[i]); break;
                                    case 1: cubeposy = atof(cubepositionInputs[i]); break;
                                    case 2: cubeposz = atof(cubepositionInputs[i]); break;
                                }
                            }
                        }
                    }
                    GuiLabel((Rectangle){panelX + 160, 75, 100, 20}, "Size:");
                    for (int i = 0; i < 3; i++) {
                        if (GuiTextBox((Rectangle){panelX + 200 + (i * 55), 75, 50, 20}, cubesizeInputs[i], 32, cubesizeEdit[i])) {
                            cubesizeEdit[i] = !cubesizeEdit[i]; // Toggle edit state
                            if (!cubesizeEdit[i]) { // If edit finished, update value
                                switch (i) {
                                    case 0: cubesizex = atof(cubesizeInputs[i]); break;
                                    case 1: cubesizey = atof(cubesizeInputs[i]); break;
                                    case 2: cubesizez = atof(cubesizeInputs[i]); break;
                                }
                            }
                        }
                    }
                    GuiSlider((Rectangle){(float)(panelX + 200), 100.0f, 105.0f, 20.0f}, "Hue", NULL, &cubehue, 0, 360);
                    Color cubecolor = HSLToRGB(cubehue, saturation, lightness);
                    DrawRectangle(1630, 100, 50, 20, cubecolor);

                // Checkbox to enable or disable collision
                GuiCheckBox((Rectangle){panelX + 130, 125, 20, 20}, "Enable Collision", &cubeCollisionActive);

                 // Create Cube
                if (GuiButton((Rectangle){ 1340, 50, 100, 50 }, "Cube")) {
                    // DrawText("Cube Created", 100, 200, 20, RED);
                    AddCube((Vector3){ cubeposx, cubeposy, cubeposz }, {cubesizex, cubesizey, cubesizez}, cubecolor);
                    cubes[cubeCount - 1].collisionActive = cubeCollisionActive; // Set collision state
                    UpdateCubeBoundingBox(&cubes[cubeCount - 1]); // Update bounding box
                }
                                    
                //Sphere
                GuiLabel((Rectangle){panelX + 130, 150, 100, 20}, "Position:");
                    for (int i = 0; i < 3; i++) {
                        if (GuiTextBox((Rectangle){panelX + 200 + (i * 55), 150, 50, 20}, spherepositionInputs[i], 32, spherepositionEdit[i])) {
                            spherepositionEdit[i] = !spherepositionEdit[i]; // Toggle edit state
                            if (!spherepositionEdit[i]) { // If edit finished, update value
                                switch (i) {
                                    case 0: sphereposx = atof(spherepositionInputs[i]); break;
                                    case 1: sphereposy = atof(spherepositionInputs[i]); break;
                                    case 2: sphereposz = atof(spherepositionInputs[i]); break;
                                }
                            }
                        }
                    }
                    GuiLabel((Rectangle){panelX + 140, 175, 100, 20}, "Radius:");
                    
                        if (GuiTextBox((Rectangle){panelX + 145 + (1 * 55), 175, 50, 20}, radiusInput, 32, radiusEdit)) {
                            radiusEdit = !radiusEdit; // Toggle edit state
                            if (!radiusEdit) { // If edit finished, update value
                                sphererad = atof(radiusInput);
                                
                            }
                        }
                    
                    GuiSlider((Rectangle){(float)(panelX + 200), 200.0f, 105.0f, 20.0f}, "Hue", NULL, &spherehue, 0, 360);
                    Color spherecolor = HSLToRGB(spherehue, saturation, lightness);
                    DrawRectangle(1630, 200, 50, 20, spherecolor);

                    // Checkbox for collision
                    GuiCheckBox((Rectangle){panelX + 130, 225, 20, 20}, "Enable Collision", &sphereCollisionActive);

                // Create Sphere
                if (GuiButton((Rectangle){ 1340, 150, 100, 50 }, "Sphere")) {
                    AddSphere((Vector3){ sphereposx, sphereposy, sphereposz }, sphererad, spherecolor);
                    spheres[sphereCount - 1].collisionActive = sphereCollisionActive; // Apply collision state
                    UpdateSphereBoundingBox(&spheres[sphereCount - 1]); // Update bounding box
                }
    
                //cylinder
                GuiLabel((Rectangle){panelX + 130, 250, 100, 20}, "Position:");
                    for (int i = 0; i < 3; i++) {
                    if (GuiTextBox((Rectangle){panelX + 200 + (i * 55), 250, 50, 20}, cylinderpositionInputs[i], 32, cylinderpositionEdit[i])) {
                            cylinderpositionEdit[i] = !cylinderpositionEdit[i]; // Toggle edit state
                            if (!cylinderpositionEdit[i]) { // If edit finished, update value
                                switch (i) {
                                    case 0: cylinderposx = atof(cylinderpositionInputs[i]); break;
                                    case 1: cylinderposy = atof(cylinderpositionInputs[i]); break;
                                    case 2: cylinderposz = atof(cylinderpositionInputs[i]); break;
                                }
                            }
                        }
                    }
                    GuiLabel((Rectangle){panelX + 140, 275, 100, 20}, "Radius:");
                    for (int i = 0; i < 2; i++) {
                    if (GuiTextBox((Rectangle){panelX + 200 + (i * 55), 275, 50, 20}, cylinderradiusInput[i], 32,cylinderradiusEdit[i])) {
                            cylinderradiusEdit[i] = !cylinderradiusEdit[i]; // Toggle edit state
                            if (!cylinderradiusEdit[i]) { // If edit finished, update value
                                switch (i) {
                                    case 0: cylinderradtop = atof(cylinderradiusInput[i]); break;
                                    case 1: cylinderradbottom = atof(cylinderradiusInput[i]); break;
                                }
                            }
                        }
                    }   
                    GuiLabel((Rectangle){panelX + 140, 300, 100, 20}, "Height:");
                
                    if (GuiTextBox((Rectangle){panelX + 145 + (1 * 55), 300, 50, 20}, cylinderheightInput, 32, cylinderheightEdit)) {
                        cylinderheightEdit = !cylinderheightEdit; // Toggle edit state
                        if (!cylinderheightEdit) { // If edit finished, update value
                            cylinderheight = atof(cylinderheightInput);
                            
                        }
                    }

                    GuiLabel((Rectangle){panelX + 140, 325, 100, 20}, "Slices:");
                
                    if (GuiTextBox((Rectangle){panelX + 145 + (1 * 55), 325, 50, 20}, cylinderslicesInput, 32, cylinderslicesEdit)) {
                        cylinderslicesEdit = !cylinderslicesEdit; // Toggle edit state
                        if (!cylinderslicesEdit) { // If edit finished, update value
                            cylinderslices = atof(cylinderslicesInput);
                            
                        }
                    }
                GuiSlider((Rectangle){(float)(panelX + 200), 350.0f, 105.0f, 20.0f}, "Hue", NULL, &cylinderhue, 0, 360);
                Color cylindercolor = HSLToRGB(cylinderhue, saturation, lightness);
                DrawRectangle(1630, 350, 50, 20, cylindercolor);

                // Checkbox for collision
                GuiCheckBox((Rectangle){panelX + 130, 375, 20, 20}, "Enable Collision", &cylinderCollisionActive);

                // Create Cylinder
                if (GuiButton((Rectangle){ 1340, 250, 100, 50 }, "Cylinder")) {
                    AddCylinder((Vector3){cylinderposx, cylinderposy, cylinderposz }, 
                    cylinderradtop, cylinderradbottom, cylinderheight, cylinderslices, cylindercolor);
                    cylinders[cylinderCount - 1].collisionActive = cylinderCollisionActive; // Apply collision state
                    UpdateCylinderBoundingBox(&cylinders[cylinderCount - 1]); // Update bounding box
                }
                
                //capsule
            GuiLabel((Rectangle){panelX + 150, 400, 100, 20}, "Pos 1:");
                    for (int i = 0; i < 3; i++) {
                    if (GuiTextBox((Rectangle){panelX + 200 + (i * 55), 400, 50, 20}, capsulestartpositionInputs[i], 32, capsulestartpositionEdit[i])) {
                            capsulestartpositionEdit[i] = !capsulestartpositionEdit[i]; // Toggle edit state
                            if (!capsulestartpositionEdit[i]) { // If edit finished, update value
                                switch (i) {
                                    case 0: capsulestartposx = atof(capsulestartpositionInputs[i]); break;
                                    case 1: capsulestartposy = atof(capsulestartpositionInputs[i]); break;
                                    case 2: capsulestartposz = atof(capsulestartpositionInputs[i]); break;
                                }
                            }
                        }
                    }
            GuiLabel((Rectangle){panelX + 150, 425, 100, 20}, "Pos 2:");
                for (int i = 0; i < 3; i++) {
                    if (GuiTextBox((Rectangle){panelX + 200 + (i * 55), 425, 50, 20}, capsuleendpositionInputs[i], 32, capsuleendpositionEdit[i])) {
                            capsuleendpositionEdit[i] = !capsuleendpositionEdit[i]; // Toggle edit state
                            if (!capsuleendpositionEdit[i]) { // If edit finished, update value
                                switch (i) {
                                    case 0: capsuleendposx = atof(capsuleendpositionInputs[i]); break;
                                    case 1: capsuleendposy = atof(capsuleendpositionInputs[i]); break;
                                    case 2: capsuleendposz = atof(capsuleendpositionInputs[i]); break;
                                }
                            }
                        }
                    }

                GuiLabel((Rectangle){panelX + 140, 450, 100, 20}, "Radius:");
                    if (GuiTextBox((Rectangle){panelX + 145 + (1 * 55), 450, 50, 20}, capsuleradiusInput, 32, capsuleradiusEdit)) {
                        capsuleradiusEdit = !capsuleradiusEdit; // Toggle edit state
                        if (!capsuleradiusEdit) { // If edit finished, update value
                            capsulerad = atof(capsuleradiusInput);
                            
                        }
                    }
                GuiLabel((Rectangle){panelX + 150, 475, 100, 20}, "Slices:");
                
                    if (GuiTextBox((Rectangle){panelX + 145 + (1 * 55), 475, 50, 20}, capsuleslicesInput, 32, capsuleslicesEdit)) {
                        capsuleslicesEdit = !capsuleslicesEdit; // Toggle edit state
                        if (!capsuleslicesEdit) { // If edit finished, update value
                            capsuleslices = atof(capsuleslicesInput);
                            
                        }
                    }
                GuiLabel((Rectangle){panelX + 160, 500, 100, 20}, "Ring:");
                
                    if (GuiTextBox((Rectangle){panelX + 145 + (1 * 55), 500, 50, 20}, capsuleringInput, 32, capsuleringEdit)) {
                        capsuleringEdit = !capsuleringEdit; // Toggle edit state
                        if (!capsuleringEdit) { // If edit finished, update value
                            capsulering = atof(capsuleringInput);
                            
                        }
                    }
                GuiSlider((Rectangle){(float)(panelX + 200), 525.0f, 105.0f, 20.0f}, "Hue", NULL, &capsulehue, 0, 360);
                Color capsulecolor = HSLToRGB(capsulehue, saturation, lightness);
                DrawRectangle(1630, 525, 50, 20, capsulecolor);

                // Checkbox for collision
                GuiCheckBox((Rectangle){panelX + 130, 550, 20, 20}, "Enable Collision", &capsuleCollisionActive);

                // Create Capsule
                if (GuiButton((Rectangle){ 1340, 400, 100, 50 }, "Capsule")) {
                    DrawText("Capsule Created", 100, 200, 20, RED);
                    AddCapsule((Vector3){ capsulestartposx, capsulestartposy, capsulestartposz }, 
                    (Vector3){ capsuleendposx, capsuleendposy, capsuleendposz }, capsulerad, capsuleslices, capsulering, capsulecolor);
                    capsules[capsuleCount - 1].collisionActive = capsuleCollisionActive; // Apply collision state
                    UpdateCapsuleBoundingBox(&capsules[capsuleCount - 1]); // Update bounding box
                }
    
                //plane
                GuiLabel((Rectangle){panelX + 130, 575, 100, 20}, "Position:");
                    for (int i = 0; i < 3; i++) {
                    if (GuiTextBox((Rectangle){panelX + 200 + (i * 55), 575, 50, 20}, planepositionInputs[i], 32, planepositionEdit[i])) {
                            planepositionEdit[i] = !planepositionEdit[i]; // Toggle edit state
                            if (!planepositionEdit[i]) { // If edit finished, update value
                                switch (i) {
                                    case 0: planeposx = atof(planepositionInputs[i]); break;
                                    case 1: planeposy = atof(planepositionInputs[i]); break;
                                    case 2: planeposz = atof(planepositionInputs[i]); break;
                                }
                            }
                        }
                    }
                GuiLabel((Rectangle){panelX + 130, 600, 100, 20}, "Size:");
                    for (int i = 0; i < 2; i++) {
                    if (GuiTextBox((Rectangle){panelX + 200 + (i * 55), 600, 50, 20}, planesizeInputs[i], 32, planesizeEdit[i])) {
                            planesizeEdit[i] = !planesizeEdit[i]; // Toggle edit state
                            if (!planesizeEdit[i]) { // If edit finished, update value
                                switch (i) {
                                    case 0: planesizex = atof(planesizeInputs[i]); break;
                                    case 1: planesizey = atof(planesizeInputs[i]); break;
                                }
                            }
                        }
                    }
                GuiSlider((Rectangle){(float)(panelX + 200), 625.0f, 105.0f, 20.0f}, "Hue", NULL, &planehue, 0, 360);
                Color planecolor = HSLToRGB(planehue, saturation, lightness);
                DrawRectangle(1630, 625, 50, 20, planecolor);

                // Checkbox for collision
                GuiCheckBox((Rectangle){panelX + 130, 650, 20, 20}, "Enable Collision", &planeCollisionActive);

                // Create Plane
                if (GuiButton((Rectangle){ 1340, 575, 100, 50 }, "Plane")) {
                    DrawText("Plane Created", 100, 200, 20, RED);
                    AddPlane((Vector3){ planeposx, planeposy, planeposz }, (Vector2){ planesizex, planesizey }, planecolor);
                    planes[planeCount - 1].collisionActive = planeCollisionActive; // Apply collision state
                    UpdatePlaneBoundingBox(&planes[planeCount - 1]); // Update bounding box
                }

                DrawText("F1 to Exit SHAPE CREATION Mode", panelX + 10, screenHeight - 30, 20, WHITE);
            } break;
            case AUDIO:
            {
                DrawText("Audio Mode", panelX + 10, 10, 20, WHITE);

                DrawText("Drag and drop your audio files", panelX - 1300, 10, 20, BLACK);
                DrawText("Format available: .ogg, .flac, .wav, .mp3", panelX - 1300, 40, 20, BLACK);
                DrawText("Only 2 sound effects and 2 musics can be loaded at a time for this version", panelX - 1300, 70, 20, BLACK);
                DrawText("You can remove the old the and add the new one", panelX - 1300, 100, 20, BLACK);
       
                // Master Volume Controls
                DrawText("Master Volume", panelX + 10, 40, 20, WHITE);
                GuiSliderBar((Rectangle){ panelX + 10, 65, 380, 20 }, NULL, NULL, &masterVolume, 0.0f, 1.0f);
                
                // Sound Master Volume
                DrawText("Sound Master Volume", panelX + 10, 90, 20, WHITE);
                GuiSliderBar((Rectangle){ panelX + 10, 115, 380, 20 }, NULL, NULL, &masterSoundVolume, 0.0f, 1.0f);
        
                // Music Master Volume
                DrawText("Music Master Volume", panelX + 10, 140, 20, WHITE);
                GuiSliderBar((Rectangle){ panelX + 10, 165, 380, 20 }, NULL, NULL, &masterMusicVolume, 0.0f, 1.0f);
        
                // Sound Effects Section
                DrawText("Sound Effects", panelX + 10, 190, 20, WHITE);
        
                // Display loaded sounds and their controls
                int yPos = 220;
                for (int i = 0; i < MAX_AUDIO_FILES; i++) {
                    if (soundFiles[i].loaded) {
                        // Display sound name
                        DrawText(GetFileName(soundFiles[i].name), panelX + 10, yPos, 20, WHITE);
                
                        // Play/Stop button
                        if (GuiButton((Rectangle){ panelX + 10, yPos + 20, 50, 25 }, "Play")) {
                            PlaySound(soundFiles[i].sound);
                            soundFiles[i].isPlaying = true;
                            cout << "Start";

                            // Immediately apply current volume settings
                            SetSoundVolume(soundFiles[i].sound, soundFiles[i].volume * masterSoundVolume * masterVolume);
                            SetSoundPitch(soundFiles[i].sound, soundFiles[i].pitch);
                            SetSoundPan(soundFiles[i].sound, soundFiles[i].pan);
                        }

                        if (GuiButton((Rectangle){ panelX + 70, yPos + 20, 50, 25 }, "Stop")) {
                            StopSound(soundFiles[i].sound);
                            soundFiles[i].isPlaying = false;
                            cout << "stopped";
                        }

                        // Pause/Resume button
                        if (GuiButton((Rectangle){ panelX + 140, yPos + 20, 65, 25 }, "Pause")) {
                            PauseSound(soundFiles[i].sound);              
                        }
                        if (GuiButton((Rectangle){ panelX + 220, yPos + 20, 70, 25 }, "Resume")) {
                            ResumeSound(soundFiles[i].sound);
                        }
                    
                        if (GuiButton((Rectangle){ panelX + 300, yPos + 20, 70, 25 }, "Remove")) {
                            // Only remove the sound at the current index
                            // if (soundFiles[i].loaded) {
                            //     UnloadSound(soundFiles[i].sound);
                        
                            //     // Reset all properties for this specific index only
                            //     soundFiles[i].loaded = false;
                            //     // soundFiles[i].sound = LoadSound("");  // This might generate the warning but is necessary
                            //     soundFiles[i] = (SoundFile){ 0 };
                            //     soundFiles[i].volume = 1.0f;
                            //     soundFiles[i].pitch = 1.0f;
                            //     soundFiles[i].pan = 0.0f;
                            //     soundFiles[i].isPlaying = false;
                            //     soundFiles[i].wasPlaying = false;
                            //     //memset(soundFiles[i].name, 0, sizeof(soundFiles[i].name));
                        
                            //     cout << "Removed sound at index " << i << std::endl;
                            soundToRemove = i;  // Mark this index for removal
                            removedSound = true;
                        
                        }
                                                           
                        // Volume slider
                        DrawText("Volume", panelX + 10, yPos + 65, 20, WHITE);
                        GuiSliderBar((Rectangle){ panelX + 80, yPos + 45, 300, 20 }, NULL, NULL, &soundFiles[i].volume, 0.0f, 1.0f);
                        SetSoundVolume(soundFiles[i].sound, soundFiles[i].volume * masterSoundVolume * masterVolume);
                
                        // Pitch slider
                        DrawText("Pitch", panelX + 10, yPos + 70, 20, WHITE);
                        GuiSliderBar((Rectangle){ panelX + 80, yPos + 70, 300, 20 }, NULL, NULL, &soundFiles[i].pitch, 0.5f, 2.0f);
                        SetSoundPitch(soundFiles[i].sound, soundFiles[i].pitch);
                
                        // Pan slider
                        DrawText("Pan", panelX + 10, yPos + 95, 20, WHITE);
                        GuiSliderBar((Rectangle){ panelX + 80, yPos + 95, 300, 20 }, NULL, NULL, &soundFiles[i].pan, -1.0f, 1.0f);
                        SetSoundPan(soundFiles[i].sound, soundFiles[i].pan);
                
                        yPos += 130;
                    }
                }
        
                // Music Section
                DrawText("Music", panelX + 10, yPos, 20, WHITE);
        
                yPos += 40;
                for (int i = 0; i < MAX_AUDIO_FILES; i++) {
                    if (musicFiles[i].loaded) {
                        // Display music name
                        DrawText(GetFileName(musicFiles[i].name), panelX + 10, yPos, 20, WHITE);
                
                        // Display music length
                        float musicLength = GetMusicTimeLength(musicFiles[i].music);
                        char timeText[32];
                        sprintf(timeText, "Length: %.2f sec", musicLength);
                        DrawText(timeText, panelX + 10, yPos + 20, 16, WHITE);

                        if (GuiButton((Rectangle){ panelX + 10, yPos + 40, 50, 25 }, "Play")) {
                                PlayMusicStream(musicFiles[i].music);
                                musicFiles[i].isPlaying = true;
                        
                                // Immediately apply current volume settings
                                SetMusicVolume(musicFiles[i].music, musicFiles[i].volume * masterMusicVolume * masterVolume);
                                SetMusicPitch(musicFiles[i].music, musicFiles[i].pitch * masterMusicVolume * masterVolume);
                                SetMusicPan(musicFiles[i].music, musicFiles[i].pan * masterMusicVolume * masterVolume);
                        }
                        if (GuiButton((Rectangle){ panelX + 70, yPos + 40, 50, 25 }, "Stop")) {
                            StopMusicStream(musicFiles[i].music);
                            musicFiles[i].isPlaying = false;                   
                        }

                        // Pause/Resume button
                        if (GuiButton((Rectangle){ panelX + 140, yPos + 40, 65, 25 }, "Pause")) {
                            PauseMusicStream(musicFiles[i].music);       
                            musicFiles[i].isPlaying = false;        
                        }
                        if (GuiButton((Rectangle){ panelX + 220, yPos + 40, 70, 25 }, "Resume")) {
                            ResumeMusicStream(musicFiles[i].music);
                            musicFiles[i].isPlaying = true;
                        }
                        if (GuiButton((Rectangle){ panelX + 300, yPos + 40, 70, 25 }, "Remove")) {
                                // UnloadMusicStream(musicFiles[i].music);
                                // musicFiles[i].music = LoadMusicStream("");  // Load empty music stream
                                // musicFiles[i].volume = 1.0f;
                                // musicFiles[i].pitch = 1.0f;
                                // musicFiles[i].pan = 0.0f;
                                // musicFiles[i].isPlaying = false;
                                // musicFiles[i].wasPlaying = false;
                                // musicFiles[i].loaded = false;
                                musicToRemove = i;  // Mark this index for removal
                                removedMusic = true;                        
                        }
                
                
                        // Volume slider
                        DrawText("Volume", panelX + 10, yPos + 65, 20, WHITE);
                        GuiSliderBar((Rectangle){ panelX + 80, yPos + 65, 300, 20 }, NULL, NULL, &musicFiles[i].volume, 0.0f, 1.0f);
                        SetMusicVolume(musicFiles[i].music, musicFiles[i].volume * masterMusicVolume * masterVolume);
                
                        // Pitch slider
                        DrawText("Pitch", panelX + 10, yPos + 90, 20, WHITE);
                        GuiSliderBar((Rectangle){ panelX + 80, yPos + 90, 300, 20 }, NULL, NULL, &musicFiles[i].pitch, 0.5f, 2.0f);
                        SetMusicPitch(musicFiles[i].music, musicFiles[i].pitch * masterMusicVolume * masterVolume);

                        // Pan slider
                        DrawText("Pan", panelX + 10, yPos + 115, 20, WHITE);
                        GuiSliderBar((Rectangle){ panelX + 80, yPos + 115, 300, 20 }, NULL, NULL, &musicFiles[i].pan, -1.0f, 1.0f);
                        SetMusicPan(musicFiles[i].music, musicFiles[i].pan * masterMusicVolume * masterVolume);

                        // Update music stream
                        if (musicFiles[i].isPlaying) {
                            UpdateMusicStream(musicFiles[i].music);
                        }
                
                        yPos += 150;
                    }
                }
        
                DrawText("F1 to Exit Audio Mode", panelX + 10, screenHeight - 30, 20, WHITE);

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
            } break;
            case COLLISION:
            {
                DrawText("Collision Mode", panelX + 10, 10, 20, WHITE);
                DrawText("F1 to Exit Collision Mode", panelX + 10, screenHeight - 30, 20, WHITE);
            } break;
            case ASSET_MANAGEMENT:
            {
                DrawText("Asset Management Mode", panelX + 10, 10, 20, WHITE);
                DrawText("F1 to Exit Asset Management Mode", panelX + 10, screenHeight - 30, 20, WHITE);

                if (selectedModelIndex != -1) {
            ModelData *selectedModel = &models[selectedModelIndex];

            GuiPanel((Rectangle){panelX + 10, 10, 380, 350}, "Asset Management");
            GuiLabel((Rectangle){panelX + 20, 40, 200, 20}, "Transform Settings");

            GuiLabel((Rectangle){panelX + 20, 70, 100, 20}, "Position:");
            for (int i = 0; i < 3; i++) {
                if (GuiTextBox((Rectangle){panelX + 120 + (i * 55), 70, 50, 20}, positionInputs[i], 32, positionEdit[i])) {
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

            GuiLabel((Rectangle){panelX + 20, 100, 100, 20}, "Rotation:");
            for (int i = 0; i < 3; i++) {
                if (GuiTextBox((Rectangle){panelX + 120 + (i * 55), 100, 50, 20}, rotationInputs[i], 32, rotationEdit[i])) {
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

            GuiLabel((Rectangle){panelX + 20, 130, 100, 20}, "Scale:");
            for (int i = 0; i < 3; i++) {
                if (GuiTextBox((Rectangle){panelX + 120 + (i * 55), 130, 50, 20}, scaleInputs[i], 32, scaleEdit[i])) {
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

            GuiLabel((Rectangle){panelX + 20, 170, 200, 20}, TextFormat("Current Mode: %s", modes[currentAssetMode]));
            if (GuiDropdownBox((Rectangle){panelX + 20, 200, 120, 25}, "Position;Rotation;Scale", &currentAssetMode, isdropDown)) {
                isdropDown = !isdropDown;
            }           

            UpdateModelTransform(selectedModel);

            //Delete button
            if (GuiButton((Rectangle){panelX + 20, 330, 100, 30}, "Delete")) {
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
            } break;
            case TEST:
            {
                DrawText("TEST PLAY Mode", panelX + 10, 10, 20, WHITE);
                DrawText("F1 to Exit TEST PLAY Mode", panelX + 10, screenHeight - 30, 20, WHITE);
            } break;
            default: break;
        }
}

void InitializeAudioFiles() {
    for (int i = 0; i < MAX_AUDIO_FILES; i++) {
        // Sound Initialization
        soundFiles[i].volume = 1.0f;
        soundFiles[i].pitch = 1.0f;
        soundFiles[i].pan = 0.0f;
        soundFiles[i].isPlaying = false;
        soundFiles[i].wasPlaying = false;
        soundFiles[i].loaded = false;
        soundFiles[i].sound = LoadSound("");  // Load empty sound

        // Music Initialization
        musicFiles[i].volume = 1.0f;
        musicFiles[i].pitch = 1.0f;
        musicFiles[i].pan = 0.0f;
        musicFiles[i].isPlaying = false;
        musicFiles[i].wasPlaying = false;
        musicFiles[i].loaded = false;
        musicFiles[i].music = LoadMusicStream("");  // Load empty music stream
    }
}
__attribute__((constructor)) void PreMainInitialization() {
    InitializeAudioFiles();
}


int main()
{   
    InitWindow(screenWidth, screenHeight, "Game Engine by Stellar Blade");
    InitAudioDevice();

    SetMasterVolume(masterVolume);

    //Mode Switching
    Mode currentMode = NONE;    
    bool isfileunsupported = false;   

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

    SetTargetFPS(60);  // Set the game to run at 60 frames per second

    while (!WindowShouldClose())  // Detect window close button or ESC key
    {   
        // Update
        //----------------------------------------------------------------------------------
        // TODO: Update MODE variables here!
        switch(currentMode)
        {
            case NONE:
            {
                //CameraMode Trigger
                if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)){
                    currentMode = CAMERA;
                }

                //Shape Creation Mode Trigger
                if(IsKeyPressed(KEY_A)){
                    currentMode =  SHAPE_CREATETION;
                }

                 //Audio Mode
                if(IsKeyPressed(KEY_M)){
                    currentMode = AUDIO;
                }

                 //Collision Mode
                 if(IsKeyPressed(KEY_C)){
                    currentMode = COLLISION;
                }

                 //Asset Management Mode
                 if(IsKeyPressed(KEY_X)){
                    currentMode = ASSET_MANAGEMENT;
                }

                 //Test Play Mode
                 if(IsKeyPressed(KEY_T)){
                    currentMode = TEST;
                }

            } break;
            case CAMERA:
            {
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
                } 
                else {
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

                //Exit Mode
                if (IsKeyPressed(KEY_F1)){
                     currentMode = NONE;
                }

            } break;
            case SHAPE_CREATETION:
            {

                //Exit Mode
                if (IsKeyPressed(KEY_F1)){
                     currentMode = NONE;
                }
            } break;
            case AUDIO:
            {
                if (IsFileDropped()) {

                    FilePathList droppedFiles = LoadDroppedFiles();

                    for (unsigned int i = 0; i < droppedFiles.count; i++) {
                        const char *filePath = droppedFiles.paths[i];
                    
                        if (IsFileExtension(filePath, ".wav") || IsFileExtension(filePath, ".ogg") || IsFileExtension(filePath, ".flac") || IsFileExtension(filePath, ".mp3")) 
                        {
                            if (IsFileExtension(filePath, ".wav") || IsFileExtension(filePath, ".ogg") || IsFileExtension(filePath, ".flac")) {
                                // Load as sound effect
                                for (int j = 0; j < MAX_AUDIO_FILES; j++) {
                                    if (!soundFiles[j].loaded) {
                                        soundFiles[j].sound = LoadSound(filePath);
                                        strncpy(soundFiles[j].name, filePath, 255);
                                        soundFiles[j].loaded = true;
                                        soundFiles[j].volume = 1.0f;
                                        soundFiles[j].pitch = 1.0f;
                                        soundFiles[j].pan = 0.0f;
                                        soundFiles[j].isPlaying = false;
                                        break;
                                    }
                                }
                            } 
                            else if (IsFileExtension(filePath, ".mp3")) {
                                // Load as music
                                for (int j = 0; j < MAX_AUDIO_FILES; j++) {
                                    if (!musicFiles[j].loaded) {
                                        musicFiles[j].music = LoadMusicStream(filePath);
                                        strncpy(musicFiles[j].name, filePath, 255);
                                        musicFiles[j].loaded = true;
                                        musicFiles[j].volume = 1.0f;
                                        musicFiles[j].pitch = 1.0f;
                                        musicFiles[j].pan = 0.0f;
                                        musicFiles[j].isPlaying = false;                                    
                                        break;
                                    }
                                }
                            }
                        }
                        else {
                            // Unsupported file format
                            isfileunsupported = true;
                        }
                    }

                    UnloadDroppedFiles(droppedFiles);
                }

                // Update master volume
                SetMasterVolume(masterVolume);
            
                // Update all sound parameters
                for (int i = 0; i < MAX_AUDIO_FILES; i++) {
                    if (soundFiles[i].loaded) {
                        UpdateSoundParameters(&soundFiles[i]);
                    
                        // Check if sound just started playing
                        if (soundFiles[i].isPlaying && !soundFiles[i].wasPlaying) {
                            PlaySound(soundFiles[i].sound);
                        }
                        // Update previous state
                        soundFiles[i].wasPlaying = soundFiles[i].isPlaying;
                    
                        // Check if sound finished playing
                        if (!IsSoundPlaying(soundFiles[i].sound) && soundFiles[i].isPlaying) {
                            soundFiles[i].isPlaying = false;
                        }
                    }
                }
            
                // Update all music parameters
                for (int i = 0; i < MAX_AUDIO_FILES; i++) {
                    if (musicFiles[i].loaded) {
                        UpdateMusicParameters(&musicFiles[i]);
                    
                        // Check if music just started playing
                        if (musicFiles[i].isPlaying && !musicFiles[i].wasPlaying) {
                            PlayMusicStream(musicFiles[i].music);
                        }
                        // Update previous state
                        musicFiles[i].wasPlaying = musicFiles[i].isPlaying;
                    
                        // Update music stream if playing
                        if (musicFiles[i].isPlaying) {
                            UpdateMusicStream(musicFiles[i].music);
                        
                            // Check if music finished playing
                            if (!IsMusicStreamPlaying(musicFiles[i].music)) {
                                musicFiles[i].isPlaying = false;
                            }
                        }
                    }
                }

                //Exit Mode
                if (IsKeyPressed(KEY_F1)){
                     currentMode = NONE;
                }

            } break;
            case COLLISION:
            {
                //Exit Mode
                if (IsKeyPressed(KEY_F1)){
                     currentMode = NONE;
                }
            } break;
            case ASSET_MANAGEMENT:
            {
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

                    if (currentAssetMode == MODE_POSITION) {
                        if (activeAxis == AXIS_X) models[i].position.x += mouseDelta.x * 0.1f;
                        if (activeAxis == AXIS_Y) models[i].position.y -= mouseDelta.y * 0.1f;
                        if (activeAxis == AXIS_Z) models[i].position.z -= mouseDelta.x * 0.1f;
                    } else if (currentAssetMode == MODE_ROTATION) {
                        if (activeAxis == AXIS_X) models[i].rotation.x += mouseDelta.y * 0.1f;
                        if (activeAxis == AXIS_Y) models[i].rotation.y += mouseDelta.x * 0.1f;
                        if (activeAxis == AXIS_Z) models[i].rotation.z += mouseDelta.x * 0.1f;
                    } else if (currentAssetMode == MODE_SCALE) {
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

                //Exit Mode
                if (IsKeyPressed(KEY_F1)){
                     currentMode = NONE;
                }
            } break;
            case TEST:
            {
                Vector2 mouseDelta = GetMouseDelta();  // Get the mouse delta
                DisableCursor();
                

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
                    //if (fabsf(Vector3Angle(direction, camera.up) - PI/2) > PI/3) {
                    //    camera.position.y = camera.target.y; // Reset to avoid flipping
                    //}

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

                //Exit Mode
                if (IsKeyPressed(KEY_F1)){
                     currentMode = NONE;
                     ShowCursor();
                }
            } break;
            default: break;
        }
        //----------------------------------------------------------------------------------

        //GUI Updater
        //Update FOV and projection mode based on GUI
        camera.fovy = fov;
        camera.projection = projection;

        // Drawing logic
        BeginDrawing();
        ClearBackground(RAYWHITE);  // Clear the screen with a white background
        
        // Begin 3D mode for the game scene
        BeginMode3D(camera);

        // Draw shapes based on the state set by button clicks
        // for (int i = 0; i < cubeCount; i++) {
        //     if (cubes[i].collisionActive)
        //         DrawBoundingBox(cubes[i].boundingBox, RED); // Use stored bounding box
        //         DrawCube(cubes[i].position, cubes[i].size.x, cubes[i].size.y, cubes[i].size.z, cubes[i].color);
        // }

        // for (int i = 0; i < sphereCount; i++) {
        //     if (spheres[i].collisionActive) 
        //         DrawBoundingBox(spheres[i].boundingBox, RED); // Use stored bounding box
        //         DrawSphere(spheres[i].position, spheres[i].radius, spheres[i].color);
        // }

        // for (int i = 0; i < cylinderCount; i++) {
        //     if (cylinders[i].collisionActive) 
        //         DrawBoundingBox(cylinders[i].boundingBox, RED); // Highlight the bounding box
        //         DrawCylinder(
        //             cylinders[i].position,
        //             cylinders[i].radiusTop,
        //             cylinders[i].radiusBottom,
        //             cylinders[i].height,
        //             cylinders[i].slices,
        //             cylinders[i].color
        //         );
        // }

        // for (int i = 0; i < capsuleCount; i++) {
        //     if (capsules[i].collisionActive)
        //         DrawBoundingBox(capsules[i].boundingBox, RED); // Highlight the bounding box
        //         DrawCapsule(
        //             capsules[i].startPos,
        //             capsules[i].endPos,
        //             capsules[i].radius,
        //             capsules[i].slices,
        //             capsules[i].rings,
        //             capsules[i].color
        //         );
        // }

        // for (int i = 0; i < planeCount; i++) {
        //     if (planes[i].collisionActive) 
        //         DrawBoundingBox(planes[i].boundingBox, RED); // Highlight the bounding box
        //         DrawPlane(
        //             planes[i].position,
        //             planes[i].size,
        //             planes[i].color
        //         );
        // }

       // Check collision for all shapes by comparing bounding boxes
// Iterate through cubes and check for collisions
for (int i = 0; i < cubeCount; i++) {
    // Always draw the cube, regardless of collision state
    DrawCube(cubes[i].position, cubes[i].size.x, cubes[i].size.y, cubes[i].size.z, cubes[i].color);

    // Draw the bounding box only when collision is active
    if (cubes[i].collisionActive) {
        DrawBoundingBox(cubes[i].boundingBox, RED); // Draw bounding box in red (or any color for active collision)
    }

    // Check for collisions with other cubes only if both are collision active
    for (int j = i + 1; j < cubeCount; j++) {
        if (cubes[j].collisionActive && cubes[i].collisionActive) {
            if (CheckBoundingBoxCollision(cubes[i].boundingBox, cubes[j].boundingBox)) {
                cubes[i].color = RED; // Change color on collision
                cubes[j].color = RED; // Change color on collision
            }
        }
    }
}

// Iterate through spheres and check for collisions
for (int i = 0; i < sphereCount; i++) {
    // Always draw the sphere, regardless of collision state
    DrawSphere(spheres[i].position, spheres[i].radius, spheres[i].color);

    // Draw the bounding box only when collision is active
    if (spheres[i].collisionActive) {
        DrawBoundingBox(spheres[i].boundingBox, RED); // Draw bounding box in red (or any color for active collision)
    }

    // Check for collisions with other spheres only if both are collision active
    for (int j = i + 1; j < sphereCount; j++) {
        if (spheres[j].collisionActive && spheres[i].collisionActive) {
            if (CheckBoundingBoxCollision(spheres[i].boundingBox, spheres[j].boundingBox)) {
                spheres[i].color = RED; // Change color on collision
                spheres[j].color = RED; // Change color on collision
            }
        }
    }
}

// Iterate through cylinders and check for collisions
for (int i = 0; i < cylinderCount; i++) {
    // Always draw the cylinder, regardless of collision state
    DrawCylinder(cylinders[i].position, cylinders[i].radiusTop, cylinders[i].radiusBottom, cylinders[i].height, cylinders[i].slices, cylinders[i].color);

    // Draw the bounding box only when collision is active
    if (cylinders[i].collisionActive) {
        DrawBoundingBox(cylinders[i].boundingBox, RED); // Draw bounding box in red (or any color for active collision)
    }

    // Check for collisions with other cylinders only if both are collision active
    for (int j = i + 1; j < cylinderCount; j++) {
        if (cylinders[j].collisionActive && cylinders[i].collisionActive) {
            if (CheckBoundingBoxCollision(cylinders[i].boundingBox, cylinders[j].boundingBox)) {
                cylinders[i].color = RED; // Change color on collision
                cylinders[j].color = RED; // Change color on collision
            }
        }
    }
}

// Iterate through capsules and check for collisions
for (int i = 0; i < capsuleCount; i++) {
    // Always draw the capsule, regardless of collision state
    DrawCapsule(capsules[i].startPos, capsules[i].endPos, capsules[i].radius, capsules[i].slices, capsules[i].rings, capsules[i].color);

    // Draw the bounding box only when collision is active
    if (capsules[i].collisionActive) {
        DrawBoundingBox(capsules[i].boundingBox, RED); // Draw bounding box in red (or any color for active collision)
    }

    // Check for collisions with other capsules only if both are collision active
    for (int j = i + 1; j < capsuleCount; j++) {
        if (capsules[j].collisionActive && capsules[i].collisionActive) {
            if (CheckBoundingBoxCollision(capsules[i].boundingBox, capsules[j].boundingBox)) {
                capsules[i].color = RED; // Change color on collision
                capsules[j].color = RED; // Change color on collision
            }
        }
    }
}

// Iterate through planes and check for collisions
for (int i = 0; i < planeCount; i++) {
    // Always draw the plane, regardless of collision state
    DrawPlane(planes[i].position, planes[i].size, planes[i].color);

    // Draw the bounding box only when collision is active
    if (planes[i].collisionActive) {
        DrawBoundingBox(planes[i].boundingBox, RED); // Draw bounding box in red (or any color for active collision)
    }

    // Check for collisions with other planes only if both are collision active
    for (int j = i + 1; j < planeCount; j++) {
        if (planes[j].collisionActive && planes[i].collisionActive) {
            if (CheckBoundingBoxCollision(planes[i].boundingBox, planes[j].boundingBox)) {
                planes[i].color = RED; // Change color on collision
                planes[j].color = RED; // Change color on collision
            }
        }
    }
}

        for (int i = 0; i < modelCount; i++) {
            DrawModel(models[i].model, models[i].position, 1.0f, WHITE);

            if (models[i].selected && currentMode == ASSET_MANAGEMENT) {
                DrawBoundingBox(models[i].bounds, GREEN);
                DrawAxisArrows(models[i].position, 100.0f);
            }
        }

        // Draw grid in 3D space
        DrawUnlimitedGrid(GRID_SIZE, GRID_STEP);  // Drawing the "unlimited" grid

        // Optionally, you can add 3D objects like cubes or spheres to your scene:
        //DrawCube((Vector3){ 0.0f, 1.0f, 0.0f }, 2.0f, 2.0f, 2.0f, BLUE);

        EndMode3D();
        
        //Draw GUI
        DrawInfoPane(currentMode, isfileunsupported, &rotationSpeed, &panSpeed, 
                    &fov, &projection, 
                    soundFiles, musicFiles,
                    masterVolume, masterSoundVolume, masterMusicVolume);

        //Remove Sound and Music main logic
            if (removedSound && soundToRemove >= 0) {
                UnloadSound(soundFiles[soundToRemove].sound);
                soundFiles[soundToRemove].sound = LoadSound("");
                soundFiles[soundToRemove].loaded = false;
                soundFiles[soundToRemove].isPlaying = false;
                soundFiles[soundToRemove].wasPlaying = false;
                soundFiles[soundToRemove].volume = 1.0f;
                soundFiles[soundToRemove].pitch = 1.0f;
                soundFiles[soundToRemove].pan = 0.0f;
                soundToRemove = -1;
            }
            if (removedMusic && musicToRemove >= 0) {
                UnloadMusicStream(musicFiles[musicToRemove].music);
                musicFiles[musicToRemove].music = LoadMusicStream("");  // Load empty music stream
                musicFiles[musicToRemove].volume = 1.0f;
                musicFiles[musicToRemove].pitch = 1.0f;
                musicFiles[musicToRemove].pan = 0.0f;
                musicFiles[musicToRemove].isPlaying = false;
                musicFiles[musicToRemove].wasPlaying = false;
                musicFiles[musicToRemove].loaded = false;
                musicToRemove = -1;
            }
            
        EndDrawing();
    
}

    // Clean up the allocated memory
    free(cubes);
    free(spheres);
    free(cylinders);
    free(capsules);
    free(planes);
    
    CloseAudioDevice();
    // De-initialization
    for (int i = 0; i < modelCount; i++) {
        UnloadModel(models[i].model);
    }
    CloseWindow();  // Close window and OpenGL context

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
