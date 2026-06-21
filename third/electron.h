#ifndef ELECTRON_H
#define ELECTRON_H

#include "raylib.h"
#include <vector>
#include <random>

// Enumerate the precise subshells of an Oxygen Atom
enum OrbitalType {
    ORBITAL_1S,
    ORBITAL_2S,
    ORBITAL_2PX,
    ORBITAL_2PY,
    ORBITAL_2PZ
};

struct ElectronParticle {
    Vector3 position;
    float alpha;
    float distanceToCam;
    OrbitalType type; // Track exact identity
};

std::vector<ElectronParticle> GenerateQuantumCloud(std::mt19937& rng, int particleCount);
void UpdateQuantumCloud(std::vector<ElectronParticle>& cloud, Vector3 cameraPos);
void DrawElectronCloud(const std::vector<ElectronParticle>& cloud, Camera3D camera, Texture2D orbTexture);
// Renders the mathematical boundary shapes for each orbital zone
void DrawOrbitalBoundaries(Camera3D camera);
#endif