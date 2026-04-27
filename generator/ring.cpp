#define _USE_MATH_DEFINES
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <vector>
#include "../point/point.hpp"

void generateRing(const std::string& fileName, float inDiam, float outDiam, int slices) {
    std::ofstream outFile(fileName);
    if (!outFile.is_open()) {
        std::cerr << "Erro ao abrir o ficheiro: " << fileName << "\n";
        return;
    }

    float inRadius = inDiam / 2.0f;
    float outRadius = outDiam / 2.0f;

    int totalTriangles = 2 * slices;
    int totalVertices = totalTriangles * 3;
    outFile << totalVertices << std::endl;

    for (int i = 0; i < slices; ++i) {
        float theta = static_cast<float>(i) * 2.0f * M_PI / slices;
        float thetaNext = static_cast<float>(i + 1) * 2.0f * M_PI / slices;

        // Pontos no plano XZ (Y=0)
        Point innerCurrent(inRadius * cos(theta), 0.0f, inRadius * sin(theta));
        Point outerCurrent(outRadius * cos(theta), 0.0f, outRadius * sin(theta));
        Point innerNext(inRadius * cos(thetaNext), 0.0f, inRadius * sin(thetaNext));
        Point outerNext(outRadius * cos(thetaNext), 0.0f, outRadius * sin(thetaNext));

        // Triângulo 1: outerNext → outerCurrent → innerCurrent
        outFile << outerNext.getX() << " " << outerNext.getY() << " " << outerNext.getZ() << std::endl;
        outFile << outerCurrent.getX() << " " << outerCurrent.getY() << " " << outerCurrent.getZ() << std::endl;
        outFile << innerCurrent.getX() << " " << innerCurrent.getY() << " " << innerCurrent.getZ() << std::endl;

        // Triângulo 2: innerNext → outerNext → innerCurrent
        outFile << innerNext.getX() << " " << innerNext.getY() << " " << innerNext.getZ() << std::endl;
        outFile << outerNext.getX() << " " << outerNext.getY() << " " << outerNext.getZ() << std::endl;
        outFile << innerCurrent.getX() << " " << innerCurrent.getY() << " " << innerCurrent.getZ() << std::endl;
    }

    outFile.close();
    std::cout << "Anel XZ corrigido gerado com sucesso: " << fileName << "\n";
}

void ring(char* file, float inDiam, float outDiam, int slices) {
    generateRing(file, inDiam, outDiam, slices);
}