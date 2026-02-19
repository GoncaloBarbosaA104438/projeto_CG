#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace std;

struct Vertex
{
    float x, y, z;
};

void saveToFile(const vector<Vertex> &vertices, const string &filename)
{
    ofstream file(filename);
    if (!file.is_open())
    {
        cerr << "Erro ao abrir ficheiro para escrita: " << filename << endl;
        return;
    }
    file << vertices.size() << "\n";
    for (const auto &v : vertices)
    {
        file << v.x << " " << v.y << " " << v.z << "\n";
    }
    file.close();
    cout << "Ficheiro '" << filename << "' gerado com sucesso (" << vertices.size() << " vertices)." << endl;
}

// FASE 1 - PRIMITIVAS COM WINDING ORDER CCW (Counter-Clockwise)

void generatePlane(float length, int divisions, const string &filename){
    vector<Vertex> vertices;
    float step = length / divisions;
    float start = -length / 2.0f;

    for (int i = 0; i < divisions; ++i)
    {
        for (int j = 0; j < divisions; ++j)
        {
            float px1 = start + i * step;
            float pz1 = start + j * step;
            float px2 = px1 + step;
            float pz2 = pz1 + step;

            // Orientado para cima (+Y). Regra da mão direita (CCW).
            // --- Triângulo 1 (CCW) ---
            vertices.push_back({px1, 0, pz1}); 
            vertices.push_back({px1, 0, pz2}); 
            vertices.push_back({px2, 0, pz1}); 

            // --- Triângulo 2 (CCW) ---
            vertices.push_back({px2, 0, pz2});
            vertices.push_back({px2, 0, pz1}); 
            vertices.push_back({px1, 0, pz2}); 
            
        }
    }
    saveToFile(vertices, filename);
}

void generateBox(float dimension, int divisions, const string &filename)
{
    vector<Vertex> vertices;
    float step = dimension / divisions;
    float start = -dimension / 2.0f;
    float d2 = dimension / 2.0f;

    for (int i = 0; i < divisions; ++i)
    {
        for (int j = 0; j < divisions; ++j)
        {
            float p1 = start + i * step;       // x ou z min
            float p2 = start + (i + 1) * step; // x ou z max
            float p3 = start + j * step;       // y ou z min
            float p4 = start + (j + 1) * step; // y ou z max

            // Face Frontal (+Z) -> Olhando de frente
            vertices.push_back({p1, p3, d2});
            vertices.push_back({p2, p3, d2});
            vertices.push_back({p2, p4, d2});
            vertices.push_back({p1, p3, d2});
            vertices.push_back({p2, p4, d2});
            vertices.push_back({p1, p4, d2});

            // Face Traseira (-Z) -> Olhando por trás
            vertices.push_back({p2, p3, -d2});
            vertices.push_back({p1, p3, -d2});
            vertices.push_back({p1, p4, -d2});
            vertices.push_back({p2, p3, -d2});
            vertices.push_back({p1, p4, -d2});
            vertices.push_back({p2, p4, -d2});

            // Face Topo (+Y) -> Olhando de cima
            vertices.push_back({p1, d2, p4});
            vertices.push_back({p2, d2, p4});
            vertices.push_back({p2, d2, p3});
            vertices.push_back({p1, d2, p4});
            vertices.push_back({p2, d2, p3});
            vertices.push_back({p1, d2, p3});

            // Face Base (-Y) -> Olhando de baixo
            vertices.push_back({p1, -d2, p3});
            vertices.push_back({p2, -d2, p3});
            vertices.push_back({p2, -d2, p4});
            vertices.push_back({p1, -d2, p3});
            vertices.push_back({p2, -d2, p4});
            vertices.push_back({p1, -d2, p4});

            // Face Direita (+X) -> Olhando da direita
            vertices.push_back({d2, p3, p2});
            vertices.push_back({d2, p3, p1});
            vertices.push_back({d2, p4, p1});
            vertices.push_back({d2, p3, p2});
            vertices.push_back({d2, p4, p1});
            vertices.push_back({d2, p4, p2});

            // Face Esquerda (-X) -> Olhando da esquerda
            vertices.push_back({-d2, p3, p1});
            vertices.push_back({-d2, p3, p2});
            vertices.push_back({-d2, p4, p2});
            vertices.push_back({-d2, p3, p1});
            vertices.push_back({-d2, p4, p2});
            vertices.push_back({-d2, p4, p1});
        }
    }
    saveToFile(vertices, filename);
}

