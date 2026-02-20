#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cmath>

// Bibliotecas do OpenGL e GLUT
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

// Biblioteca para ler o XML (TinyXML-2)
#include "tinyxml2.h"

using namespace std;
using namespace tinyxml2;

// Estrutura para os vértices
struct Vertex
{
    float x, y, z;
};

// Armazenar os modelos carregados
vector<vector<Vertex>> models;

// Variáveis da Janela
int winWidth = 800, winHeight = 600;

// Variáveis da Câmara (lidas do XML)
float camX = 10.0f, camY = 10.0f, camZ = 10.0f;
float lookX = 0.0f, lookY = 0.0f, lookZ = 0.0f;
float upX = 0.0f, upY = 1.0f, upZ = 0.0f;
float fov = 60.0f, nearP = 1.0f, farP = 1000.0f;

// Variáveis para a câmara esférica (para interagires com o teclado)
float alpha_angle = 0.0f;
float beta_angle = M_PI / 4.0f;
float radius_cam = 15.0f;

// Modo de desenho (GL_FILL, GL_LINE, GL_POINT)
GLenum polyMode = GL_FILL;

// ---------------------------------------------------------
// FUNÇÃO PARA LER FICHEIROS .3D
// ---------------------------------------------------------
void load3DFile(const string &filename)
{
    ifstream file(filename);
    if (!file.is_open())
    {
        cerr << "Erro: Nao foi possivel abrir o ficheiro " << filename << endl;
        return;
    }

    int numVertices;
    file >> numVertices;

    vector<Vertex> vertices;
    for (int i = 0; i < numVertices; ++i)
    {
        Vertex v;
        file >> v.x >> v.y >> v.z;
        vertices.push_back(v);
    }
    models.push_back(vertices);
    file.close();
    cout << "Modelo '" << filename << "' carregado com " << numVertices << " vertices." << endl;
}

// ---------------------------------------------------------
// FUNÇÃO PARA LER O FICHEIRO XML
// ---------------------------------------------------------
void loadXML(const string &filename)
{
    XMLDocument doc;
    if (doc.LoadFile(filename.c_str()) != XML_SUCCESS)
    {
        cerr << "Erro ao carregar o ficheiro XML: " << filename << endl;
        return;
    }

    XMLElement *world = doc.FirstChildElement("world");
    if (!world)
        return;

    // LER WINDOW
    XMLElement *window = world->FirstChildElement("window");
    if (window)
    {
        window->QueryIntAttribute("width", &winWidth);
        window->QueryIntAttribute("height", &winHeight);
    }

    // LER CAMERA
    XMLElement *camera = world->FirstChildElement("camera");
    if (camera)
    {
        XMLElement *pos = camera->FirstChildElement("position");
        if (pos)
        {
            pos->QueryFloatAttribute("x", &camX);
            pos->QueryFloatAttribute("y", &camY);
            pos->QueryFloatAttribute("z", &camZ);
        }

        XMLElement *look = camera->FirstChildElement("lookAt");
        if (look)
        {
            look->QueryFloatAttribute("x", &lookX);
            look->QueryFloatAttribute("y", &lookY);
            look->QueryFloatAttribute("z", &lookZ);
        }

        XMLElement *up = camera->FirstChildElement("up");
        if (up)
        {
            up->QueryFloatAttribute("x", &upX);
            up->QueryFloatAttribute("y", &upY);
            up->QueryFloatAttribute("z", &upZ);
        }

        XMLElement *proj = camera->FirstChildElement("projection");
        if (proj)
        {
            proj->QueryFloatAttribute("fov", &fov);
            proj->QueryFloatAttribute("near", &nearP);
            proj->QueryFloatAttribute("far", &farP);
        }

        // Inicializar raio e ângulos da câmara esférica a partir do XML
        radius_cam = sqrt(camX * camX + camY * camY + camZ * camZ);
        beta_angle = asin(camY / radius_cam);
        alpha_angle = atan2(camX, camZ);
    }

    // LER MODELOS (Dentro do <group>)
    XMLElement *group = world->FirstChildElement("group");
    if (group)
    {
        XMLElement *modelsNode = group->FirstChildElement("models");
        if (modelsNode)
        {
            XMLElement *model = modelsNode->FirstChildElement("model");
            while (model)
            {
                const char *fileAttr = model->Attribute("file");
                if (fileAttr)
                {
                    load3DFile(fileAttr);
                }
                model = model->NextSiblingElement("model");
            }
        }
    }
}

