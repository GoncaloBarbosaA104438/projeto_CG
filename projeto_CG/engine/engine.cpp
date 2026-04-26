
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
#include <cmath>

#include "../point/point.hpp"
#include "../tinyXML/tinyxml2.h"

using namespace tinyxml2;

enum TransformType { TRANSLATE, ROTATE, SCALE };

struct Transform {
    TransformType type;
    float x, y, z;
    float angle;

    //NOVOS ATRIBUTOS FASE 3
    float time;                     // Tempo da animação em segundos (0 significa que é estático)
    bool align;                     // Indica se o objeto se alinha com a curva
    std::vector<Point> curvePoints; // Pontos da curva para animação (apenas relevante se time > 0)

    // Construtor original (Usado para Scale e Translate estático)
    Transform(TransformType t, float px, float py, float pz) 
        : type(t), x(px), y(py), z(pz), angle(0.0f), time(0.0f), align(false) {}
    
    // Construtor original (Usado para Rotate estático)
    Transform(TransformType t, float ang, float px, float py, float pz) 
        : type(t), angle(ang), x(px), y(py), z(pz), time(0.0f), align(false) {}

    // Novo construtor vazio para inicializar transformações dinâmicas linha a linha
    Transform(TransformType t) 
        : type(t), x(0.0f), y(0.0f), z(0.0f), angle(0.0f), time(0.0f), align(false) {}
};

struct Group {
    std::vector<Transform> transforms;
    std::vector<std::vector<Point>> models; 
    
    // Novas variáveis para os VBOs
    std::vector<GLuint> vbo_ids; 
    std::vector<int> vbo_vertex_counts; 
    
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

    // --- 1. LER TRANSFORMAÇÕES ---
    XMLElement* transformElement = groupElement->FirstChildElement("transform");
    if (transformElement) {
        XMLElement* opElement = transformElement->FirstChildElement();
        while (opElement) {
            std::string opName = opElement->Name();

            if (opName == "translate") {
                Transform t(TRANSLATE);
                
                // Lê o tempo de translação (se existir, é dinâmico)
                if (opElement->QueryFloatAttribute("time", &t.time) == XML_SUCCESS) {
                    // Lê o alinhamento
                    const char* alignAttr = opElement->Attribute("align");
                    if (alignAttr && (std::string(alignAttr) == "True" || std::string(alignAttr) == "true")) {
                        t.align = true;
                    }
                    
                    // Lê os pontos da curva (filhos <point>)
                    XMLElement* pointElement = opElement->FirstChildElement("point");
                    while (pointElement) {
                        float px = 0.0f, py = 0.0f, pz = 0.0f;
                        pointElement->QueryFloatAttribute("x", &px);
                        pointElement->QueryFloatAttribute("y", &py);
                        pointElement->QueryFloatAttribute("z", &pz);
                        t.curvePoints.push_back(Point(px, py, pz));
                        
                        pointElement = pointElement->NextSiblingElement("point");
                    }
                } else {
                    // Se não tem tempo, é uma translação estática
                    opElement->QueryFloatAttribute("x", &t.x);
                    opElement->QueryFloatAttribute("y", &t.y);
                    opElement->QueryFloatAttribute("z", &t.z);
                }
                
                currentGroup.transforms.push_back(t);

            } else if (opName == "rotate") {
                Transform t(ROTATE);
                
                // Lê os eixos de rotação
                opElement->QueryFloatAttribute("x", &t.x);
                opElement->QueryFloatAttribute("y", &t.y);
                opElement->QueryFloatAttribute("z", &t.z);
                
                // Verifica se é animada ('time') ou estática ('angle')
                if (opElement->QueryFloatAttribute("time", &t.time) != XML_SUCCESS) {
                    opElement->QueryFloatAttribute("angle", &t.angle);
                }
                
                currentGroup.transforms.push_back(t);

            } else if (opName == "scale") {
                Transform t(SCALE);
                opElement->QueryFloatAttribute("x", &t.x);
                opElement->QueryFloatAttribute("y", &t.y);
                opElement->QueryFloatAttribute("z", &t.z);
                currentGroup.transforms.push_back(t);
            }
            
            opElement = opElement->NextSiblingElement();
        }
    }

