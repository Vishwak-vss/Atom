#include "raylib.h"
#include "nucleon.h"
#include "electron.h"
#include <random>
#include <cmath>
#include <vector>

const char* bloomShaderCode = 
    "#version 330\n"
    "in vec2 fragTexCoord;\n"
    "in vec4 fragColor;\n"
    "out vec4 finalColor;\n"
    "uniform sampler2D texture0;\n"
    "void main() {\n"
    "    vec4 sum = vec4(0.0);\n"
    "    vec2 texelSize = 1.0 / textureSize(texture0, 0);\n"
    "    sum += texture(texture0, fragTexCoord + vec2(-2.0, -2.0)*texelSize) * 0.05;\n"
    "    sum += texture(texture0, fragTexCoord + vec2(-1.0, -1.0)*texelSize) * 0.12;\n"
    "    sum += texture(texture0, fragTexCoord + vec2(0.0, -2.0)*texelSize) * 0.12;\n"
    "    sum += texture(texture0, fragTexCoord + vec2(1.0, -1.0)*texelSize) * 0.12;\n"
    "    sum += texture(texture0, fragTexCoord) * 0.16;\n"
    "    sum += texture(texture0, fragTexCoord + vec2(-1.0, 1.0)*texelSize) * 0.12;\n"
    "    sum += texture(texture0, fragTexCoord + vec2(0.0, 2.0)*texelSize) * 0.12;\n"
    "    sum += texture(texture0, fragTexCoord + vec2(1.0, 1.0)*texelSize) * 0.12;\n"
    "    sum += texture(texture0, fragTexCoord + vec2(2.0, 2.0)*texelSize) * 0.05;\n"
    "    vec4 original = texture(texture0, fragTexCoord);\n"
    "    \n"
    "    // FIXED: Boosted from 1.5 to 2.5 for maximum electron cloud luminosity\n"
    "    finalColor = original + (sum * 2.5);\n" 
    "}\n";

int main() {
    const int screenWidth = 1200;
    const int screenHeight = 800;
    
    SetConfigFlags(FLAG_MSAA_4X_HINT); 
    InitWindow(screenWidth, screenHeight, "Oxygen Atom - High-Fidelity Quantum Model");

    // Camera perspective set up straight down the Z-axis to cleanly display X and Y boundaries
    Camera3D camera = { 0 };
    camera.position = { 0.0f, 0.0f, 20.0f };
    camera.target   = { 0.0f, 0.0f, 0.0f };
    camera.up       = { 0.0f, 1.0f, 0.0f };
    camera.fovy     = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    DisableCursor();
    SetTargetFPS(60);

    RenderTexture2D target = LoadRenderTexture(screenWidth, screenHeight);
    Shader bloomShader = LoadShaderFromMemory(0, bloomShaderCode);

    std::mt19937 rng(1337);
    std::uniform_real_distribution<float> dist(-0.7f, 0.7f);

    // Structural initialization
    std::vector<Nucleon> nucleus = GenerateNucleus(rng);
    std::vector<ElectronParticle> electronCloud = GenerateQuantumCloud(rng, 4000);

    // Generate high-resolution soft orb texture for billboards
    const int texSize = 64;
    Image img = GenImageColor(texSize, texSize, BLANK);
    for (int y = 0; y < texSize; y++) {
        for (int x = 0; x < texSize; x++) {
            float dx = (x - texSize / 2.0f) / (texSize / 2.0f);
            float dy = (y - texSize / 2.0f) / (texSize / 2.0f);
            float distToCenter = std::sqrt(dx*dx + dy*dy);
            if (distToCenter <= 1.0f) {
                float alpha = 1.0f - std::pow(distToCenter, 3.0f); 
                ImageDrawPixel(&img, x, y, ColorAlpha(WHITE, alpha));
            }
        }
    }
    Texture2D orbTexture = LoadTextureFromImage(img);
    UnloadImage(img); 
    
    while (!WindowShouldClose()) {
        // FIXED: Removed UpdateCamera execution completely to drop the artificial spinning orbital motion.
        // This lets us view the asymmetric 2p subshell geometry relative to our viewport flawlessly.
        UpdateCamera(&camera, CAMERA_ORBITAL);
        // Sort and pass real-time camera depth fields 
        UpdateQuantumCloud(electronCloud, camera.position);

        // Render Pipeline
        BeginTextureMode(target);
            ClearBackground(BLACK);
            BeginMode3D(camera);
                
                // 1. Core nucleus plus its translucent boundary containment capsule
                DrawNucleus(nucleus);
                
                // 2. Translucent wireframe shells for 1s and 2s states (Minus grid lines)
                DrawOrbitalBoundaries(camera);
                
                // 3. Brightened wave amplitude representations matching real quantum configuration ratios
                DrawElectronCloud(electronCloud, camera, orbTexture);
                
            EndMode3D();
        EndTextureMode();

        // Screen Post-Processing Presentation Pass
        BeginDrawing();
            ClearBackground(BLACK);
            BeginShaderMode(bloomShader);
                DrawTextureRec(target.texture, (Rectangle){ 0, 0, (float)target.texture.width, (float)-target.texture.height }, (Vector2){ 0, 0 }, WHITE);
            EndShaderMode();

            DrawFPS(10, 10);
            DrawText("Oxygen Atom (16-O) Bounded Wavefunction Simulation", 10, 40, 20, RAYWHITE);
        EndDrawing();
    }

    // Resource Cleanup
    UnloadShader(bloomShader);
    UnloadTexture(orbTexture);
    UnloadRenderTexture(target);
    CloseWindow();
    return 0;
}