#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <cmath>
#include <random>
#include <algorithm>

struct Nucleon {
    Vector3 position;
    Color color;
    bool isProton;
};

struct ElectronParticle {
    Vector3 position;
    float alpha;
    float speed;
    float targetRadius;
    float distanceToCam;
};

// Custom GLSL shader code for crisp, anti-aliased orb particles and bloom glow
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
    InitWindow(screenWidth, screenHeight, "Oxygen Atom - High Quality Orbs");

    Camera3D camera = { 0 };
    camera.position = { 0.0f, 0.0f, 20.0f }; // Straight eye-level POV
    camera.target   = { 0.0f, 0.0f, 0.0f };
    camera.up       = { 0.0f, 1.0f, 0.0f };
    camera.fovy     = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    DisableCursor();
    SetTargetFPS(60);

    RenderTexture2D target = LoadRenderTexture(screenWidth, screenHeight);
    Shader bloomShader = LoadShaderFromMemory(0, bloomShaderCode);

    // 2. Generate Oxygen Nucleus
    std::vector<Nucleon> nucleus;
    std::mt19937 rng(1337);
    std::uniform_real_distribution<float> dist(-0.7f, 0.7f);

    int protons = 0, neutrons = 0;
    while (protons < 8 || neutrons < 8) {
        float x = dist(rng), y = dist(rng), z = dist(rng);
        if (std::sqrt(x*x + y*y + z*z) < 0.9f) {
            bool placeProton = (protons < 8) && (neutrons >= 8 || (rng() % 2 == 0));
            Nucleon n;
            n.position = { x, y, z };
            n.color = placeProton ? RED : GRAY;
            if (placeProton) protons++; else neutrons++;
            nucleus.push_back(n);
        }
    }

    // 3. Generate Electron Cloud
    const int particleCount = 4000;
    std::vector<ElectronParticle> electronCloud;
    std::normal_distribution<float> shell1s(1.2f, 0.2f);
    std::normal_distribution<float> shell2s(3.5f, 0.5f);
    std::normal_distribution<float> shell2p(5.5f, 0.8f);
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * PI);
    std::uniform_real_distribution<float> uniformDist(0.0f, 1.0f);

    for (int i = 0; i < particleCount; i++) {
        ElectronParticle p;
        float theta = angleDist(rng);
        float phi = std::acos(2.0f * uniformDist(rng) - 1.0f);
        float radius = 0.0f;

        if (i < 1000) { radius = shell1s(rng); p.alpha = 0.4f; } 
        else if (i < 2000) { radius = shell2s(rng); p.alpha = 0.25f; } 
        else {
            radius = shell2p(rng); p.alpha = 0.35f;
            int subOrbital = i % 3; 
            if (subOrbital == 0) {
                while (uniformDist(rng) > (std::cos(phi) * std::cos(phi))) phi = std::acos(2.0f * uniformDist(rng) - 1.0f);
            } else if (subOrbital == 1) {
                while (uniformDist(rng) > (std::sin(phi) * std::sin(phi) * std::cos(theta) * std::cos(theta))) {
                    phi = std::acos(2.0f * uniformDist(rng) - 1.0f); theta = angleDist(rng);
                }
            } else {
                while (uniformDist(rng) > (std::sin(phi) * std::sin(phi) * std::sin(theta) * std::sin(theta))) {
                    phi = std::acos(2.0f * uniformDist(rng) - 1.0f); theta = angleDist(rng);
                }
            }
        }
        if (radius < 0.4f) radius = 0.4f;
        p.position.x = radius * std::sin(phi) * std::cos(theta);
        p.position.y = radius * std::sin(phi) * std::sin(theta);
        p.position.z = radius * std::cos(phi);
        p.targetRadius = radius;
        p.speed = 2.5f / radius; 
        p.distanceToCam = 0.0f;
        electronCloud.push_back(p);
    }

    // GENERATING TINY ORBS: High-resolution soft-edge circular mask texture
    // This removes blocky artifacts and renders smooth circular shapes.
    const int texSize = 64;
    Image img = GenImageColor(texSize, texSize, BLANK);
    for (int y = 0; y < texSize; y++) {
        for (int x = 0; x < texSize; x++) {
            float dx = (x - texSize / 2.0f) / (texSize / 2.0f);
            float dy = (y - texSize / 2.0f) / (texSize / 2.0f);
            float distToCenter = std::sqrt(dx*dx + dy*dy);
            if (distToCenter <= 1.0f) {
                // Smooth alpha falloff toward the edges for perfect orb rendering
                float alpha = 1.0f - std::pow(distToCenter, 3.0f); 
                ImageDrawPixel(&img, x, y, ColorAlpha(WHITE, alpha));
            }
        }
    }
    Texture2D orbTexture = LoadTextureFromImage(img);
    UnloadImage(img); 
    
    // 4. Main Loop
    while (!WindowShouldClose()) {
        UpdateCamera(&camera, CAMERA_ORBITAL);

        for (auto& ep : electronCloud) {
            float speedMult = ep.speed * 0.02f;
            float cosS = std::cos(speedMult), sinS = std::sin(speedMult);
            float x = ep.position.x, z = ep.position.z;

            ep.position.x = x * cosS - z * sinS;
            ep.position.z = x * sinS + z * cosS;
            
            ep.alpha += dist(rng) * 0.01f;
            if (ep.alpha < 0.05f) ep.alpha = 0.05f;
            if (ep.alpha > 0.5f)  ep.alpha = 0.5f;

            ep.distanceToCam = Vector3Distance(ep.position, camera.position);
        }

        std::sort(electronCloud.begin(), electronCloud.end(), [](const ElectronParticle& a, const ElectronParticle& b) {
            return a.distanceToCam > b.distanceToCam;
        });

        // 5. Drawing Phase
        BeginTextureMode(target);
            ClearBackground(BLACK);
            BeginMode3D(camera);

                // Draw Cloud using our new high-res smooth orb texture
                BeginBlendMode(BLEND_ADDITIVE);
                for (const auto& p : electronCloud) {
                    Color c = { 0, 210, 255, static_cast<unsigned char>(p.alpha * 255) };
                    DrawBillboard(camera, orbTexture, p.position, 0.08f, c); // 0.08f size keeps them clean and tiny
                }
                EndBlendMode();

                // Draw Nucleus (REMOVED WIREFRAME MESH LINES)
                for (const auto& n : nucleus) {
                    DrawSphere(n.position, 0.28f, n.color); // Smooth solid color shading
                }

            EndMode3D();
        EndTextureMode();

        BeginDrawing();
            ClearBackground(BLACK);
            
            BeginShaderMode(bloomShader);
                DrawTextureRec(target.texture, (Rectangle){ 0, 0, (float)target.texture.width, (float)-target.texture.height }, (Vector2){ 0, 0 }, WHITE);
            EndShaderMode();

            // 2D Layout Overlays
            DrawFPS(10, 10);
            DrawText("Oxygen Atom (16-O) High-Fidelity Simulation", 10, 40, 20, RAYWHITE);
            DrawText("Subshells Visible: 1s, 2s, 2p (Dumbbell Orbitals)", 10, 70, 16, SKYBLUE);
        EndDrawing();
    }

    UnloadShader(bloomShader);
    UnloadTexture(orbTexture);
    UnloadRenderTexture(target);
    CloseWindow();
    return 0;
}