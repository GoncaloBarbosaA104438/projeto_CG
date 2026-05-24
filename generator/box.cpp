#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "box.hpp"

void generateBox(const std::string& fileName, float length, int divisions)
{
    std::ofstream out(fileName);
    if (!out.is_open()) { std::cerr << "Erro ao abrir: " << fileName << "\n"; return; }

    float h = length / 2.0f;
    float step = length / divisions;
    int totalVertices = 6 * 2 * 3 * divisions * divisions;
    out << totalVertices << "\n";

    // face: axis=0 X-fixed, axis=1 Y-fixed, axis=2 Z-fixed
    // sign: +1 or -1 (which side)
    auto writeFace = [&](int axis, float sign) {
        float nx = 0, ny = 0, nz = 0;
        if (axis == 0) nx = sign;
        if (axis == 1) ny = sign;
        if (axis == 2) nz = sign;

        for (int i = 0; i < divisions; ++i) {
            for (int j = 0; j < divisions; ++j) {
                float a0 = -h + i * step, a1 = a0 + step;
                float b0 = -h + j * step, b1 = b0 + step;
                float u0 = (float)j / divisions, u1 = (float)(j+1) / divisions;
                float v0 = (float)i / divisions, v1 = (float)(i+1) / divisions;

                // Build 4 corners based on axis
                struct V { float x,y,z,s,t; };
                V p[4];
                if (axis == 0) { // X fixed, vary Y=a, Z=b
                    p[0] = {sign*h, a0, b0, u0, v0};
                    p[1] = {sign*h, a1, b0, u0, v1};
                    p[2] = {sign*h, a0, b1, u1, v0};
                    p[3] = {sign*h, a1, b1, u1, v1};
                } else if (axis == 1) { // Y fixed, vary X=b, Z=a
                    p[0] = {b0, sign*h, a0, u0, v0};
                    p[1] = {b1, sign*h, a0, u1, v0};
                    p[2] = {b0, sign*h, a1, u0, v1};
                    p[3] = {b1, sign*h, a1, u1, v1};
                } else { // Z fixed, vary X=a, Y=b
                    p[0] = {a0, b0, sign*h, u0, v0};
                    p[1] = {a1, b0, sign*h, u1, v0};
                    p[2] = {a0, b1, sign*h, u0, v1};
                    p[3] = {a1, b1, sign*h, u1, v1};
                }

                auto write = [&](int idx) {
                    out << p[idx].x << " " << p[idx].y << " " << p[idx].z << " "
                        << nx << " " << ny << " " << nz << " "
                        << p[idx].s << " " << p[idx].t << "\n";
                };

                if (sign > 0) {
                    write(0); write(1); write(3);
                    write(0); write(3); write(2);
                } else {
                    write(0); write(3); write(1);
                    write(0); write(2); write(3);
                }
            }
        }
    };

    writeFace(2,  1); // Front  Z+
    writeFace(2, -1); // Back   Z-
    writeFace(0, -1); // Left   X-
    writeFace(0,  1); // Right  X+
    writeFace(1,  1); // Top    Y+
    writeFace(1, -1); // Bottom Y-

    out.close();
    std::cout << "Caixa gerada: " << fileName << "\n";
}

void box(char* file, float length, int divisions) {
    generateBox(file, length, divisions);
}
