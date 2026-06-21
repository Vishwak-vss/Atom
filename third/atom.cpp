#include "raylib.h"
#include "nucleon.h"
#include "electron.h"

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
    "    finalColor = original + sum * 0.8;\n"
    "}\n";

int main() {
    const int screenWidth = 1200;
    const int screenHeight = 800;
    
    SetConfigFlags(FLAG_MSAA_4X_HINT); 
    InitWindow(screenWidth, screenHeight, "Oxygen Atom - Refactored");

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

    // Call modular setup components
    std::vector<Nucleon> nucleus = GenerateNucleus(rng);
    std::vector<ElectronParticle> electronCloud = GenerateElectronCloud(rng, 4000);

    // Generate high-resolution soft orb texture
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
        UpdateCamera(&camera, CAMERA_ORBITAL);
        
        // Update module positions
        float customTimeStep = 0.02f;
        UpdateElectronCloud(electronCloud, rng, dist, camera.position, GetFrameTime() * customTimeStep);
        // UpdateElectronCloud(electronCloud, rng, dist, camera.position, 0.06f);

        // Rendering Pipeline Pass
        BeginTextureMode(target);
            ClearBackground(BLACK);
            BeginMode3D(camera);
                DrawNucleus(nucleus);
                DrawElectronCloud(electronCloud, camera, orbTexture);
                
            EndMode3D();
        EndTextureMode();

        BeginDrawing();
            ClearBackground(BLACK);
            BeginShaderMode(bloomShader);
                DrawTextureRec(target.texture, (Rectangle){ 0, 0, (float)target.texture.width, (float)-target.texture.height }, (Vector2){ 0, 0 }, WHITE);
            EndShaderMode();

            DrawFPS(10, 10);
            DrawText("Oxygen Atom (16-O) Modular High-Fidelity Simulation", 10, 40, 20, RAYWHITE);
        EndDrawing();
    }

    UnloadShader(bloomShader);
    UnloadTexture(orbTexture);
    UnloadRenderTexture(target);
    CloseWindow();
    return 0;
}