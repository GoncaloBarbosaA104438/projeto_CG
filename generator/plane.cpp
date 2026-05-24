#include <iostream>
#include <fstream>
#include <vector>
#include "plane.hpp"

void generatePlane(const std::string& fileName, float length, int divisions)
{
    std::ofstream out(fileName);
    if (!out.is_open()) { std::cerr << "Erro ao abrir: " << fileName << "\n"; return; }

    float step = length / divisions;
    int totalVertices = 6 * divisions * divisions;
    out << totalVertices << "\n";

    for (int i = 0; i < divisions; ++i) {
        for (int j = 0; j < divisions; ++j) {
            float x0 = j * step - length / 2, x1 = x0 + step;
            float z0 = i * step - length / 2, z1 = z0 + step;
            float s0 = (float)j / divisions, s1 = (float)(j+1) / divisions;
            float t0 = (float)i / divisions, t1 = (float)(i+1) / divisions;

            // Triangle 1
            out << x0 << " 0 " << z0 << " 0 1 0 " << s0 << " " << t0 << "\n";
            out << x0 << " 0 " << z1 << " 0 1 0 " << s0 << " " << t1 << "\n";
            out << x1 << " 0 " << z0 << " 0 1 0 " << s1 << " " << t0 << "\n";
            // Triangle 2
            out << x1 << " 0 " << z0 << " 0 1 0 " << s1 << " " << t0 << "\n";
            out << x0 << " 0 " << z1 << " 0 1 0 " << s0 << " " << t1 << "\n";
            out << x1 << " 0 " << z1 << " 0 1 0 " << s1 << " " << t1 << "\n";
        }
    }

    out.close();
    std::cout << "Plano gerado: " << fileName << "\n";
}

void plane(char* file, float length, int divisions) {
    generatePlane(file, length, divisions);
}
