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

typedef enum Mode { NONE = 0, CAMERA, SHAPE_CREATETION, AUDIO, COLLISION, ASSET_MANAGEMENT } Mode;

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

//Shape Struct
typedef struct {
    Vector3 position;
    Vector3 size;
    Color color;
    bool collisionActive;  // Collision activation flag
} Cube;

typedef struct {
    Vector3 position;
    float radius;
    Color color;
    bool collisionActive;  // Collision activation flag
} Sphere;

typedef struct {
    Vector3 position;
    float radiusTop;
    float radiusBottom;
    float height;
    int slices;
    Color color;
    bool collisionActive;  // Collision activation flag
} Cylinder;

typedef struct {
    Vector3 startPos;
    Vector3 endPos;
    float radius;
    int slices;
    int rings;
    Color color;
    bool collisionActive;  // Collision activation flag
} Capsule;

typedef struct {
    Vector3 position;
    Vector2 size;
    Color color;
    bool collisionActive;  // Collision activation flag
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
    cubes[cubeCount - 1] = (Cube){ position, size, color, false }; // Default collisionActive to false
}

void AddSphere(Vector3 position, float radius, Color color) {
    sphereCount++;
    spheres = (Sphere*)realloc(spheres, sphereCount * sizeof(Sphere));
    spheres[sphereCount - 1] = (Sphere){ position, radius, color, false }; // Default collisionActive to false
}

void AddCylinder(Vector3 position, float radiusTop, float radiusBottom, float height, int slices, Color color) {
    cylinderCount++;
    cylinders = (Cylinder*)realloc(cylinders, cylinderCount * sizeof(Cylinder));
    cylinders[cylinderCount - 1] = (Cylinder){ position, radiusTop, radiusBottom, height, slices, color, false }; // Default collisionActive to false
}

void AddCapsule(Vector3 startPos, Vector3 endPos, float radius, int slices, int rings, Color color) {
    capsuleCount++;
    capsules = (Capsule*)realloc(capsules, capsuleCount * sizeof(Capsule));
    capsules[capsuleCount - 1] = (Capsule){ startPos, endPos, radius, slices, rings, color, false }; // Default collisionActive to false
}

