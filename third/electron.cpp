#include "electron.h"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>
#include <algorithm>

const float a0 = 1.0f; 

// --- Mathematical Wavefunctions (\psi) ---
float Psi1s(float r, float theta, float phi) {
    return 2.0f * std::pow(1.0f / a0, 1.5f) * std::exp(-r / a0) * (1.0f / std::sqrt(4.0f * PI));
}

float Psi2s(float r, float theta, float phi) {
    return (1.0f / std::sqrt(2.0f)) * std::pow(1.0f / a0, 1.5f) * (1.0f - r / (2.0f * a0)) * std::exp(-r / (2.0f * a0)) * (1.0f / std::sqrt(4.0f * PI));
}

float Psi2pz(float r, float theta, float phi) {
    return (1.0f / std::sqrt(24.0f)) * std::pow(1.0f / a0, 1.5f) * (r / a0) * std::exp(-r / (2.0f * a0)) * (std::sqrt(3.0f / (4.0f * PI)) * std::cos(theta));
}

float Psi2px(float r, float theta, float phi) {
    return (1.0f / std::sqrt(24.0f)) * std::pow(1.0f / a0, 1.5f) * (r / a0) * std::exp(-r / (2.0f * a0)) * (std::sqrt(3.0f / (4.0f * PI)) * std::sin(theta) * std::cos(phi));
}

float Psi2py(float r, float theta, float phi) {
    return (1.0f / std::sqrt(24.0f)) * std::pow(1.0f / a0, 1.5f) * (r / a0) * std::exp(-r / (2.0f * a0)) * (std::sqrt(3.0f / (4.0f * PI)) * std::sin(theta) * std::sin(phi));
}

// --- Space-Partitioned Metropolis Sampler ---
Vector3 SampleBoundedOrbital(std::mt19937& rng, float (*Psi)(float, float, float), float minR, float maxR, OrbitalType type) {
    std::uniform_real_distribution<float> stepDist(-0.4f, 0.4f); 
    std::uniform_real_distribution<float> acceptDist(0.0f, 1.0f);
    
    // NEW: Randomize the initial direction on a unit sphere using basic trig
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * PI);
    std::uniform_real_distribution<float> uDist(-1.0f, 1.0f);
    
    float theta = angleDist(rng);
    float u = uDist(rng);
    float sqrtOneMinusU2 = std::sqrt(1.0f - u*u);
    
    // Scale the random unit vector slightly above the minimum radius boundary
    float startR = minR + 0.1f;
    Vector3 current = {
        startR * sqrtOneMinusU2 * std::cos(theta),
        startR * sqrtOneMinusU2 * std::sin(theta),
        startR * u
    }; 

    // Override specific axes alignments for 2p states to make sure they fall into high-probability zones immediately
    if (type == ORBITAL_2PX) current.x = (current.x > 0 ? 1.0f : -1.0f) * startR;
    if (type == ORBITAL_2PY) current.y = (current.y > 0 ? 1.0f : -1.0f) * startR;
    if (type == ORBITAL_2PZ) current.z = (current.z > 0 ? 1.0f : -1.0f) * startR;

    for (int step = 0; step < 60; step++) {
        Vector3 proposal = { current.x + stepDist(rng), current.y + stepDist(rng), current.z + stepDist(rng) };
        
        float r_prop = std::sqrt(proposal.x*proposal.x + proposal.y*proposal.y + proposal.z*proposal.z);
        
        if (r_prop < minR || r_prop > maxR) {
            continue; 
        }

        float r_curr = std::sqrt(current.x*current.x + current.y*current.y + current.z*current.z);
        float theta_curr = std::acos(current.z / (r_curr + 1e-6f));
        float phi_curr = std::atan2(current.y, current.x);

        float theta_prop = std::acos(proposal.z / (r_prop + 1e-6f));
        float phi_prop = std::atan2(proposal.y, proposal.x);
        
        float p_curr = std::pow(Psi(r_curr, theta_curr, phi_curr), 2.0f);
        float p_prop = std::pow(Psi(r_prop, theta_prop, phi_prop), 2.0f);
        
        if (p_curr == 0.0f || acceptDist(rng) < (p_prop / p_curr)) {
            current = proposal;
        }
    }
    return current;
}
std::vector<ElectronParticle> GenerateQuantumCloud(std::mt19937& rng, int particleCount) {
    std::vector<ElectronParticle> cloud;
    
    int p1s  = particleCount * 0.250f;
    int p2s  = particleCount * 0.250f;
    int p2px = particleCount * 0.250f; // Paired shell (Dense)
    int p2py = particleCount * 0.125f; // Unpaired shell (Sparse)
    int p2pz = particleCount * 0.125f; // Unpaired shell (Sparse)

    for (int i = 0; i < particleCount; i++) {
        ElectronParticle p;
        
        if (i < p1s) {
            p.type = ORBITAL_1S;
            p.position = SampleBoundedOrbital(rng, Psi1s, 1.1f, 2.2f, p.type);
            p.alpha = 0.40f;
        } 
        else if (i < p1s + p2s) {
            p.type = ORBITAL_2S;
            p.position = SampleBoundedOrbital(rng, Psi2s, 2.2f, 4.8f, p.type); 
            p.alpha = 0.22f;
        } 
        else if (i < p1s + p2s + p2px) {
            p.type = ORBITAL_2PX;
            p.position = SampleBoundedOrbital(rng, Psi2px, 0.4f, 8.5f, p.type);
            p.alpha = 0.35f; 
        } 
        else if (i < p1s + p2s + p2px + p2py) {
            p.type = ORBITAL_2PY;
            p.position = SampleBoundedOrbital(rng, Psi2py, 0.4f, 8.5f, p.type);
            p.alpha = 0.20f; 
        } 
        else {
            p.type = ORBITAL_2PZ;
            p.position = SampleBoundedOrbital(rng, Psi2pz, 0.4f, 8.5f, p.type);
            p.alpha = 0.20f; 
        }
        
        p.distanceToCam = 0.0f;
        cloud.push_back(p);
    }
    return cloud;
}

