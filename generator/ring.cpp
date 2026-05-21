#define _USE_MATH_DEFINES
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <vector>
#include "../point/point.hpp"

void generateRing(const std::string& fileName, float inDiam, float outDiam, int slices)
{
    std::ofstream out(fileName);
    if (!out.is_open()) { std::cerr << "Erro ao abrir: " << fileName << "\n"; return; }

    float inR  = inDiam  / 2.0f;
    float outR = outDiam / 2.0f;

    int totalVertices = 2 * slices * 3;
    out << totalVertices << "\n";

    for (int i = 0; i < slices; ++i) {
        float t0 = (float)i       * 2.0f * M_PI / slices;
        float t1 = (float)(i + 1) * 2.0f * M_PI / slices;

        float iox0 = inR  * cos(t0), ioz0 = inR  * sin(t0);
        float oox0 = outR * cos(t0), ooz0 = outR * sin(t0);
        float iox1 = inR  * cos(t1), ioz1 = inR  * sin(t1);
        float oox1 = outR * cos(t1), ooz1 = outR * sin(t1);

        // UV: map radial distance to v, angle to u
        float u0 = (float)i / slices, u1 = (float)(i + 1) / slices;

        // Tri 1: outer_next, outer_cur, inner_cur (normal up)
        out << oox1<<" 0 "<<ooz1<<" 0 1 0 "<<u1<<" 1\n";
        out << oox0<<" 0 "<<ooz0<<" 0 1 0 "<<u0<<" 1\n";
        out << iox0<<" 0 "<<ioz0<<" 0 1 0 "<<u0<<" 0\n";

        // Tri 2: inner_next, outer_next, inner_cur
        out << iox1<<" 0 "<<ioz1<<" 0 1 0 "<<u1<<" 0\n";
        out << oox1<<" 0 "<<ooz1<<" 0 1 0 "<<u1<<" 1\n";
        out << iox0<<" 0 "<<ioz0<<" 0 1 0 "<<u0<<" 0\n";
    }

    out.close();
    std::cout << "Anel gerado: " << fileName << "\n";
}

void ring(char* file, float inDiam, float outDiam, int slices) {
    generateRing(file, inDiam, outDiam, slices);
}
