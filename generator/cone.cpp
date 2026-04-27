#define _USE_MATH_DEFINES
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include "cone.hpp"
#include "../point/point.hpp"

void generateCone(const std::string &fileName, float radius, float height, int slices, int stacks)
{
    std::ofstream outFile(fileName);
    if (!outFile.is_open())
    {
        std::cerr << "Erro ao abrir o ficheiro: " << fileName << "\n";
        return;
    }

    Point baseCenter(0, 0, 0);
    Point apex(0, height, 0);

    int baseTriangles = slices;
    int lateralTriangles = (stacks - 1) * slices * 2 + slices;
    int totalVertices = (baseTriangles + lateralTriangles) * 3;
    outFile << totalVertices << std::endl;

    float angleStep = 2.0f * M_PI / slices;
    for (int i = 0; i < slices; ++i)
    {
        float theta1 = i * angleStep;
        float theta2 = (i + 1) * angleStep;

        Point p1(radius * cos(theta1), 0, radius * sin(theta1));
        Point p2(radius * cos(theta2), 0, radius * sin(theta2));

        outFile << baseCenter.getX() << " " << baseCenter.getY() << " " << baseCenter.getZ() << std::endl;
        outFile << p1.getX() << " " << p1.getY() << " " << p1.getZ() << std::endl;
        outFile << p2.getX() << " " << p2.getY() << " " << p2.getZ() << std::endl;
    }

    for (int i = 0; i < stacks; ++i)
    {
        float yCurrent = i * (height / stacks);
        float rCurrent = radius * (1.0f - static_cast<float>(i) / stacks);

        float yNext, rNext;
        if (i < stacks - 1)
        {
            yNext = (i + 1) * (height / stacks);
            rNext = radius * (1.0f - static_cast<float>(i + 1) / stacks);
        }
        else
        {
            yNext = height;
            rNext = 0.0f;
        }

        for (int j = 0; j < slices; ++j)
        {
            float theta1 = j * angleStep;
            float theta2 = (j + 1) * angleStep;

            Point pCurrent1(rCurrent * cos(theta1), yCurrent, rCurrent * sin(theta1));
            Point pCurrent2(rCurrent * cos(theta2), yCurrent, rCurrent * sin(theta2));
            Point pNext1(rNext * cos(theta1), yNext, rNext * sin(theta1));
            Point pNext2(rNext * cos(theta2), yNext, rNext * sin(theta2));

            if (i < stacks - 1)
            {
                outFile << pCurrent1.getX() << " " << pCurrent1.getY() << " " << pCurrent1.getZ() << std::endl;
                outFile << pCurrent2.getX() << " " << pCurrent2.getY() << " " << pCurrent2.getZ() << std::endl;
                outFile << pNext1.getX() << " " << pNext1.getY() << " " << pNext1.getZ() << std::endl;

                outFile << pCurrent2.getX() << " " << pCurrent2.getY() << " " << pCurrent2.getZ() << std::endl;
                outFile << pNext2.getX() << " " << pNext2.getY() << " " << pNext2.getZ() << std::endl;
                outFile << pNext1.getX() << " " << pNext1.getY() << " " << pNext1.getZ() << std::endl;
            }
            else
            {
                outFile << pCurrent1.getX() << " " << pCurrent1.getY() << " " << pCurrent1.getZ() << std::endl;
                outFile << pCurrent2.getX() << " " << pCurrent2.getY() << " " << pCurrent2.getZ() << std::endl;
                outFile << apex.getX() << " " << apex.getY() << " " << apex.getZ() << std::endl;
            }
        }
    }

    outFile.close();
    std::cout << "Cone gerado com sucesso: " << fileName << "\n";
}

void cone(char *file, float radius, float height, int slices, int stacks)
{
    generateCone(file, radius, height, slices, stacks);
}