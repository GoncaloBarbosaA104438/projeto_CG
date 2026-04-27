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
    float x = 0, y = 0, z = 0;
    float angle = 0;
    
    // --- NOVOS ATRIBUTOS PARA A FASE 3 ---
    float time = 0; 
    bool align = false;
    std::vector<Point> curvePoints;

    Transform() {}
};

struct Model {
    GLuint vbo;
    int vertexCount;
};

struct Group {
    std::vector<Transform> transforms;
    std::vector<Model> models;
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

// =========================================================================
// MATEMÁTICA PARA CATMULL-ROM (FASE 3)
// =========================================================================

void buildRotMatrix(float *x, float *y, float *z, float *m) {
    m[0] = x[0]; m[1] = x[1]; m[2] = x[2]; m[3] = 0;
    m[4] = y[0]; m[5] = y[1]; m[6] = y[2]; m[7] = 0;
    m[8] = z[0]; m[9] = z[1]; m[10] = z[2]; m[11] = 0;
    m[12] = 0; m[13] = 0; m[14] = 0; m[15] = 1;
}

void cross(float *a, float *b, float *res) {
    res[0] = a[1]*b[2] - a[2]*b[1];
    res[1] = a[2]*b[0] - a[0]*b[2];
    res[2] = a[0]*b[1] - a[1]*b[0];
}

void normalize(float *a) {
    float l = sqrt(a[0]*a[0] + a[1]*a[1] + a[2]*a[2]);
    if (l > 0.0) { a[0] = a[0]/l; a[1] = a[1]/l; a[2] = a[2]/l; }
}

void getCatmullRomPoint(float t, Point p0, Point p1, Point p2, Point p3, float *pos, float *deriv) {
    // Matriz Catmull-Rom
    float m[4][4] = { {-0.5f,  1.5f, -1.5f,  0.5f},
                      { 1.0f, -2.5f,  2.0f, -0.5f},
                      {-0.5f,  0.0f,  0.5f,  0.0f},
                      { 0.0f,  1.0f,  0.0f,  0.0f} };

    // Construir os vetores T
    float t_vec[4] = {t * t * t, t * t, t, 1};
    float t_deriv[4] = {3 * t * t, 2 * t, 1, 0};

    float pX[4] = {(float)p0.getX(), (float)p1.getX(), (float)p2.getX(), (float)p3.getX()};
    float pY[4] = {(float)p0.getY(), (float)p1.getY(), (float)p2.getY(), (float)p3.getY()};
    float pZ[4] = {(float)p0.getZ(), (float)p1.getZ(), (float)p2.getZ(), (float)p3.getZ()};

    // Calcular Pos e Derivada
    for (int i = 0; i < 3; i++) {
        pos[i] = 0;
        deriv[i] = 0;
        float a[4] = {0};

        float* p = (i == 0) ? pX : ((i == 1) ? pY : pZ);

        // Multiplicar Pela Matriz Catmull-Rom
        for (int j = 0; j < 4; j++) {
            a[j] = m[j][0]*p[0] + m[j][1]*p[1] + m[j][2]*p[2] + m[j][3]*p[3];
        }
        
        pos[i] = t_vec[0]*a[0] + t_vec[1]*a[1] + t_vec[2]*a[2] + t_vec[3]*a[3];
        deriv[i] = t_deriv[0]*a[0] + t_deriv[1]*a[1] + t_deriv[2]*a[2] + t_deriv[3]*a[3];
    }
}

void getGlobalCatmullRomPoint(float gt, float *pos, float *deriv, const std::vector<Point>& points) {
    int POINT_COUNT = points.size();
    float t = gt * POINT_COUNT; 
    int index = floor(t);  
    t = t - index; 

    int indices[4]; 
    indices[0] = (index + POINT_COUNT - 1) % POINT_COUNT;	
    indices[1] = (indices[0] + 1) % POINT_COUNT;
    indices[2] = (indices[1] + 1) % POINT_COUNT; 
    indices[3] = (indices[2] + 1) % POINT_COUNT;

    getCatmullRomPoint(t, points[indices[0]], points[indices[1]], points[indices[2]], points[indices[3]], pos, deriv);
}

// =========================================================================

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
            Transform t;
            
