#include "nucleon.h"
#include <cmath>

std::vector<Nucleon> GenerateNucleus(std::mt19937& rng) {
    std::vector<Nucleon> nucleus;
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
    return nucleus;
}

void DrawNucleus(const std::vector<Nucleon>& nucleus) {
    // 1. Draw a pristine, semi-transparent boundary sphere to depict the nucleus container
    // A radius of 1.1f neatly encloses our packed protons and neutrons
    DrawSphere({ 0.0f, 0.0f, 0.0f }, 1.1f, Color{ 255, 255, 255, 40 }); 
    
    // 2. Draw the solid nucleons inside it
    for (const auto& n : nucleus) {
        DrawSphere(n.position, 0.28f, n.color);
    }
}