// ---------------------------------------------------------
// CALLBACKS DO OPENGL
// ---------------------------------------------------------
void changeSize(int w, int h)
{
    if (h == 0)
        h = 1;
    float ratio = w * 1.0f / h;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0, 0, w, h);
    gluPerspective(fov, ratio, nearP, farP);
    glMatrixMode(GL_MODELVIEW);
}

void renderScene(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Atualizar posição da câmara (Coordenadas Esféricas)
    camX = radius_cam * cos(beta_angle) * sin(alpha_angle);
    camY = radius_cam * sin(beta_angle);
    camZ = radius_cam * cos(beta_angle) * cos(alpha_angle);

    gluLookAt(camX, camY, camZ,
              lookX, lookY, lookZ,
              upX, upY, upZ);

    // Definir como queremos desenhar (Sólido, Linhas ou Pontos)
    glPolygonMode(GL_FRONT_AND_BACK, polyMode);

    // Desenhar os eixos XYZ
    glBegin(GL_LINES);
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(100.0f, 0.0f, 0.0f); // X Vermelho
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 100.0f, 0.0f); // Y Verde
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 100.0f); // Z Azul
    glEnd();

    glPolygonMode(GL_FRONT_AND_BACK, polyMode);
    // Desenhar todos os modelos
    glBegin(GL_TRIANGLES);
    glColor3f(1.0f, 1.0f, 1.0f); // Branco para os modelos
    for (const auto &model : models)
    {
        for (const auto &v : model)
        {
            glVertex3f(v.x, v.y, v.z);
        }
    }
    glEnd();

    glutSwapBuffers();
}

// ---------------------------------------------------------
// INPUT DO TECLADO (Interatividade)
// ---------------------------------------------------------
void processKeys(unsigned char key, int xx, int yy)
{
    switch (key)
    {
    case '1':
        polyMode = GL_FILL;
        break;
    case '2':
        polyMode = GL_LINE;
        break;
    case '3':
        polyMode = GL_POINT;
        break;
    }
    glutPostRedisplay();
}

void processSpecialKeys(int key, int xx, int yy)
{
    switch (key)
    {
    case GLUT_KEY_RIGHT:
        alpha_angle -= 0.1f;
        break;
    case GLUT_KEY_LEFT:
        alpha_angle += 0.1f;
        break;
    case GLUT_KEY_UP:
        beta_angle += 0.1f;
        if (beta_angle > 1.5f)
            beta_angle = 1.5f;
        break;
    case GLUT_KEY_DOWN:
        beta_angle -= 0.1f;
        if (beta_angle < -1.5f)
            beta_angle = -1.5f;
        break;
    }
    glutPostRedisplay();
}

// ---------------------------------------------------------
// FUNÇÃO MAIN
// ---------------------------------------------------------
int main(int argc, char **argv)
{
    if (argc < 2)
    {
        cout << "Uso: engine <ficheiro_configuracao.xml>\n";
        return 1;
    }

    // 1. LER O XML ANTES DE CRIAR A JANELA
    loadXML(argv[1]);

    // 2. INICIALIZAR O GLUT E OPENGL
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowPosition(100, 100);
    glutInitWindowSize(winWidth, winHeight);
    glutCreateWindow("Motor 3D - Fase 1");

    // Registar Callbacks
    glutDisplayFunc(renderScene);
    glutReshapeFunc(changeSize);
    glutKeyboardFunc(processKeys);
    glutSpecialFunc(processSpecialKeys);

    // Activar o Z-Buffer e Face Culling (Regra CCW que falámos)
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    cout << "Motor 3D Iniciado! Use as SETAS para rodar a camara e 1, 2, 3 para mudar o modo de desenho." << endl;

    // Entrar no ciclo principal do OpenGL
    glutMainLoop();

    return 1;
}