            if (opName == "translate") {
                t.type = TRANSLATE;
                // Verificar se é translação animada
                if (opElement->Attribute("time")) {
                    t.time = opElement->FloatAttribute("time");
                    t.align = opElement->BoolAttribute("align");
                    
                    XMLElement* pointEl = opElement->FirstChildElement("point");
                    while(pointEl) {
                        t.curvePoints.push_back(Point(pointEl->FloatAttribute("x"), pointEl->FloatAttribute("y"), pointEl->FloatAttribute("z")));
                        pointEl = pointEl->NextSiblingElement("point");
                    }
                } else { // Translação estática normal
                    t.x = opElement->FloatAttribute("x");
                    t.y = opElement->FloatAttribute("y");
                    t.z = opElement->FloatAttribute("z");
                }
                currentGroup.transforms.push_back(t);

            } else if (opName == "rotate") {
                t.type = ROTATE;
                t.x = opElement->FloatAttribute("x");
                t.y = opElement->FloatAttribute("y");
                t.z = opElement->FloatAttribute("z");
                
                if (opElement->Attribute("time")) {
                    t.time = opElement->FloatAttribute("time");
                } else {
                    t.angle = opElement->FloatAttribute("angle");
                }
                currentGroup.transforms.push_back(t);

            } else if (opName == "scale") {
                t.type = SCALE;
                t.x = opElement->FloatAttribute("x");
                t.y = opElement->FloatAttribute("y");
                t.z = opElement->FloatAttribute("z");
                currentGroup.transforms.push_back(t);
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

// A função de desenho agora passa o tempo como parâmetro para otimizar
void drawGroup(const Group& g, float timeInSeconds) {
    glPushMatrix(); 

    for (const auto& t : g.transforms) {
        if (t.type == TRANSLATE) {
            if (t.time > 0 && t.curvePoints.size() >= 4) { // Animação Catmull-Rom
                float pos[3], deriv[3];
                // Calcular o tempo global [0, 1]
                float globalT = fmod(timeInSeconds, t.time) / t.time;
                
                getGlobalCatmullRomPoint(globalT, pos, deriv, t.curvePoints);
                glTranslatef(pos[0], pos[1], pos[2]);

                if (t.align) {
                    float x[3], y[3], z[3], m[16];
                    
                    // X = vetor derivada (frente)
                    x[0] = deriv[0]; x[1] = deriv[1]; x[2] = deriv[2];
                    normalize(x);

                    // Up vector estático auxiliar
                    float up[3] = {0, 1, 0};
                    
                    // Z = X cross Up
                    cross(x, up, z);
                    normalize(z);
                    
                    // Y = Z cross X (para garantir que estão ortogonais)
                    cross(z, x, y);
                    normalize(y);

                    buildRotMatrix(x, y, z, m);
                    glMultMatrixf(m);
                }
            } else { // Translação estática
                glTranslatef(t.x, t.y, t.z);
            }

        } else if (t.type == ROTATE) {
            if (t.time > 0) { // Animação de Rotação Contínua
                float animAngle = (timeInSeconds * 360.0f) / t.time;
                glRotatef(animAngle, t.x, t.y, t.z);
            } else { // Rotação estática
                glRotatef(t.angle, t.x, t.y, t.z);
            }
            
        } else if (t.type == SCALE) {
            glScalef(t.x, t.y, t.z);
        }
    }

    glColor3f(1.0f, 1.0f, 1.0f);
    
    for (const auto &m : g.models) {
        glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
        glVertexPointer(3, GL_FLOAT, 0, 0);
        glDrawArrays(GL_TRIANGLES, 0, m.vertexCount);
    }

    for (const auto& child : g.children) drawGroup(child, timeInSeconds);

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

    // Ir buscar o tempo real numa única vez e converter para segundos
    float timeInSeconds = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;

    glEnableClientState(GL_VERTEX_ARRAY);
    drawGroup(sceneRoot, timeInSeconds); // Passa o tempo à função!
    glDisableClientState(GL_VERTEX_ARRAY);

    glutSwapBuffers();
    
    // Opcional mas recomendado: forçar a cena a redesenhar a cada frame para a animação ser fluída
    glutPostRedisplay(); 
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
    glutCreateWindow("CG26 - Fase 3 Animações");

    #ifndef __APPLE__
    glewInit();
    #endif

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