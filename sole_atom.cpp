#include "raylib.h"
#include <vector>
#include <cmath>
#include <random>

// Structure for Nucleons (Protons and Neutrons)
struct Nucleon {
    Vector3 position;
    Color color;
    bool isProton;
};

// Structure for Electron Cloud Particles
struct ElectronParticle {
    Vector3 position;
    float alpha;
    float speed;
    float targetRadius;
};

int main() {
    // 1. Initialization
    const int screenWidth = 1200;
    const int screenHeight = 800;
    
    // Configures anti-aliasing for smoother looking spheres/wires
    SetConfigFlags(FLAG_MSAA_4X_HINT); 
    InitWindow(screenWidth, screenHeight, "Oxygen Atom Simulation - Nucleus & Electron Cloud");

    // Define 3D Camera
    Camera3D camera = { 0 };
    camera.position = { 0.0f, 0.0f, 20.0f }; // Straight point of view
    camera.target   = { 0.0f, 0.0f, 0.0f };
    camera.up       = { 0.0f, 1.0f, 0.0f };
    camera.fovy     = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    DisableCursor(); // Limit cursor to window for camera controls
    SetTargetFPS(60);

    // 2. Generate Oxygen Nucleus (8 Protons, 8 Neutrons)
    std::vector<Nucleon> nucleus;
    std::mt19937 rng(1337); // Fixed seed for reproducible nucleus structure
    std::uniform_real_distribution<float> dist(-0.7f, 0.7f);

    int protons = 0;
    int neutrons = 0;
    while (protons < 8 || neutrons < 8) {
        float x = dist(rng);
        float y = dist(rng);
        float z = dist(rng);
        
        // Keep them tightly packed in a spherical core
        if (std::sqrt(x*x + y*y + z*z) < 0.9f) {
            bool placeProton = (protons < 8) && (neutrons >= 8 || (rng() % 2 == 0));
            
            Nucleon n;
            n.position = { x, y, z };
            if (placeProton) {
                n.color = RED; // Proton = Red
                n.isProton = true;
                protons++;
            } else {
                n.color = GRAY; // Neutron = Gray
                n.isProton = false;
                neutrons++;
            }
            nucleus.push_back(n);
        }
    }

    // 3. Generate Electron Cloud (Actual Quantum Orbitals)
    const int particleCount = 4000;
    std::vector<ElectronParticle> electronCloud;
    
    std::normal_distribution<float> shell1s(1.2f, 0.2f); // Tight core sphere
    std::normal_distribution<float> shell2s(3.5f, 0.5f); // Larger outer sphere
    std::normal_distribution<float> shell2p(5.5f, 0.8f); // Outer dumbbell lobes
    
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * PI);
    std::uniform_real_distribution<float> uniformDist(0.0f, 1.0f);

    for (int i = 0; i < particleCount; i++) {
        ElectronParticle p;
        float theta = angleDist(rng);
        float phi = std::acos(2.0f * uniformDist(rng) - 1.0f); // Uniform spherical distribution baseline
        float radius = 0.0f;

        // Distribute particles based on Oxygen's configuration: 1s^2, 2s^2, 2p^4
        if (i < 1000) {
            // --------- 1s SUBSHELL (2e-) ---------
            radius = shell1s(rng);
            p.alpha = 0.4f;
        } 
        else if (i < 2000) {
            // --------- 2s SUBSHELL (2e-) ---------
            radius = shell2s(rng);
            p.alpha = 0.25f;
        } 
        else {
            // --------- 2p SUBSHELL (4e-) ---------
            radius = shell2p(rng);
            p.alpha = 0.35f;

            int subOrbital = i % 3; 
            if (subOrbital == 0) {
                // 2pz Orbital
                while (uniformDist(rng) > (std::cos(phi) * std::cos(phi))) {
                    phi = std::acos(2.0f * uniformDist(rng) - 1.0f);
                }
            } 
            else if (subOrbital == 1) {
                // 2px Orbital
                while (uniformDist(rng) > (std::sin(phi) * std::sin(phi) * std::cos(theta) * std::cos(theta))) {
                    phi = std::acos(2.0f * uniformDist(rng) - 1.0f);
                    theta = angleDist(rng);
                }
            } 
            else {
                // 2py Orbital
                while (uniformDist(rng) > (std::sin(phi) * std::sin(phi) * std::sin(theta) * std::sin(theta))) {
                    phi = std::acos(2.0f * uniformDist(rng) - 1.0f);
                    theta = angleDist(rng);
                }
            }
        }

        if (radius < 0.4f) radius = 0.4f;

        p.position.x = radius * std::sin(phi) * std::cos(theta);
        p.position.y = radius * std::sin(phi) * std::sin(theta);
        p.position.z = radius * std::cos(phi);
        
        p.targetRadius = radius;
        p.speed = 2.5f / radius; 
        electronCloud.push_back(p);
    }

    // Generate a radial gradient for smooth electron particles (FIXED TYPO & COLORS HERE)
    Image image = GenImageGradientRadial(16, 16, 0.0f, WHITE, BLACK); 
    Texture2D particleTexture = LoadTextureFromImage(image);
    UnloadImage(image); 
    
    // 4. Main Game Loop
    while (!WindowShouldClose()) {
        // Update Camera
        UpdateCamera(&camera, CAMERA_ORBITAL);

        // Update Electron Cloud (Simulate dynamic uncertainty/orbitals)
        for (auto& ep : electronCloud) {
            float speedMult = ep.speed * 0.02f;
            
            float cosS = std::cos(speedMult);
            float sinS = std::sin(speedMult);

            float x = ep.position.x;
            float z = ep.position.z;

            // Rotate slightly around Y axis to simulate dynamic clouds
            ep.position.x = x * cosS - z * sinS;
            ep.position.z = x * sinS + z * cosS;
            
            // Slight quantum flicker
            ep.alpha += dist(rng) * 0.01f;
            if (ep.alpha < 0.05f) ep.alpha = 0.05f;
            if (ep.alpha > 0.5f)  ep.alpha = 0.5f;
        }

        // 5. Drawing
        BeginDrawing();
            ClearBackground(BLACK);

            BeginMode3D(camera);

                // Draw Electron Cloud (Rendered first with blending)
                BeginBlendMode(BLEND_ADDITIVE);
                for (const auto& p : electronCloud) {
                    Color c = { 0, 228, 255, static_cast<unsigned char>(p.alpha * 255) };
                    DrawBillboard(camera, particleTexture, p.position, 0.08f, c);
                }
                EndBlendMode();

                // Draw Nucleus Internal Structure
                for (const auto& n : nucleus) {
                    DrawSphere(n.position, 0.28f, n.color);
                    DrawSphereWires(n.position, 0.28f, 24, 24, Color{ 20, 20, 20, 80 });
                }

            EndMode3D();

            // 2D Overlay Info / Legend
            DrawFPS(10, 10);
            DrawText("Oxygen Atom (16-O) Simulation", 10, 40, 20, RAYWHITE);
            
            // Legend
            DrawCircle(20, 90, 8, RED);
            DrawText("Protons (8)", 35, 82, 16, RAYWHITE);
            
            DrawCircle(20, 115, 8, GRAY);
            DrawText("Neutrons (8)", 35, 107, 16, RAYWHITE);
            
            DrawCircle(20, 140, 6, SKYBLUE);
            DrawText("Electron Cloud Density (8e-)", 35, 132, 16, RAYWHITE);

            DrawText("Controls: Use Mouse to Orbit/Pan Camera around Atom", 10, screenHeight - 30, 16, LIGHTGRAY);

        EndDrawing();
    }

    UnloadTexture(particleTexture); // Clean up texture resource
    CloseWindow();
    return 0;
}