#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cmath>
#include <cstring>
#include <stdlib.h>

// Inclui os teus headers das primitivas
#include "plane.hpp"
#include "box.hpp"
#include "sphere.hpp"
#include "cone.hpp"
#include "ring.hpp"
#include "../point/point.hpp"// Descomenta e certifica-te que o caminho para a tua classe Point está correto

// ============================================================================
// LÓGICA DAS SUPERFÍCIES DE BÉZIER
// ============================================================================

// 1. Função para calcular o Polinómio de Bernstein de grau 3
float getBernstein(int i, float t) {
    switch (i) {
        case 0: return pow(1.0f - t, 3);
        case 1: return 3.0f * t * pow(1.0f - t, 2);
        case 2: return 3.0f * pow(t, 2) * (1.0f - t);
        case 3: return pow(t, 3);
    }
    return 0.0f;
}

// 2. Função para calcular um ponto P(u,v) num patch usando 16 pontos de controlo
Point getBezierPoint(float u, float v, const std::vector<Point>& controlPoints) {
    float px = 0.0f, py = 0.0f, pz = 0.0f;

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            float bu = getBernstein(i, u);
            float bv = getBernstein(j, v);
            float weight = bu * bv;

            // O índice na lista unidimensional de 16 pontos
            Point p = controlPoints[(i * 4) + j];
            
            px += p.getX() * weight;
            py += p.getY() * weight;
            pz += p.getZ() * weight;
        }
    }
    return Point(px, py, pz);
}

// 3. Função principal que lê o ficheiro .patch, tessela e escreve no .3d
void generateBezier(const char* patchFile, int tessellation, const char* outputFile) {
    std::ifstream fileIn(patchFile);
    if (!fileIn.is_open()) {
        std::cerr << "Erro ao abrir o ficheiro de controlo: " << patchFile << std::endl;
        return;
    }

    // LER O FICHEIRO .PATCH
    std::string line;
    
    // Ler o número de patches
    std::getline(fileIn, line);
    int numPatches = std::stoi(line);

    // Ler os índices de cada patch
    std::vector<std::vector<int>> patchIndices(numPatches, std::vector<int>(16));
    for (int i = 0; i < numPatches; i++) {
        std::getline(fileIn, line);
        std::stringstream ss(line);
        std::string token;
        int j = 0;
        // Separar os índices pelas vírgulas
        while (std::getline(ss, token, ',')) {
            if (j < 16) {
                patchIndices[i][j] = std::stoi(token);
                j++;
            }
        }
    }

    // Ler o número total de pontos de controlo
    std::getline(fileIn, line);
    int numControlPoints = std::stoi(line);

    // Ler as coordenadas X, Y, Z de todos os pontos de controlo
    std::vector<Point> allControlPoints;
    for (int i = 0; i < numControlPoints; i++) {
        std::getline(fileIn, line);
        std::stringstream ss(line);
        std::string token;
        
        std::getline(ss, token, ','); float x = std::stof(token);
        std::getline(ss, token, ','); float y = std::stof(token);
        std::getline(ss, token, ','); float z = std::stof(token);
        
        allControlPoints.push_back(Point(x, y, z));
    }
    fileIn.close();

    // GERAR OS VÉRTICES (TESSELAÇÃO)
    std::vector<Point> finalVertices;
    float step = 1.0f / tessellation;

    for (int p = 0; p < numPatches; p++) {
        // Extrair os 16 pontos de controlo específicos deste patch
        std::vector<Point> currentPatchPoints(16, Point(0.0, 0.0, 0.0));
        for (int i = 0; i < 16; i++) {
            currentPatchPoints[i] = allControlPoints[patchIndices[p][i]];
        }

        // Percorrer a grelha de tesselação
        for (int i = 0; i < tessellation; i++) {
            for (int j = 0; j < tessellation; j++) {
                float u1 = i * step;
                float v1 = j * step;
                float u2 = (i + 1) * step;
                float v2 = (j + 1) * step;

                // Calcular os 4 cantos de cada pequeno quadrado na grelha
                Point p1 = getBezierPoint(u1, v1, currentPatchPoints);
                Point p2 = getBezierPoint(u2, v1, currentPatchPoints);
                Point p3 = getBezierPoint(u1, v2, currentPatchPoints);
                Point p4 = getBezierPoint(u2, v2, currentPatchPoints);

                // Triângulo 1 (p1, p3, p2) - A ordem importa para o culling!
                finalVertices.push_back(p1);
                finalVertices.push_back(p3);
                finalVertices.push_back(p2);

                // Triângulo 2 (p2, p3, p4)
                finalVertices.push_back(p2);
                finalVertices.push_back(p3);
                finalVertices.push_back(p4);
            }
        }
    }

    // ESCREVER NO FICHEIRO .3D DE SAÍDA
    std::ofstream fileOut(outputFile);
    if (!fileOut.is_open()) {
        std::cerr << "Erro ao abrir ficheiro de destino: " << outputFile << std::endl;
        return;
    }

    // Escreve o número total de vértices na primeira linha (formato da tua fase 1)
    fileOut << finalVertices.size() << std::endl;
    for (const auto& pt : finalVertices) {
        fileOut << pt.getX() << " " << pt.getY() << " " << pt.getZ() << std::endl;
    }

    fileOut.close();
    std::cout << "Modelo Bezier gerado com sucesso em " << outputFile << " (" << finalVertices.size() << " vertices)" << std::endl;
}


int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cout << "Erro: Faltam argumentos! Uso: ./generator <forma> [parametros] <ficheiro.3d>\n";
        return 1;
    }

    if (strcmp(argv[1], "plane") == 0)
    {
        plane(argv[4], atof(argv[2]), atoi(argv[3]));
    }
    else if (strcmp(argv[1], "box") == 0)
    {
        box(argv[4], atof(argv[2]), atoi(argv[3]));
    }
    else if (strcmp(argv[1], "sphere") == 0)
    {
        sphere(argv[5], atof(argv[2]), atoi(argv[3]), atoi(argv[4]));
    }
    else if (strcmp(argv[1], "cone") == 0)
    {
        cone(argv[6], atof(argv[2]), atof(argv[3]), atoi(argv[4]), atoi(argv[5]));
    }
    else if (strcmp(argv[1], "ring") == 0)
    {
        if (argc < 6) {
            std::cout << "Erro: Argumentos insuficientes. Uso: ./generator ring <raio_int> <raio_ext> <fatias> <ficheiro.3d>\n";
            return 1;
        }
        ring(argv[5], atof(argv[2]), atof(argv[3]), atoi(argv[4]));
    }
    else if (strcmp(argv[1], "patch") == 0)
    {
        // Novo comando para Bézier [cite: 118, 119]
        // Exemplo: ./generator patch teapot.patch 10 teapot.3d
        if (argc < 5) {
            std::cout << "Erro: Argumentos insuficientes para patch. Uso: ./generator patch <file.patch> <tessellation> <file.3d>\n";
            return 1;
        }
        generateBezier(argv[2], atoi(argv[3]), argv[4]);
    }
    else
    {
        std::cout << "Erro: Forma '" << argv[1] << "' nao reconhecida.\n";
    }

    return 0;
}