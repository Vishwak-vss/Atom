#include "raylib.h"
#include <vector>
#include <cmath>
#include <iostream>

// Standard constants (normalized for simulation stability)
const float DT = 0.005f;           // Timestep
const float SIGMA = 1.0f;         // Atomic diameter (size)
const float EPSILON = 1.0f;       // Binding energy depth
const float CUTOFF_SQ = 6.25f;    // 2.5 * SIGMA squared (forces drop to 0)
const float ATOM_RADIUS = 0.2f;   // Visual radius of the atom sphere

// The simulation space
const float BOX_SIZE = 12.0f;

struct Atom {
    Vector3 pos;
    Vector3 vel;
    Vector3 force;
    Color color;
};

// --- Bare Truth Physics Engine ---
void ComputeForces(std::vector<Atom>& atoms) {
    // Reset forces
    for (auto& atom : atoms) atom.force = { 0.0f, 0.0f, 0.0f };

    // Pairwise interactions (O(N^2) for simplicity; good for ~300 atoms)
    for (size_t i = 0; i < atoms.size(); ++i) {
        for (size_t j = i + 1; j < atoms.size(); ++j) {
            Vector3 delta = {
                atoms[j].pos.x - atoms[i].pos.x,
                atoms[j].pos.y - atoms[i].pos.y,
                atoms[j].pos.z - atoms[i].pos.z
            };

            // Periodic Boundary Correction (bare truth logic)
            if (delta.x > BOX_SIZE / 2) delta.x -= BOX_SIZE;
            if (delta.x < -BOX_SIZE / 2) delta.x += BOX_SIZE;
            if (delta.y > BOX_SIZE / 2) delta.y -= BOX_SIZE;
            if (delta.y < -BOX_SIZE / 2) delta.y += BOX_SIZE;
            if (delta.z > BOX_SIZE / 2) delta.z -= BOX_SIZE;
            if (delta.z < -BOX_SIZE / 2) delta.z += BOX_SIZE;

            float r2 = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;

            // Only compute if within the potential well (cutoff)
            if (r2 < CUTOFF_SQ && r2 > 0.001f) { // Added min dist check for safety
                float inv_r2 = 1.0f / r2;
                float inv_r6 = inv_r2 * inv_r2 * inv_r2;
                float inv_r12 = inv_r6 * inv_r6;

                // Lennard-Jones Force: F = 48 * epsilon * [ (sigma/r)^12 - 0.5 * (sigma/r)^6 ] / r^2
                // (Note: Optimized form pre-factors sigma and 48)
                float f_scalar = (48.0f * EPSILON * (inv_r12 - 0.5f * inv_r6) * inv_r2);

                // Cap the force scalar for live simulation stability (Prevents crash if spawned near 0)
                if (f_scalar > 1000.0f) f_scalar = 1000.0f;

                // Vector forces
                Vector3 f_vec = { f_scalar * delta.x, f_scalar * delta.y, f_scalar * delta.z };

                // Apply forces (Newton's 3rd Law)
                atoms[i].force.x -= f_vec.x; atoms[i].force.y -= f_vec.y; atoms[i].force.z -= f_vec.z;
                atoms[j].force.x += f_vec.x; atoms[j].force.y += f_vec.y; atoms[j].force.z += f_vec.z;
            }
        }
    }
}

void UpdateSimulation(std::vector<Atom>& atoms) {
    ComputeForces(atoms);

    // Integrate motion (Velocity Verlet)
    for (auto& atom : atoms) {
        atom.vel.x += atom.force.x * DT; atom.vel.y += atom.force.y * DT; atom.vel.z += atom.force.z * DT;
        atom.pos.x += atom.vel.x * DT; atom.pos.y += atom.vel.y * DT; atom.pos.z += atom.vel.z * DT;

        // Apply Periodic Boundary (wrap around box)
        if (atom.pos.x < 0) atom.pos.x += BOX_SIZE; if (atom.pos.x > BOX_SIZE) atom.pos.x -= BOX_SIZE;
        if (atom.pos.y < 0) atom.pos.y += BOX_SIZE; if (atom.pos.y > BOX_SIZE) atom.pos.y -= BOX_SIZE;
        if (atom.pos.z < 0) atom.pos.z += BOX_SIZE; if (atom.pos.z > BOX_SIZE) atom.pos.z -= BOX_SIZE;
    }
}

// --- MAIN (Live Loop) ---
int main() {
    const int screenWidth = 1200;
    const int screenHeight = 800;

    InitWindow(screenWidth, screenHeight, "Bare Truth Atom Simulator [C++ / Raylib]");
    SetTargetFPS(60);

    // Initialize 200 atoms (Species A and B)
    int num_atoms = 200;
    std::vector<Atom> atoms;
    for (int i = 0; i < num_atoms; ++i) {
        Atom a;
        a.pos = { GetRandomValue(0, (int)BOX_SIZE * 10) / 10.0f, GetRandomValue(0, (int)BOX_SIZE * 10) / 10.0f, GetRandomValue(0, (int)BOX_SIZE * 10) / 10.0f };
        a.vel = { GetRandomValue(-100, 100) / 50.0f, GetRandomValue(-100, 100) / 50.0f, GetRandomValue(-100, 100) / 50.0f };
        a.force = { 0,0,0 };
        a.color = (i < num_atoms / 2) ? SKYBLUE : LIME; // Divide into two species visually
        atoms.push_back(a);
    }

    // Set up 3D Camera
    Camera3D camera = { 0 };
    camera.position = { 15.0f, 15.0f, 15.0f };
    camera.target = { BOX_SIZE / 2, BOX_SIZE / 2, BOX_SIZE / 2 };
    camera.up = { 0.0f, 1.0f, 0.0f };
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // Optional: Keep the simulation running separately on another thread,
    // but running it here synchronizes it easily for this demo.

    // Live Execution Loop
    while (!WindowShouldClose()) {
        // Handle input (allow basic camera rotation)
        UpdateCamera(&camera, CAMERA_ORBITAL);

        // Update the physics engine (this is the live simulation step)
        UpdateSimulation(atoms);

        // --- DRAW EVERYTHING ---
        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode3D(camera);

            // Draw the simulation container box
            DrawCubeWires({ BOX_SIZE / 2, BOX_SIZE / 2, BOX_SIZE / 2 }, BOX_SIZE, BOX_SIZE, BOX_SIZE, BLACK);

            // Draw every atom live
            for (const auto& atom : atoms) {
                DrawSphere(atom.pos, ATOM_RADIUS, atom.color);
            }

        EndMode3D();

        // UI Information
        DrawFPS(10, 10);
        DrawText(TextFormat("Atoms: %i (Blue=A, Green=B)", num_atoms), 10, 30, 20, DARKGRAY);
        DrawText(TextFormat("Box Size: %.1f Units", BOX_SIZE), 10, 50, 20, DARKGRAY);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}