void generateSphere(float radius, int slices, int stacks, const string &filename)
{
    vector<Vertex> vertices;
    for (int i = 0; i < stacks; ++i)
    {
        float phi1 = (M_PI / stacks) * i - M_PI / 2.0;
        float phi2 = (M_PI / stacks) * (i + 1) - M_PI / 2.0;

        for (int j = 0; j < slices; ++j)
        {
            float theta1 = (2.0 * M_PI / slices) * j;
            float theta2 = (2.0 * M_PI / slices) * (j + 1);

            auto p = [&](float p_phi, float p_theta) -> Vertex
            {
                return {radius * (float)(cos(p_phi) * sin(p_theta)), radius * (float)sin(p_phi), radius * (float)(cos(p_phi) * cos(p_theta))};
            };

            Vertex v1 = p(phi1, theta1);
            Vertex v2 = p(phi1, theta2);
            Vertex v3 = p(phi2, theta1);
            Vertex v4 = p(phi2, theta2);

            vertices.push_back(v1);
            vertices.push_back(v2);
            vertices.push_back(v3);
            vertices.push_back(v3);
            vertices.push_back(v2);
            vertices.push_back(v4);
        }
    }
    saveToFile(vertices, filename);
}

void generateCone(float radius, float height, int slices, int stacks, const string &filename)
{
    vector<Vertex> vertices;

    // Base (virada para -Y) -> CCW olhando de baixo
    for (int i = 0; i < slices; ++i)
    {
        float angle1 = (2.0 * M_PI * i) / slices;
        float angle2 = (2.0 * M_PI * (i + 1)) / slices;
        vertices.push_back({0, 0, 0});
        vertices.push_back({radius * (float)sin(angle2), 0, radius * (float)cos(angle2)});
        vertices.push_back({radius * (float)sin(angle1), 0, radius * (float)cos(angle1)});
        
    }

    // Lados (CCW virados para fora)
    float stackHeight = height / stacks;
    float stackRadius = radius / stacks;

    for (int i = 0; i < stacks; ++i)
    {
        float h1 = i * stackHeight, h2 = (i + 1) * stackHeight;
        float r1 = radius - i * stackRadius, r2 = radius - (i + 1) * stackRadius;

        for (int j = 0; j < slices; ++j)
        {
            float angle1 = (2.0 * M_PI * j) / slices;
            float angle2 = (2.0 * M_PI * (j + 1)) / slices;

            auto p = [&](float r, float h, float a) -> Vertex
            {
                return {r * (float)sin(a), h, r * (float)cos(a)};
            };

            Vertex v1 = p(r1, h1, angle1);
            Vertex v2 = p(r1, h1, angle2);
            Vertex v3 = p(r2, h2, angle1);
            Vertex v4 = p(r2, h2, angle2);

            vertices.push_back(v1);
            vertices.push_back(v2);
            vertices.push_back(v3);
            vertices.push_back(v3);
            vertices.push_back(v2);
            vertices.push_back(v4);
        }
    }
    saveToFile(vertices, filename);
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        cout << "Uso: generator <primitiva> [parametros] <ficheiro.3d>\n";
        return 1;
    }
    string primitive = argv[1];

    if (primitive == "plane" && argc == 5)
    {
        generatePlane(stof(argv[2]), stoi(argv[3]), argv[4]);
    }
    else if (primitive == "box" && argc == 5)
    {
        generateBox(stof(argv[2]), stoi(argv[3]), argv[4]);
    }
    else if (primitive == "sphere" && argc == 6)
    {
        generateSphere(stof(argv[2]), stoi(argv[3]), stoi(argv[4]), argv[5]);
    }
    else if (primitive == "cone" && argc == 7)
    {
        generateCone(stof(argv[2]), stof(argv[3]), stoi(argv[4]), stoi(argv[5]), argv[6]);
    }
    else
    {
        cout << "Argumentos invalidos.\n";
    }
    return 0;
}