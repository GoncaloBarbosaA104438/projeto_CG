#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include "../point/point.hpp" // Ajusta o caminho se necessário

void ring(const char* filename, float innerRadius, float outerRadius, int slices) {
    std::vector<Point> vertices;
    float angleStep = (2.0f * M_PI) / slices;

    for (int i = 0; i < slices; i++) {
        float a1 = i * angleStep;
        float a2 = (i + 1) * angleStep;

        // Calcular os 4 pontos da fatia atual
        Point p1(innerRadius * sin(a1), 0.0f, innerRadius * cos(a1));
        Point p2(outerRadius * sin(a1), 0.0f, outerRadius * cos(a1));
        Point p3(innerRadius * sin(a2), 0.0f, innerRadius * cos(a2));
        Point p4(outerRadius * sin(a2), 0.0f, outerRadius * cos(a2));

        // Face Superior (Visível quando olhamos de cima)
        vertices.push_back(p1); vertices.push_back(p2); vertices.push_back(p4);
        vertices.push_back(p1); vertices.push_back(p4); vertices.push_back(p3);

        // Face Inferior (Visível quando olhamos de baixo - desenhamos com a ordem invertida)
        vertices.push_back(p1); vertices.push_back(p4); vertices.push_back(p2);
        vertices.push_back(p1); vertices.push_back(p3); vertices.push_back(p4);
    }

    // Escrever no ficheiro
    std::ofstream fileOut(filename);
    if (!fileOut.is_open()) {
        std::cerr << "Erro ao abrir ficheiro de destino: " << filename << std::endl;
        return;
    }

    fileOut << vertices.size() << std::endl;
    for (const auto& pt : vertices) {
        fileOut << pt.getX() << " " << pt.getY() << " " << pt.getZ() << std::endl;
    }

    fileOut.close();
    std::cout << "Anel gerado com sucesso em " << filename << " (" << vertices.size() << " vertices)" << std::endl;
}