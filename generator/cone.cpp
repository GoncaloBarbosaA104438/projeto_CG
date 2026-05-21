#define _USE_MATH_DEFINES
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include "cone.hpp"

void generateCone(const std::string& fileName, float radius, float height, int slices, int stacks)
{
    std::ofstream out(fileName);
    if (!out.is_open()) { std::cerr << "Erro ao abrir: " << fileName << "\n"; return; }

    int baseVerts    = slices * 3;
    int lateralVerts = (stacks - 1) * slices * 6 + slices * 3;
    int totalVertices = baseVerts + lateralVerts;
    out << totalVertices << "\n";

    float angleStep = 2.0f * M_PI / slices;
    // Precompute lateral normal magnitude for normalisation
    float lateralLen = sqrt(height * height + radius * radius);

    // Base (y=0, normal pointing down)
    for (int i = 0; i < slices; ++i) {
        float t1 = i * angleStep, t2 = (i + 1) * angleStep;
        float c1 = cos(t1), s1 = sin(t1);
        float c2 = cos(t2), s2 = sin(t2);
        // Center
        out << "0 0 0 0 -1 0 0.5 0.5\n";
        // p1 - UV radial
        float u1 = 0.5f + 0.5f * c1, v1 = 0.5f + 0.5f * s1;
        out << radius*c1 << " 0 " << radius*s1 << " 0 -1 0 " << u1 << " " << v1 << "\n";
        // p2
        float u2 = 0.5f + 0.5f * c2, v2 = 0.5f + 0.5f * s2;
        out << radius*c2 << " 0 " << radius*s2 << " 0 -1 0 " << u2 << " " << v2 << "\n";
    }

    // Lateral surface
    for (int i = 0; i < stacks; ++i) {
        float yCur = i * (height / stacks);
        float rCur = radius * (1.0f - (float)i / stacks);
        float yNxt = (i < stacks - 1) ? (i + 1) * (height / stacks) : height;
        float rNxt = (i < stacks - 1) ? radius * (1.0f - (float)(i + 1) / stacks) : 0.0f;

        float vCur = (float)i / stacks;
        float vNxt = (float)(i + 1) / stacks;

        for (int j = 0; j < slices; ++j) {
            float t1 = j * angleStep, t2 = (j + 1) * angleStep;
            float c1 = cos(t1), s1 = sin(t1);
            float c2 = cos(t2), s2 = sin(t2);

            // Lateral normals: n = (H*cos(θ), R, H*sin(θ)) / lateralLen
            float nx1 = height * c1 / lateralLen, ny = radius / lateralLen, nz1 = height * s1 / lateralLen;
            float nx2 = height * c2 / lateralLen,                            nz2 = height * s2 / lateralLen;

            float uCur1 = (float)j / slices, uCur2 = (float)(j + 1) / slices;

            float pC1x = rCur*c1, pC1y = yCur, pC1z = rCur*s1;
            float pC2x = rCur*c2, pC2y = yCur, pC2z = rCur*s2;
            float pN1x = rNxt*c1, pN1y = yNxt, pN1z = rNxt*s1;
            float pN2x = rNxt*c2, pN2y = yNxt, pN2z = rNxt*s2;

            if (i < stacks - 1) {
                // Quad: 2 triangles
                out << pC1x<<" "<<pC1y<<" "<<pC1z<<" "<<nx1<<" "<<ny<<" "<<nz1<<" "<<uCur1<<" "<<vCur<<"\n";
                out << pC2x<<" "<<pC2y<<" "<<pC2z<<" "<<nx2<<" "<<ny<<" "<<nz2<<" "<<uCur2<<" "<<vCur<<"\n";
                out << pN1x<<" "<<pN1y<<" "<<pN1z<<" "<<nx1<<" "<<ny<<" "<<nz1<<" "<<uCur1<<" "<<vNxt<<"\n";

                out << pC2x<<" "<<pC2y<<" "<<pC2z<<" "<<nx2<<" "<<ny<<" "<<nz2<<" "<<uCur2<<" "<<vCur<<"\n";
                out << pN2x<<" "<<pN2y<<" "<<pN2z<<" "<<nx2<<" "<<ny<<" "<<nz2<<" "<<uCur2<<" "<<vNxt<<"\n";
                out << pN1x<<" "<<pN1y<<" "<<pN1z<<" "<<nx1<<" "<<ny<<" "<<nz1<<" "<<uCur1<<" "<<vNxt<<"\n";
            } else {
                // Apex triangle - apex normal interpolated
                float nApexX = (nx1 + nx2) * 0.5f, nApexZ = (nz1 + nz2) * 0.5f;
                float uApex = (uCur1 + uCur2) * 0.5f;
                out << pC1x<<" "<<pC1y<<" "<<pC1z<<" "<<nx1<<" "<<ny<<" "<<nz1<<" "<<uCur1<<" "<<vCur<<"\n";
                out << pC2x<<" "<<pC2y<<" "<<pC2z<<" "<<nx2<<" "<<ny<<" "<<nz2<<" "<<uCur2<<" "<<vCur<<"\n";
                out << "0 "<<height<<" 0 "<<nApexX<<" "<<ny<<" "<<nApexZ<<" "<<uApex<<" 1\n";
            }
        }
    }

    out.close();
    std::cout << "Cone gerado: " << fileName << "\n";
}

void cone(char* file, float radius, float height, int slices, int stacks) {
    generateCone(file, radius, height, slices, stacks);
}
