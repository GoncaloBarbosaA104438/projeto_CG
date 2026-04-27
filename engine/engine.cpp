#include <stdlib.h>

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glew.h>
#include <GL/glut.h>
#endif

#define _USE_MATH_DEFINES
#include <math.h>

#include <stdio.h>
#include <fstream>
#include <string>
#include <vector>

#include "../point/point.hpp"
#include "../tinyXML/tinyxml2.h"

using namespace tinyxml2;

enum TransformType { TRANSLATE, ROTATE, SCALE };

struct Transform {
    TransformType type;
    float x, y, z;
    float angle;

    Transform(TransformType t, float px, float py, float pz) 
        : type(t), x(px), y(py), z(pz), angle(0.0f) {}
    Transform(TransformType t, float ang, float px, float py, float pz) 
        : type(t), angle(ang), x(px), y(py), z(pz) {}
};

// Estrutura para armazenar os IDs dos VBOs na GPU [cite: 48, 137]
struct Model {
    GLuint vbo;
    int vertexCount;
};

struct Group {
    std::vector<Transform> transforms;
    std::vector<Model> models; // Agora guarda referências para VBOs
    std::vector<Group> children;
};

struct WindowSettings {
    int width = 800;
    int height = 800;
} windowSettings;

struct CameraSettings {
    float posX = 0, posY = 50, posZ = 100;
    float lookX = 0, lookY = 0, lookZ = 0;
    float upX = 0, upY = 1, upZ = 0;
    float fov = 60, nearPlane = 1, farPlane = 1000;
} cameraSettings;

XMLDocument doc;
Group sceneRoot;

struct Polar {
    double radius;
    double alpha;
    double beta;
};

Polar camPos = {sqrt(75), M_PI_4, M_PI_4}; 

double polarX(Polar polar) { return polar.radius * cos(polar.beta) * sin(polar.alpha); }
double polarY(Polar polar) { return polar.radius * sin(polar.beta); }
double polarZ(Polar polar) { return polar.radius * cos(polar.beta) * cos(polar.alpha); }

void changeSize(int width, int height) {
    if (height == 0) height = 1;
    float ratio = width * 1.0 / height;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0, 0, width, height);
    gluPerspective(cameraSettings.fov, ratio, cameraSettings.nearPlane, cameraSettings.farPlane);
    glMatrixMode(GL_MODELVIEW);
}

void draw_axis() {
    glBegin(GL_LINES);
    glColor3f(1.0f, 0.0f, 0.0f); glVertex3f(-100.0f, 0.0f, 0.0f); glVertex3f(100.0f, 0.0f, 0.0f);
    glColor3f(0.0f, 1.0f, 0.0f); glVertex3f(0.0f, -100.0f, 0.0f); glVertex3f(0.0f, 100.0f, 0.0f);
    glColor3f(0.0f, 0.0f, 1.0f); glVertex3f(0.0f, 0.0f, -100.0f); glVertex3f(0.0f, 0.0f, 100.0f);
    glEnd();
}

// Função atualizada para carregar dados diretamente para VBOs 
Model vectorize(const char *filename) {
    std::ifstream file(filename);
    if (!file.good()) {
        file.open(std::string("../").append(filename).c_str());
        if (!file.good()) {
            printf("Error opening file %s\n", filename);
            exit(1);
        }
    }

    std::string line;
    std::vector<float> vertexData;
    printf("Reading %s\n", filename);
    std::getline(file, line);

    unsigned long N = std::stoul(line);
    vertexData.reserve(N * 3);
    for (unsigned long i = 0; i < N; i++) {
        std::getline(file, line);
        float a, b, c;
        sscanf(line.c_str(), "%f %f %f", &a, &b, &c);
        vertexData.push_back(a);
        vertexData.push_back(b);
        vertexData.push_back(c);
    }
    file.close();

    Model m;
    m.vertexCount = N;
    
    // Gerar e preencher o buffer na GPU 
    glGenBuffers(1, &m.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);

    printf("Finished reading %s - VBO ID: %d\n", filename, m.vbo);
    return m;
}

Group parseGroup(XMLElement* groupElement) {
    Group currentGroup;

    XMLElement* transformElement = groupElement->FirstChildElement("transform");
    if (transformElement) {
        XMLElement* opElement = transformElement->FirstChildElement();
        while (opElement) {
            std::string opName = opElement->Name();
            if (opName == "translate") {
                currentGroup.transforms.push_back(Transform(TRANSLATE, opElement->FloatAttribute("x"), opElement->FloatAttribute("y"), opElement->FloatAttribute("z")));
            } else if (opName == "rotate") {
                currentGroup.transforms.push_back(Transform(ROTATE, opElement->FloatAttribute("angle"), opElement->FloatAttribute("x"), opElement->FloatAttribute("y"), opElement->FloatAttribute("z")));
            } else if (opName == "scale") {
                currentGroup.transforms.push_back(Transform(SCALE, opElement->FloatAttribute("x"), opElement->FloatAttribute("y"), opElement->FloatAttribute("z")));
            }
            opElement = opElement->NextSiblingElement();
        }
    }

    XMLElement* modelsElement = groupElement->FirstChildElement("models");
    if (modelsElement) {
        XMLElement* modelElement = modelsElement->FirstChildElement("model");
        while (modelElement) {
            const char* file = modelElement->Attribute("file");
            if (file) currentGroup.models.push_back(vectorize(file));
            modelElement = modelElement->NextSiblingElement("model");
        }
    }

    XMLElement* childGroupElement = groupElement->FirstChildElement("group");
    while (childGroupElement) {
        currentGroup.children.push_back(parseGroup(childGroupElement));
        childGroupElement = childGroupElement->NextSiblingElement("group");
    }

    return currentGroup;
}

