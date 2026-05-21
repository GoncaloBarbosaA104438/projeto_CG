#define _USE_MATH_DEFINES
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <vector>
#include "sphere.hpp"

struct SVertex { float x,y,z, nx,ny,nz, s,t; };

static void writeSV(std::ofstream& f, const SVertex& v) {
    f << v.x << " " << v.y << " " << v.z << " "
      << v.nx << " " << v.ny << " " << v.nz << " "
      << v.s << " " << v.t << "\n";
}

void generateSphere(const std::string& fileName, float radius, int slices, int stacks)
{
    std::ofstream out(fileName);
    if (!out.is_open()) { std::cerr << "Erro ao abrir: " << fileName << "\n"; return; }

    // Each stack generates slices quads (2 triangles), except poles which are 1 triangle
    int totalVertices = slices * 2 + (stacks - 2) * slices * 6;
    // top cap: slices*3, bottom cap: slices*3, middle bands: (stacks-2)*slices*6
    totalVertices = slices * 3 + slices * 3 + (stacks - 2) * slices * 6;
    out << totalVertices << "\n";

    // Build vertex grid with proper UV (slices+1 columns for seam)
    std::vector<std::vector<SVertex>> grid(stacks + 1, std::vector<SVertex>(slices + 1));
    for (int st = 0; st <= stacks; ++st) {
        float phi = (float)st * M_PI / stacks;
        float cosPhi = cos(phi), sinPhi = sin(phi);
        float y = radius * cosPhi;
        float r = radius * sinPhi;
        for (int sl = 0; sl <= slices; ++sl) {
            float theta = (float)sl * 2.0f * M_PI / slices;
            float cosTheta = cos(theta), sinTheta = sin(theta);
            SVertex v;
            v.x = r * cosTheta;
            v.y = y;
            v.z = r * sinTheta;
            v.nx = sinPhi * cosTheta;
            v.ny = cosPhi;
            v.nz = sinPhi * sinTheta;
            v.s = (float)sl / slices;
            v.t = 1.0f - (float)st / stacks;
            grid[st][sl] = v;
        }
    }

    for (int st = 0; st < stacks; ++st) {
        for (int sl = 0; sl < slices; ++sl) {
            const SVertex& p00 = grid[st][sl];
            const SVertex& p01 = grid[st][sl + 1];
            const SVertex& p10 = grid[st + 1][sl];
            const SVertex& p11 = grid[st + 1][sl + 1];

            if (st == 0) {
                // Top cap: pole vertex with averaged s
                SVertex pole = p00;
                pole.s = ((float)sl + 0.5f) / slices;
                writeSV(out, pole);
                writeSV(out, p11);
                writeSV(out, p10);
            } else if (st == stacks - 1) {
                // Bottom cap: pole vertex with averaged s
                SVertex pole = p10;
                pole.s = ((float)sl + 0.5f) / slices;
                writeSV(out, p00);
                writeSV(out, p01);
                writeSV(out, pole);
            } else {
                writeSV(out, p00);
                writeSV(out, p01);
                writeSV(out, p10);
                writeSV(out, p01);
                writeSV(out, p11);
                writeSV(out, p10);
            }
        }
    }

    out.close();
    std::cout << "Esfera gerada: " << fileName << " (" << totalVertices << " vertices)\n";
}

void sphere(char* file, float radius, int slices, int stacks) {
    generateSphere(file, radius, slices, stacks);
}
