#include <stdlib.h>

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
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

struct Group {
    std::vector<Transform> transforms;
    std::vector<std::vector<Point>> models;
    std::vector<Group> children;
};

// Estruturas para guardar as configurações lidas do XML
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

// Valores iniciais (serão substituídos pelos do XML no loadScene)
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
    
    // Atualizado para usar os valores lidos do XML
    gluPerspective(cameraSettings.fov, ratio, cameraSettings.nearPlane, cameraSettings.farPlane);
    
    glMatrixMode(GL_MODELVIEW);
}

void draw_axis() {
    glBegin(GL_LINES);
    
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(-100.0f, 0.0f, 0.0f);
    glVertex3f(100.0f, 0.0f, 0.0f);
    
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f(0.0f, -100.0f, 0.0f);
    glVertex3f(0.0f, 100.0f, 0.0f);
   
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f(0.0f, 0.0f, -100.0f);
    glVertex3f(0.0f, 0.0f, 100.0f);
    glEnd();
}

std::vector<Point> vectorize(const char *filename) {
    std::ifstream file(filename);
    if (!file.good()) {
        file.open(std::string("../").append(filename).c_str());
        if (!file.good()) {
            printf("Error opening file %s\n", filename);
            exit(1);
        }
    }

    std::string line;
    std::vector<Point> solid;
    printf("Reading %s\n", filename);
    std::getline(file, line);

    unsigned long N = std::stoul(line);
    solid.reserve(N);
    for (unsigned long i = 0; i < N; i++) {
        std::getline(file, line);
        double a, b, c;
        int matches = sscanf(line.c_str(), "%lf %lf %lf", &a, &b, &c);
        if (matches != 3) {
            printf("ERROR - invalid number of points in vertex %s\n", line.c_str());
            exit(1);
        }
        solid.push_back(Point(a, b, c));
    }
    file.close();
    printf("Finished reading %s\n", filename);
    return solid;
}

Group parseGroup(XMLElement* groupElement) {
    Group currentGroup;

    XMLElement* transformElement = groupElement->FirstChildElement("transform");
    if (transformElement) {
        XMLElement* opElement = transformElement->FirstChildElement();
        while (opElement) {
            std::string opName = opElement->Name();
            if (opName == "translate") {
                float x = opElement->FloatAttribute("x");
                float y = opElement->FloatAttribute("y");
                float z = opElement->FloatAttribute("z");
                currentGroup.transforms.push_back(Transform(TRANSLATE, x, y, z));
            } else if (opName == "rotate") {
                float angle = opElement->FloatAttribute("angle");
                float x = opElement->FloatAttribute("x");
                float y = opElement->FloatAttribute("y");
                float z = opElement->FloatAttribute("z");
                currentGroup.transforms.push_back(Transform(ROTATE, angle, x, y, z));
            } else if (opName == "scale") {
                float x = opElement->FloatAttribute("x");
                float y = opElement->FloatAttribute("y");
                float z = opElement->FloatAttribute("z");
                currentGroup.transforms.push_back(Transform(SCALE, x, y, z));
            }
            opElement = opElement->NextSiblingElement();
        }
    }

    XMLElement* modelsElement = groupElement->FirstChildElement("models");
    if (modelsElement) {
        XMLElement* modelElement = modelsElement->FirstChildElement("model");
        while (modelElement) {
            const char* file = modelElement->Attribute("file");
            if (file) {
                currentGroup.models.push_back(vectorize(file));
            }
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

    // --- LER WINDOW ---
    XMLElement *window = world->FirstChildElement("window");
    if (window) {
        window->QueryIntAttribute("width", &windowSettings.width);
        window->QueryIntAttribute("height", &windowSettings.height);
    }

    // --- LER CAMERA ---
    XMLElement *camera = world->FirstChildElement("camera");
    if (camera) {
        XMLElement *pos = camera->FirstChildElement("position");
        if (pos) {
            pos->QueryFloatAttribute("x", &cameraSettings.posX);
            pos->QueryFloatAttribute("y", &cameraSettings.posY);
            pos->QueryFloatAttribute("z", &cameraSettings.posZ);
            
            // Converter Cartesianas para Polares para não quebrar os controlos de teclado
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

    // --- LER GRUPOS ---
    XMLElement *rootGroupElement = world->FirstChildElement("group");
    if (rootGroupElement) {
        sceneRoot = parseGroup(rootGroupElement);
    }
    return true;
}

void drawGroup(const Group& g) {
    glPushMatrix(); 

    for (const auto& t : g.transforms) {
        if (t.type == TRANSLATE) {
            glTranslatef(t.x, t.y, t.z);
        } else if (t.type == ROTATE) {
            glRotatef(t.angle, t.x, t.y, t.z);
        } else if (t.type == SCALE) {
            glScalef(t.x, t.y, t.z);
        }
    }

    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_TRIANGLES);
    for (const auto &solid : g.models) {
        for (const auto &point : solid) {
            glVertex3f(point.getX(), point.getY(), point.getZ());
        }
    }
    glEnd();

    for (const auto& child : g.children) {
        drawGroup(child);
    }

    glPopMatrix(); 
}

void renderScene() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();
    
    // Atualizado para usar os valores lidos do XML (com camPos para interatividade)
    gluLookAt(polarX(camPos), polarY(camPos), polarZ(camPos),
              cameraSettings.lookX, cameraSettings.lookY, cameraSettings.lookZ,
              cameraSettings.upX, cameraSettings.upY, cameraSettings.upZ);

    draw_axis();

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    drawGroup(sceneRoot);

    glutSwapBuffers();
}

void keyboardFunc(unsigned char key, int x, int y) {
    switch (key) {
    case '+':
        if (camPos.radius > 1) camPos.radius -= 1;
        break;
    case '-':
        camPos.radius += 1;
        break;
    }
    glutPostRedisplay();
}

void specialKeysFunc(int key, int x, int y) {
    switch (key) {
    case GLUT_KEY_LEFT:  camPos.alpha -= M_PI / 16; break;
    case GLUT_KEY_RIGHT: camPos.alpha += M_PI / 16; break;
    case GLUT_KEY_DOWN:  camPos.beta -= M_PI / 16; break;
    case GLUT_KEY_UP:    camPos.beta += M_PI / 16; break;
    }

    if (camPos.alpha < 0) camPos.alpha += M_PI * 2;
    else if (camPos.alpha > M_PI * 2) camPos.alpha -= M_PI * 2;

    if (camPos.beta < -M_PI_2) camPos.beta += M_PI * 2;
    else if (camPos.beta > (3 * M_PI_2)) camPos.beta -= M_PI * 2;

    glutPostRedisplay();
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Uso: ./engine <caminho_para_xml>\n");
        return 1;
    }

    // Carregar a cena *antes* de inicializar o GLUT garante que temos a largura e altura corretas
    if (!loadScene(argv[1])) {
        puts("Erro ao carregar a cena XML!");
        return 1;
    }

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowPosition(100, 100);
    
    // Atualizado para usar os valores lidos do XML
    glutInitWindowSize(windowSettings.width, windowSettings.height);
    glutCreateWindow("CG26 - Fase 2");

    glutDisplayFunc(renderScene);
    glutKeyboardFunc(keyboardFunc);
    glutSpecialFunc(specialKeysFunc);
    glutReshapeFunc(changeSize);

    glEnable(GL_DEPTH_TEST);

    glutMainLoop();
    return 0;
}