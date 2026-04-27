#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <string>

// Estrutura simples para o ponto 3D
struct Point3D {
    float x, y, z;
    Point3D(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}
};

// Matriz de base Cúbica de Bezier (M)
const float bezierMatrix[4][4] = {
    {-1.0f,  3.0f, -3.0f,  1.0f},
    { 3.0f, -6.0f,  3.0f,  0.0f},
    {-3.0f,  3.0f,  0.0f,  0.0f},
    { 1.0f,  0.0f,  0.0f,  0.0f}
};

void multiplyTM(float t, float result[4]) {
    float t3 = t * t * t;
    float t2 = t * t;
    
    for (int i = 0; i < 4; i++) {
        result[i] = t3 * bezierMatrix[0][i] + 
                    t2 * bezierMatrix[1][i] + 
                    t  * bezierMatrix[2][i] + 
                    1  * bezierMatrix[3][i];
    }
}

Point3D evaluateBezierSurface(const std::vector<std::vector<Point3D>>& controlGrid, float u, float v) {
    float U[4], V[4];
    multiplyTM(u, U);
    multiplyTM(v, V);
    
    Point3D result(0, 0, 0);
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            float coef = U[i] * V[j];
            result.x += controlGrid[i][j].x * coef;
            result.y += controlGrid[i][j].y * coef;
            result.z += controlGrid[i][j].z * coef;
        }
    }
    return result;
}

void bezier(const char* patchFile, const char* outputFile, int tessellationLevel) {
    std::ifstream file(patchFile);
    if (!file.is_open()) {
        std::cerr << "Erro ao abrir o ficheiro patch: " << patchFile << std::endl;
        return;
    }

    // 1. LER O NÚMERO DE PATCHES
    int numPatches = 0;
    if (!(file >> numPatches)) return;
    
    std::vector<std::vector<int>> patchIndices(numPatches, std::vector<int>(16));
    
    // Ler os índices dos 16 pontos de controlo por patch
    for (int i = 0; i < numPatches; i++) {
        for (int j = 0; j < 16; j++) {
            file >> patchIndices[i][j];
            if (j < 15) file.ignore(1, ','); 
        }
    }

    // 2. LER OS PONTOS DE CONTROLO ATÉ AO FIM DO FICHEIRO
    std::vector<Point3D> controlPoints;
    float cx, cy, cz;
    
    // Verifica se a próxima linha é um número isolado (como o 306) e ignora-o
    std::string tempStr;
    file >> tempStr;
    if(tempStr.find(',') == std::string::npos) {
        // Era só a contagem, vamos ler o verdadeiro X a seguir
        file >> cx; 
    } else {
        // Já era a coordenada X!
        cx = std::stof(tempStr);
    }
    
    // Ler o resto do primeiro ponto e os restantes
    file.ignore(1, ','); file >> cy;
    file.ignore(1, ','); file >> cz;
    controlPoints.push_back(Point3D(cx, cy, cz));

    while (file >> cx) {
        file.ignore(1, ','); file >> cy;
        file.ignore(1, ','); file >> cz;
        controlPoints.push_back(Point3D(cx, cy, cz));
    }
    file.close();

    // 3. TESSELAÇÃO E CÁLCULO
    std::vector<Point3D> finalVertices;
    float step = 1.0f / tessellationLevel;

    for (int p = 0; p < numPatches; p++) {
        std::vector<std::vector<Point3D>> controlGrid(4, std::vector<Point3D>(4));
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                controlGrid[i][j] = controlPoints[patchIndices[p][i * 4 + j]];
            }
        }

        for (int i = 0; i < tessellationLevel; i++) {
            for (int j = 0; j < tessellationLevel; j++) {
                float u1 = i * step;
                float u2 = (i + 1) * step;
                float v1 = j * step;
                float v2 = (j + 1) * step;

                Point3D p1 = evaluateBezierSurface(controlGrid, u1, v1);
                Point3D p2 = evaluateBezierSurface(controlGrid, u1, v2);
                Point3D p3 = evaluateBezierSurface(controlGrid, u2, v1);
                Point3D p4 = evaluateBezierSurface(controlGrid, u2, v2);

                finalVertices.push_back(p1); finalVertices.push_back(p3); finalVertices.push_back(p2);
                finalVertices.push_back(p2); finalVertices.push_back(p3); finalVertices.push_back(p4);
            }
        }
    }

    // 4. GRAVAR NO FICHEIRO .3D
    std::ofstream out(outputFile);
    if (!out.is_open()) {
        std::cerr << "Erro ao criar ficheiro de destino: " << outputFile << std::endl;
        return;
    }

    out << finalVertices.size() << "\n";
    for (const auto& v : finalVertices) {
        out << v.x << " " << v.y << " " << v.z << "\n";
    }
    out.close();
    
    std::cout << "Bezier patch gerado em '" << outputFile << "' com " << finalVertices.size() << " vertices!" << std::endl;
}