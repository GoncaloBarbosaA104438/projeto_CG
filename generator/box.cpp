#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "box.hpp"
#include "../point/point.hpp"

void generateBox(const std::string &fileName, float length, int divisions)
{
    std::ofstream outFile(fileName);
    if (!outFile.is_open())
    {
        std::cerr << "Erro ao abrir o ficheiro: " << fileName << "\n";
        return;
    }

    float halfDim = length / 2.0f;
    float step = length / divisions;
    int totalVertices = 6 * 2 * 3 * divisions * divisions;
    outFile << totalVertices << std::endl;

    auto generateFace = [&](float fixedCoord, int axis, bool reverseOrder)
    {
        for (int i = 0; i < divisions; ++i)
        {
            for (int j = 0; j < divisions; ++j)
            {
                float v1_start = -halfDim + i * step;
                float v1_end = v1_start + step;
                float v2_start = -halfDim + j * step;
                float v2_end = v2_start + step;

                Point p1{0, 0, 0}, p2{0, 0, 0}, p3{0, 0, 0}, p4{0, 0, 0};

                // Cálculo dos pontos conforme o eixo fixo
                switch (axis)
                {
                case 0: // Faces esquerda/direita (X fixo)
                    p1 = Point(fixedCoord, v1_start, v2_start);
                    p2 = Point(fixedCoord, v1_end, v2_start);
                    p3 = Point(fixedCoord, v1_start, v2_end);
                    p4 = Point(fixedCoord, v1_end, v2_end);
                    break;
                case 1: // Faces topo/base (Y fixo)
                    p1 = Point(v2_start, fixedCoord, v1_start);
                    p2 = Point(v2_end, fixedCoord, v1_start);
                    p3 = Point(v2_start, fixedCoord, v1_end);
                    p4 = Point(v2_end, fixedCoord, v1_end);
                    break;
                case 2: // Faces frente/trás (Z fixo)
                    p1 = Point(v1_start, v2_start, fixedCoord);
                    p2 = Point(v1_end, v2_start, fixedCoord);
                    p3 = Point(v1_start, v2_end, fixedCoord);
                    p4 = Point(v1_end, v2_end, fixedCoord);
                    break;
                }

                // Ordem dos vértices para CCW (vista do exterior)
                if (!reverseOrder)
                {
                    // Triângulo 1: p1 -> p2 -> p4 (CCW)
                    outFile << p1.getX() << " " << p1.getY() << " " << p1.getZ() << std::endl;
                    outFile << p2.getX() << " " << p2.getY() << " " << p2.getZ() << std::endl;
                    outFile << p4.getX() << " " << p4.getY() << " " << p4.getZ() << std::endl;

                    // Triângulo 2: p1 -> p4 -> p3 (CCW)
                    outFile << p1.getX() << " " << p1.getY() << " " << p1.getZ() << std::endl;
                    outFile << p4.getX() << " " << p4.getY() << " " << p4.getZ() << std::endl;
                    outFile << p3.getX() << " " << p3.getY() << " " << p3.getZ() << std::endl;
                }
                else
                {
                    // Ordem invertida para CCW em faces opostas
                    outFile << p1.getX() << " " << p1.getY() << " " << p1.getZ() << std::endl;
                    outFile << p3.getX() << " " << p3.getY() << " " << p3.getZ() << std::endl;
                    outFile << p2.getX() << " " << p2.getY() << " " << p2.getZ() << std::endl;

                    outFile << p2.getX() << " " << p2.getY() << " " << p2.getZ() << std::endl;
                    outFile << p3.getX() << " " << p3.getY() << " " << p3.getZ() << std::endl;
                    outFile << p4.getX() << " " << p4.getY() << " " << p4.getZ() << std::endl;
                }
            }
        }
    };

    // Gerar todas as faces com orientação correta
    generateFace(halfDim, 2, false);  // Frente (Z positivo) - CCW
    generateFace(-halfDim, 2, true);  // Trás (Z negativo) - CCW invertido
    generateFace(-halfDim, 0, true);  // Esquerda (X negativo) - CCW invertido
    generateFace(halfDim, 0, false);  // Direita (X positivo) - CCW
    generateFace(halfDim, 1, true);   // Topo (Y positivo) - CCW invertido
    generateFace(-halfDim, 1, false); // Base (Y negativo) - CCW

    outFile.close();
    std::cout << "Caixa gerada com sucesso e salva em " << fileName << "\n";
}

void box(char *file, float length, int divisions)
{
    generateBox(file, length, divisions);
}