#ifndef ELECTRON_H
#define ELECTRON_H

#include "raylib.h"
#include <vector>
#include <random>

struct ElectronParticle {
    Vector3 position;
    float alpha;
    float speed;
    float targetRadius;
    float distanceToCam;
};

// Generates the 4000 particles mapped to 1s, 2s, and 2p subshells
std::vector<ElectronParticle> GenerateElectronCloud(std::mt19937& rng, int particleCount);

// Updates particle positions and paths (rotates around Y-axis)
void UpdateElectronCloud(std::vector<ElectronParticle>& cloud, std::mt19937& rng, std::uniform_real_distribution<float>& dist, Vector3 cameraPos);

// Renders the cloud using billboard orb textures
void DrawElectronCloud(const std::vector<ElectronParticle>& cloud, Camera3D camera, Texture2D orbTexture);

#endif