void AddPlane(Vector3 position, Vector2 size, Color color) {
    planeCount++;
    planes = (Plane*)realloc(planes, planeCount * sizeof(Plane));
    planes[planeCount - 1] = (Plane){ position, size, color, false }; // Default collisionActive to false
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
                DrawText("Zero to Exit CAMERA Mode", panelX + 10, 390, 20, WHITE);

            } break;
            case SHAPE_CREATETION:
            {
                DrawText("Shape Creation Mode", panelX + 10, 10, 20, WHITE);
                

                 // Create Cube
                if (GuiButton((Rectangle){ 1345, 50, 350, 50 }, "Cube")) {
                    DrawText("Cube Created", 100, 200, 20, RED);
                    AddCube((Vector3){ 0.0f, 1.0f, 0.0f }, {2.0f, 2.0f, 2.0f}, BLUE);
                }
    
                // Checkbox to enable collision for the last created Cube
                if (cubeCount > 0) {
                    if (GuiCheckBox((Rectangle){ 1345, 110, 30, 30 }, " ", &cubes[cubeCount - 1].collisionActive)) {
                        // Toggle collisionActive for the last created cube
                    }
                    DrawText("Enable Collision", 1385, 110, 20, WHITE); // Text to the right of the checkbox
                }
    
                // Create Sphere
                if (GuiButton((Rectangle){ 1345, 150, 350, 50 }, "Sphere")) {
                    DrawText("Sphere Created", 100, 200, 20, RED);
                    AddSphere((Vector3){ 0.0f, 1.0f, 0.0f }, 1.5f, RED);
                }
    
                // Checkbox to enable collision for the last created Sphere
                if (sphereCount > 0) {
                    if (GuiCheckBox((Rectangle){ 1345, 210, 30, 30 }, " ", &spheres[sphereCount - 1].collisionActive)) {
                        // Toggle collisionActive for the last created sphere
                    }
                    DrawText("Enable Collision", 1385, 210, 20, WHITE); // Text to the right of the checkbox
                }

                // Create Cylinder
                if (GuiButton((Rectangle){ 1345, 250, 350, 50 }, "Cylinder")) {
                    DrawText("Cylinder Created", 100, 200, 20, RED);
                    AddCylinder((Vector3){ 0.0f, 1.0f, 0.0f }, 1.0f, 1.0f, 3.0f, 16, GREEN);
                }
    
                // Checkbox to enable collision for the last created Cylinder
                if (cylinderCount > 0) {
                    if (GuiCheckBox((Rectangle){ 1345, 310, 30, 30 }, " ", &cylinders[cylinderCount - 1].collisionActive)) {
                        // Toggle collisionActive for the last created cylinder
                    }
                    DrawText("Enable Collision", 1385, 310, 20, WHITE); // Text to the right of the checkbox
                }
    
                // Create Capsule
                if (GuiButton((Rectangle){ 1345, 350, 350, 50 }, "Capsule")) {
                    DrawText("Capsule Created", 100, 200, 20, RED);
                    AddCapsule((Vector3){ 0.0f, 1.0f, 0.0f }, (Vector3){ 0.0f, -1.0f, 0.0f }, 0.5f, 16, 8, GREEN);
                }
    
                // Checkbox to enable collision for the last created Capsule
                if (capsuleCount > 0) {
                    if (GuiCheckBox((Rectangle){ 1345, 410, 30, 30 }, " ", &capsules[capsuleCount - 1].collisionActive)) {
                        // Toggle collisionActive for the last created capsule
                    }
                    DrawText("Enable Collision", 1385, 410, 20, WHITE); // Text to the right of the checkbox
                }

                // Create Plane
                if (GuiButton((Rectangle){ 1345, 450, 350, 50 }, "Plane")) {
                    DrawText("Plane Created", 100, 200, 20, RED);
                    AddPlane((Vector3){ 0.0f, 1.0f, 0.0f }, (Vector2){ 3.0f, 3.0f }, DARKGRAY);
                }
    
                // Checkbox to enable collision for the last created Plane
                if (planeCount > 0) {
                    if (GuiCheckBox((Rectangle){ 1345, 510, 30, 30 }, " ", &planes[planeCount - 1].collisionActive)) {
                        // Toggle collisionActive for the last created plane
                    }
                    DrawText("Enable Collision", 1385, 510, 20, WHITE); // Text to the right of the checkbox
                }

                DrawText("Zero to Exit SHAPE CREATION Mode", panelX + 10, 550, 20, WHITE);
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
        
                DrawText("Zero to Exit Audio Mode", panelX + 10, screenHeight - 30, 20, WHITE);

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
                DrawText("Zero to Exit Collision Mode", panelX + 10, screenHeight - 30, 20, WHITE);
            } break;
            case ASSET_MANAGEMENT:
            {
                DrawText("Asset Management Mode", panelX + 10, 10, 20, WHITE);
                DrawText("Zero to Exit Asset Management Mode", panelX + 10, screenHeight - 30, 20, WHITE);

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
                if (IsKeyPressed(KEY_ZERO)){
                     currentMode = NONE;
                }

            } break;
            case SHAPE_CREATETION:
            {

                //Exit Mode
                if (IsKeyPressed(KEY_ZERO)){
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
                if (IsKeyPressed(KEY_ZERO)){
                     currentMode = NONE;
                }

            } break;
            case COLLISION:
            {

                //Exit Mode
                if (IsKeyPressed(KEY_ZERO)){
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
                if (IsKeyPressed(KEY_ZERO)){
                     currentMode = NONE;
                }
            } break;
            default: break;
        }
        //----------------------------------------------------------------------------------

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
    if (cubes[i].collisionActive) {
        // Draw a collision box around the cube (for simplicity, we use a wireframe cube here)
        DrawCubeWires(cubes[i].position, cubes[i].size.x, cubes[i].size.y, cubes[i].size.z, DARKGRAY);
    }
}

for (int i = 0; i < sphereCount; i++) {
    DrawSphere(spheres[i].position, spheres[i].radius, spheres[i].color);
    if (spheres[i].collisionActive) {
        // Draw a collision sphere (wireframe)
        DrawSphereWires(spheres[i].position, spheres[i].radius, 16, 16, DARKGRAY);
    }
}

for (int i = 0; i < cylinderCount; i++) {
    DrawCylinder(cylinders[i].position, cylinders[i].radiusTop, cylinders[i].radiusBottom, cylinders[i].height, cylinders[i].slices, cylinders[i].color);
    if (cylinders[i].collisionActive) {
        // Draw a collision wireframe cylinder
        DrawCylinderWires(cylinders[i].position, cylinders[i].radiusTop, cylinders[i].radiusBottom, cylinders[i].height, cylinders[i].slices, DARKGRAY);
    }
}

for (int i = 0; i < capsuleCount; i++) {
    DrawCapsule(capsules[i].startPos, capsules[i].endPos, capsules[i].radius, capsules[i].slices, capsules[i].rings, capsules[i].color);
    if (capsules[i].collisionActive) {
        // Draw a collision wireframe capsule
        DrawCapsuleWires(capsules[i].startPos, capsules[i].endPos, capsules[i].radius, capsules[i].slices, capsules[i].rings, DARKGRAY);
    }
}

for (int i = 0; i < planeCount; i++) {
    DrawPlane(planes[i].position, planes[i].size, planes[i].color);
    if (planes[i].collisionActive) {
        // Draw a collision wireframe plane (we'll use a large box for simplicity)
        DrawCubeWires(planes[i].position, planes[i].size.x, 0.1f, planes[i].size.y, DARKGRAY);
    }
}

        for (int i = 0; i < modelCount; i++) {
            DrawModel(models[i].model, models[i].position, 1.0f, WHITE);

            if (models[i].selected) {
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
        // Draw the blank canvas (just a white background for now)
        // DrawText("Hold RMB to Enter CAMERA Mode", 250, 20, 20, DARKGRAY);

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