void UpdateQuantumCloud(std::vector<ElectronParticle>& cloud, Vector3 cameraPos) {
    for (auto& ep : cloud) {
        ep.distanceToCam = Vector3Distance(ep.position, cameraPos);
    }
    std::sort(cloud.begin(), cloud.end(), [](const ElectronParticle& a, const ElectronParticle& b) {
        return a.distanceToCam > b.distanceToCam;
    });
}

void DrawElectronCloud(const std::vector<ElectronParticle>& cloud, Camera3D camera, Texture2D orbTexture) {
    rlDisableDepthMask(); 
    BeginBlendMode(BLEND_ADDITIVE);
    
    float time = GetTime();
    
    for (const auto& p : cloud) {
        Color c;

        float quantumFlicker = 0.8f + 0.2f * std::sin(time * 3.0f + p.position.x * p.position.y);
        float targeAlpha = p.alpha * quantumFlicker;   
        unsigned char brightAlpha = static_cast<unsigned char>(fminf(targeAlpha * 255.f, 255.0f));
        switch (p.type) {
            case ORBITAL_1S:  c = { 50, 180, 255, brightAlpha };  break; // Deep Royal Blue
            case ORBITAL_2S:  c = { 0, 255, 255, brightAlpha };  break; // FIXED: True Cyan
            case ORBITAL_2PX: c = { 255, 0, 150, brightAlpha };  break; // Magenta (X axis)
            case ORBITAL_2PY: c = { 200, 50, 255, brightAlpha }; break; // Purple (Y axis)
            case ORBITAL_2PZ: c = { 255, 160, 0, brightAlpha };  break; // Amber (Z axis)
        }
        DrawBillboard(camera, orbTexture, p.position, 0.07f, c);
    }
    
    EndBlendMode();
    rlEnableDepthMask(); 
}

void DrawOrbitalBoundaries(Camera3D camera) {
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ALPHA);

    // Dropped alpha down to 8 and 5 out of 255 for a whisper-thin containment look
    DrawSphere({ 0.0f, 0.0f, 0.0f }, 2.2f, Color{ 0, 120, 255, 8 });
    DrawSphere({ 0.0f, 0.0f, 0.0f }, 4.8f, Color{ 0, 230, 240, 5 });

    EndBlendMode();
    rlEnableDepthMask();
}