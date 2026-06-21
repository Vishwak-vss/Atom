#ifndef NUCLEON_H
#define NUCLEON_H

#include "raylib.h"
#include <vector>
#include <random>

struct Nucleon {
    Vector3 position;
    Color color;
    bool isProton;
};

// Generates 8 protons and 8 neutrons tightly packed
std::vector<Nucleon> GenerateNucleus(std::mt19937& rng);

// Draws the solid nucleons without wireframe meshes
void DrawNucleus(const std::vector<Nucleon>& nucleus);

#endif