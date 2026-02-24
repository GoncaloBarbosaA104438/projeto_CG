#include <iostream>
#include <fstream>
#include <vector>
#include "plane.hpp"

void generatePlane(const std::string &fileName, float length, int divisions)
{
    std::ofstream outFile(fileName);
    if (!outFile.is_open())
    {
        std::cerr << "Erro ao abrir o ficheiro: " << fileName << "\n";
        return;
    }

    float step = length / divisions;
    int totalTriangles = 2 * divisions * divisions; // 2 triângulos por quadrado
    int totalVertices = 3 * totalTriangles;         // 3 vértices por triângulo

    // Escreve o número total de vértices (agrupados em triângulos)
    outFile << totalVertices << std::endl;

    for (int i = 0; i < divisions; ++i)
    {
        for (int j = 0; j < divisions; ++j)
        {
            // Calcula as coordenadas dos 4 cantos do quadrado atual
            float x0 = j * step - (length / 2);
            float z0 = i * step - (length / 2);
            float x1 = (j + 1) * step - (length / 2);
            float z1 = (i + 1) * step - (length / 2);

            // Triângulo 1: (x0, z0) -> (x1, z0) -> (x0, z1)
            outFile << x0 << " 0.0 " << z0 << std::endl; // Vértice 1
            outFile << x0 << " 0.0 " << z1 << std::endl; // Vértice 3
            outFile << x1 << " 0.0 " << z0 << std::endl; // Vértice 2

            // Triângulo 2: (x1, z0) -> (x1, z1) -> (x0, z1)
            outFile << x1 << " 0.0 " << z0 << std::endl; // Vértice 4
            outFile << x0 << " 0.0 " << z1 << std::endl; // Vértice 6
            outFile << x1 << " 0.0 " << z1 << std::endl; // Vértice 5
        }
    }

    outFile.close();
    std::cout << "Plano gerado com sucesso e salvo em " << fileName << "\n";
}

void plane(char *file, float length, int divisions)
{
    generatePlane(file, length, divisions);
}