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
    int totalTriangles = 2 * slices * (stacks - 1);
    int totalVertices = totalTriangles * 3;
    outFile << totalVertices << std::endl;

    std::vector<std::vector<Point>> points(stacks + 1);
    for (int stack = 0; stack <= stacks; ++stack)
    {
        float phi = static_cast<float>(stack) * M_PI / stacks;
        float y = radius * cos(phi);
        float r = radius * sin(phi);

        points[stack].resize(slices, Point(0, 0, 0));
        for (int slice = 0; slice < slices; ++slice)
        {
            float theta = static_cast<float>(slice) * 2.0f * M_PI / slices;
            float x = r * cos(theta);
            float z = r * sin(theta);
            points[stack][slice] = Point(x, y, z);
        }
    }

    for (int stack = 0; stack < stacks; ++stack)
    {
        for (int slice = 0; slice < slices; ++slice)
        {
            int nextSlice = (slice + 1) % slices;

            if (stack == 0)
            {
                Point topPole = points[0][0];
                Point p1 = points[1][slice];
                Point p2 = points[1][nextSlice];

                outFile << topPole.getX() << " " << topPole.getY() << " " << topPole.getZ() << std::endl;
                outFile << p2.getX() << " " << p2.getY() << " " << p2.getZ() << std::endl;
                outFile << p1.getX() << " " << p1.getY() << " " << p1.getZ() << std::endl;
            }
            else if (stack == stacks - 1)
            {
                Point p1 = points[stack][slice];
                Point p2 = points[stack][nextSlice];
                Point bottomPole = points[stacks][0];

                outFile << p1.getX() << " " << p1.getY() << " " << p1.getZ() << std::endl;
                outFile << p2.getX() << " " << p2.getY() << " " << p2.getZ() << std::endl;
                outFile << bottomPole.getX() << " " << bottomPole.getY() << " " << bottomPole.getZ() << std::endl;
            }
            else
            {
                Point p1 = points[stack][slice];
                Point p2 = points[stack][nextSlice];
                Point p3 = points[stack + 1][slice];
                Point p4 = points[stack + 1][nextSlice];

                outFile << p1.getX() << " " << p1.getY() << " " << p1.getZ() << std::endl;
                outFile << p2.getX() << " " << p2.getY() << " " << p2.getZ() << std::endl;
                outFile << p3.getX() << " " << p3.getY() << " " << p3.getZ() << std::endl;

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