    // --- 2. LER MODELOS ---
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

    // --- 3. LER SUBGRUPOS ---
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

// Função que multiplica as matrizes para obter a posição e a derivada num segmento
void getCatmullRomPoint(float t, Point p0, Point p1, Point p2, Point p3, float *pos, float *deriv) {
    // Matriz de Catmull-Rom
    float m[4][4] = {
        {-0.5f,  1.5f, -1.5f,  0.5f},
        { 1.0f, -2.5f,  2.0f, -0.5f},
        {-0.5f,  0.0f,  0.5f,  0.0f},
        { 0.0f,  1.0f,  0.0f,  0.0f}
    };

    float Px[4] = {(float)p0.getX(), (float)p1.getX(), (float)p2.getX(), (float)p3.getX()};
    float Py[4] = {(float)p0.getY(), (float)p1.getY(), (float)p2.getY(), (float)p3.getY()};
    float Pz[4] = {(float)p0.getZ(), (float)p1.getZ(), (float)p2.getZ(), (float)p3.getZ()};

    // Vetores de tempo
    float T[4] = {t*t*t, t*t, t, 1};
    float T_deriv[4] = {3*t*t, 2*t, 1, 0};

    // Inicializar a zero
    pos[0] = 0.0; pos[1] = 0.0; pos[2] = 0.0;
    deriv[0] = 0.0; deriv[1] = 0.0; deriv[2] = 0.0;

    // Multiplicação T * M * P
    for (int i = 0; i < 4; i++) {
        float a = 0.0;
        float b = 0.0;
        for (int j = 0; j < 4; j++) {
            a += T[j] * m[j][i];
            b += T_deriv[j] * m[j][i];
        }
        pos[0] += a * Px[i];
        pos[1] += a * Py[i];
        pos[2] += a * Pz[i];

        deriv[0] += b * Px[i];
        deriv[1] += b * Py[i];
        deriv[2] += b * Pz[i];
    }
}

// Função que descobre em que segmento da curva estamos com base no tempo global (gt)
void getGlobalCatmullRomPoint(float gt, float *pos, float *deriv, const std::vector<Point>& points) {
    int POINT_COUNT = points.size();
    float t = gt * POINT_COUNT; 
    int index = floor(t);  
    t = t - index;        

    // Os índices dos 4 pontos necessários para a Catmull-Rom
    int p0 = (index - 1 + POINT_COUNT) % POINT_COUNT;
    int p1 = (index) % POINT_COUNT;
    int p2 = (index + 1) % POINT_COUNT;
    int p3 = (index + 2) % POINT_COUNT;

    getCatmullRomPoint(t, points[p0], points[p1], points[p2], points[p3], pos, deriv);
}

// Função para normalizar vetores (necessária para calcular a rotação correta do align)
void normalize(float *a) {
    float l = sqrt(a[0]*a[0] + a[1]*a[1] + a[2]*a[2]);
    if (l != 0) { a[0] /= l; a[1] /= l; a[2] /= l; }
}

void drawGroup(const Group& g) {
    glPushMatrix(); 

    // Ir buscar o tempo em segundos
    float currentTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;

    for (const auto& t : g.transforms) {
        if (t.type == TRANSLATE) {
            if (t.type == TRANSLATE) {
            if (t.time > 0.0f && t.curvePoints.size() >= 4) {
                // Cálculo da percentagem da viagem (gt vai de 0.0 a 1.0)
                float gt = fmod(currentTime, t.time) / t.time;
                
                float pos[3];
                float deriv[3];
                
                // Obtém a posição na curva e a direção para onde está a olhar
                getGlobalCatmullRomPoint(gt, pos, deriv, t.curvePoints);
                
                // 1. Move para a posição da curva
                glTranslatef(pos[0], pos[1], pos[2]);
                
                // 2. Alinha o objeto com a curva, se pedido
                if (t.align) {
                    float X[3] = {deriv[0], deriv[1], deriv[2]};
                    normalize(X);
                    
                    // Assumindo que o "up" geral do mundo é (0,1,0)
                    float up[3] = {0.0f, 1.0f, 0.0f};
                    
                    // Z = X cross UP
                    float Z[3];
                    Z[0] = X[1] * up[2] - X[2] * up[1];
                    Z[1] = X[2] * up[0] - X[0] * up[2];
                    Z[2] = X[0] * up[1] - X[1] * up[0];
                    normalize(Z);
                    
                    // Recalcular UP = Z cross X (para garantir ortogonalidade perfeita)
                    float Y[3];
                    Y[0] = Z[1] * X[2] - Z[2] * X[1];
                    Y[1] = Z[2] * X[0] - Z[0] * X[2];
                    Y[2] = Z[0] * X[1] - Z[1] * X[0];
                    normalize(Y);
                    
                    // Construir matriz de rotação
                    float m[16];
                    m[0] = X[0]; m[1] = X[1]; m[2] = X[2]; m[3] = 0;
                    m[4] = Y[0]; m[5] = Y[1]; m[6] = Y[2]; m[7] = 0;
                    m[8] = Z[0]; m[9] = Z[1]; m[10]= Z[2]; m[11]= 0;
                    m[12]= 0;    m[13]= 0;    m[14]= 0;    m[15]= 1;
                    
                    glMultMatrixf(m);
                }
            } else {
                // Translação estática
                glTranslatef(t.x, t.y, t.z);
            
        }
            } else {
                glTranslatef(t.x, t.y, t.z);
            }
        } else if (t.type == ROTATE) {
            if (t.time > 0.0f) {
                // Animação: Dá 360 graus a cada 't.time' segundos
                // A função fmod garante que o tempo reseta a cada volta (evita números gigantes)
                float dynamicAngle = (fmod(currentTime, t.time) / t.time) * 360.0f;
                glRotatef(dynamicAngle, t.x, t.y, t.z);
            } else {
                // Estático (Fase 2)
                glRotatef(t.angle, t.x, t.y, t.z);
            }
        } else if (t.type == SCALE) {
            glScalef(t.x, t.y, t.z);
        }
    }

    // ... (o resto da função mantém-se igual com os VBOs e os filhos)
    glColor3f(1.0f, 1.0f, 1.0f);
    glEnableClientState(GL_VERTEX_ARRAY);
    
    
    glEnableClientState(GL_VERTEX_ARRAY);

    for (size_t i = 0; i < g.vbo_ids.size(); i++) {
        glBindBuffer(GL_ARRAY_BUFFER, g.vbo_ids[i]);
        glVertexPointer(3, GL_FLOAT, 0, 0);
        glDrawArrays(GL_TRIANGLES, 0, g.vbo_vertex_counts[i]);
    }

    glDisableClientState(GL_VERTEX_ARRAY);

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

void prepareVBOs(Group& g) {
    for (const auto& solid : g.models) {
        std::vector<float> vertexData;
        for (const auto& p : solid) {
            vertexData.push_back(p.getX());
            vertexData.push_back(p.getY());
            vertexData.push_back(p.getZ());
        }

        GLuint vbo_id;
        glGenBuffers(1, &vbo_id);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
        glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);

        g.vbo_ids.push_back(vbo_id);
        g.vbo_vertex_counts.push_back(vertexData.size() / 3);
    }

    for (auto& child : g.children) {
        prepareVBOs(child);
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Uso: ./engine <caminho_xml>\n");
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
    glutCreateWindow("CG26 - Fase 3");

    glEnable(GL_DEPTH_TEST);

    // === INICIALIZAR VBOs AQUI ===
    prepareVBOs(sceneRoot);


    glutDisplayFunc(renderScene);
    glutIdleFunc(renderScene);
    glutKeyboardFunc(keyboardFunc);
    glutSpecialFunc(specialKeysFunc);
    glutReshapeFunc(changeSize);

    glEnable(GL_DEPTH_TEST);

    glutMainLoop();
    return 0;
}