bool loadScene(const char *filename) {
    doc.LoadFile(filename);
    if (doc.ErrorID()) {
        doc.LoadFile(std::string("../").append(filename).c_str());
        if (doc.ErrorID()) return false;
    }

    XMLElement *world = doc.FirstChildElement("world");
    if (!world) return false;

    XMLElement *window = world->FirstChildElement("window");
    if (window) {
        window->QueryIntAttribute("width", &windowSettings.width);
        window->QueryIntAttribute("height", &windowSettings.height);
    }

    XMLElement *camera = world->FirstChildElement("camera");
    if (camera) {
        XMLElement *pos = camera->FirstChildElement("position");
        if (pos) {
            pos->QueryFloatAttribute("x", &cameraSettings.posX);
            pos->QueryFloatAttribute("y", &cameraSettings.posY);
            pos->QueryFloatAttribute("z", &cameraSettings.posZ);
            camPos.radius = sqrt(pow(cameraSettings.posX, 2) + pow(cameraSettings.posY, 2) + pow(cameraSettings.posZ, 2));
            if (camPos.radius != 0) {
                camPos.beta = asin(cameraSettings.posY / camPos.radius);
                camPos.alpha = atan2(cameraSettings.posX, cameraSettings.posZ);
            }
        }
        XMLElement *look = camera->FirstChildElement("lookAt");
        if (look) {
            look->QueryFloatAttribute("x", &cameraSettings.lookX);
            look->QueryFloatAttribute("y", &cameraSettings.lookY);
            look->QueryFloatAttribute("z", &cameraSettings.lookZ);
        }
        XMLElement *up = camera->FirstChildElement("up");
        if (up) {
            up->QueryFloatAttribute("x", &cameraSettings.upX);
            up->QueryFloatAttribute("y", &cameraSettings.upY);
            up->QueryFloatAttribute("z", &cameraSettings.upZ);
        }
        XMLElement *proj = camera->FirstChildElement("projection");
        if (proj) {
            proj->QueryFloatAttribute("fov", &cameraSettings.fov);
            proj->QueryFloatAttribute("near", &cameraSettings.nearPlane);
            proj->QueryFloatAttribute("far", &cameraSettings.farPlane);
        }
    }

    XMLElement *rootGroupElement = world->FirstChildElement("group");
    if (rootGroupElement) sceneRoot = parseGroup(rootGroupElement);

    return true;
}

// Função de desenho atualizada para VBOs 
void drawGroup(const Group& g) {
    glPushMatrix(); 

    for (const auto& t : g.transforms) {
        if (t.type == TRANSLATE) glTranslatef(t.x, t.y, t.z);
        else if (t.type == ROTATE) glRotatef(t.angle, t.x, t.y, t.z);
        else if (t.type == SCALE) glScalef(t.x, t.y, t.z);
    }

    glColor3f(1.0f, 1.0f, 1.0f);
    
    // Renderização via Vertex Arrays 
    for (const auto &m : g.models) {
        glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
        glVertexPointer(3, GL_FLOAT, 0, 0);
        glDrawArrays(GL_TRIANGLES, 0, m.vertexCount);
    }

    for (const auto& child : g.children) drawGroup(child);

    glPopMatrix(); 
}

void renderScene() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    
    gluLookAt(polarX(camPos), polarY(camPos), polarZ(camPos),
              cameraSettings.lookX, cameraSettings.lookY, cameraSettings.lookZ,
              cameraSettings.upX, cameraSettings.upY, cameraSettings.upZ);

    draw_axis();
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // Ativar o estado para desenho por arrays 
    glEnableClientState(GL_VERTEX_ARRAY);
    drawGroup(sceneRoot);
    glDisableClientState(GL_VERTEX_ARRAY);

    glutSwapBuffers();
}

void keyboardFunc(unsigned char key, int x, int y) {
    if (key == '+') { if (camPos.radius > 1) camPos.radius -= 1; }
    else if (key == '-') camPos.radius += 1;
    glutPostRedisplay();
}

void specialKeysFunc(int key, int x, int y) {
    switch (key) {
        case GLUT_KEY_LEFT:  camPos.alpha -= M_PI / 16; break;
        case GLUT_KEY_RIGHT: camPos.alpha += M_PI / 16; break;
        case GLUT_KEY_DOWN:  camPos.beta -= M_PI / 16; break;
        case GLUT_KEY_UP:    camPos.beta += M_PI / 16; break;
    }
    if (camPos.beta < -M_PI_2 + 0.01) camPos.beta = -M_PI_2 + 0.01;
    if (camPos.beta > M_PI_2 - 0.01) camPos.beta = M_PI_2 - 0.01;
    glutPostRedisplay();
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Uso: ./engine <caminho_para_xml>\n");
        return 1;
    }

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowPosition(100, 100);
    glutInitWindowSize(800, 800);
    glutCreateWindow("CG26 - Fase 3 VBOs");

    #ifndef __APPLE__
    glewInit(); // Inicializa GLEW se não estiver em Mac 
    #endif

    // IMPORTANTE: loadScene tem de vir DEPOIS do glutCreateWindow para o OpenGL inicializar VBOs
    if (!loadScene(argv[1])) {
        puts("Erro ao carregar a cena XML!");
        return 1;
    }

    glutReshapeWindow(windowSettings.width, windowSettings.height);

    glutDisplayFunc(renderScene);
    glutKeyboardFunc(keyboardFunc);
    glutSpecialFunc(specialKeysFunc);
    glutReshapeFunc(changeSize);

    glEnable(GL_DEPTH_TEST);
    glutMainLoop();
    return 0;
}