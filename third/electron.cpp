#include "electron.h"
#include "raylib.h"
#include "raymath.h"
#include <cmath>
#include <algorithm>
#include "rlgl.h"

std::vector<ElectronParticle> GenerateElectronCloud(std::mt19937& rng, int particleCount) {
    std::vector<ElectronParticle> cloud;
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
        cloud.push_back(p);
    }
    return cloud;
}

void UpdateElectronCloud(std::vector<ElectronParticle>& cloud, std::mt19937& rng, std::uniform_real_distribution<float>& dist, Vector3 cameraPos) {
    for (auto& ep : cloud) {
        float speedMult = ep.speed * 0.02f;
        float cosS = std::cos(speedMult), sinS = std::sin(speedMult);
        float x = ep.position.x, z = ep.position.z;

        ep.position.x = x * cosS - z * sinS;
        ep.position.z = x * sinS + z * cosS;
        
        ep.alpha += dist(rng) * 0.01f;
        if (ep.alpha < 0.05f) ep.alpha = 0.05f;
        if (ep.alpha > 0.5f)  ep.alpha = 0.5f;

        ep.distanceToCam = Vector3Distance(ep.position, cameraPos);
    }

    // Depth-Sort transparency layout pass
    std::sort(cloud.begin(), cloud.end(), [](const ElectronParticle& a, const ElectronParticle& b) {
        return a.distanceToCam > b.distanceToCam;
    });
}

void DrawElectronCloud(const std::vector<ElectronParticle>& cloud, Camera3D camera, Texture2D orbTexture) {
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);
    for (const auto& p : cloud) {
        Color c = { 0, 210, 255, static_cast<unsigned char>(p.alpha * 255) };
        DrawBillboard(camera, orbTexture, p.position, 0.08f, c);
    }
    EndBlendMode();
    rlEnableDepthMask();
}