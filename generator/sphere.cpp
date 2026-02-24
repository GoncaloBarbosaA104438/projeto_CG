#define _USE_MATH_DEFINES
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <vector>
#include "sphere.hpp"
#include "../point/point.hpp"

void generateSphere(const std::string &fileName, float radius, int slices, int stacks)
{
    std::ofstream outFile(fileName);
    if (!outFile.is_open())
    {
        std::cerr << "Erro ao abrir o ficheiro: " << fileName << "\n";
        return;
    }

    // Total de vértices: 3 vértices por triângulo × 2 triângulos por fatia × (stacks - 1) camadas
    int totalTriangles = 2 * slices * (stacks - 1);
    int totalVertices = totalTriangles * 3;
    outFile << totalVertices << std::endl;

    // Gerar pontos para cada camada e fatia
    std::vector<std::vector<Point>> points(stacks + 1);
    for (int stack = 0; stack <= stacks; ++stack)
    {
        float phi = static_cast<float>(stack) * M_PI / stacks;
        float y = radius * cos(phi); // Coordenada Y (vertical)
        float r = radius * sin(phi); // Raio no plano XZ

        points[stack].resize(slices, Point(0, 0, 0));
        for (int slice = 0; slice < slices; ++slice)
        {
            float theta = static_cast<float>(slice) * 2.0f * M_PI / slices;
            float x = r * cos(theta);
            float z = r * sin(theta);
            points[stack][slice] = Point(x, y, z);
        }
    }

    // Gerar triângulos
    for (int stack = 0; stack < stacks; ++stack)
    {
        for (int slice = 0; slice < slices; ++slice)
        {
            int nextSlice = (slice + 1) % slices;

            if (stack == 0)
            {
                // Tampo superior (conecta ao polo norte)
                Point topPole = points[0][0]; // Polo norte (Y máximo)
                Point p1 = points[1][slice];
                Point p2 = points[1][nextSlice];

                // CORREÇÃO FEITA AQUI: A ordem é topPole → p2 → p1 (CCW)
                outFile << topPole.getX() << " " << topPole.getY() << " " << topPole.getZ() << std::endl;
                outFile << p2.getX() << " " << p2.getY() << " " << p2.getZ() << std::endl;
                outFile << p1.getX() << " " << p1.getY() << " " << p1.getZ() << std::endl;
            }
            else if (stack == stacks - 1)
            {
                // Tampo inferior (conecta ao polo sul)
                Point p1 = points[stack][slice];
                Point p2 = points[stack][nextSlice];
                Point bottomPole = points[stacks][0]; // Polo sul (Y mínimo)

                // Triângulo: p1 → p2 → bottomPole (CCW visto de fora)
                outFile << p1.getX() << " " << p1.getY() << " " << p1.getZ() << std::endl;
                outFile << p2.getX() << " " << p2.getY() << " " << p2.getZ() << std::endl;
                outFile << bottomPole.getX() << " " << bottomPole.getY() << " " << bottomPole.getZ() << std::endl;
            }
            else
            {
                // Camadas intermediárias (formam quads divididos em dois triângulos)
                Point p1 = points[stack][slice];
                Point p2 = points[stack][nextSlice];
                Point p3 = points[stack + 1][slice];
                Point p4 = points[stack + 1][nextSlice];

                // Triângulo 1: p1 → p2 → p3
                outFile << p1.getX() << " " << p1.getY() << " " << p1.getZ() << std::endl;
                outFile << p2.getX() << " " << p2.getY() << " " << p2.getZ() << std::endl;
                outFile << p3.getX() << " " << p3.getY() << " " << p3.getZ() << std::endl;

                // Triângulo 2: p2 → p4 → p3
                outFile << p2.getX() << " " << p2.getY() << " " << p2.getZ() << std::endl;
                outFile << p4.getX() << " " << p4.getY() << " " << p4.getZ() << std::endl;
                outFile << p3.getX() << " " << p3.getY() << " " << p3.getZ() << std::endl;
            }
        }
    }

    outFile.close();
    std::cout << "Esfera gerada com sucesso: " << fileName << "\n";
}

void sphere(char *file, float radius, int slices, int stacks)
{
    generateSphere(file, radius, slices, stacks);
}