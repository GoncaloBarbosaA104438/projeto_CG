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

#define STB_IMAGE_IMPLEMENTATION
#include "../include/stb_image.h"

#include "../point/point.hpp"
#include "../tinyXML/tinyxml2.h"

using namespace tinyxml2;

// =========================================================================
// DATA STRUCTURES
// =========================================================================

enum TransformType
{
    TRANSLATE,
    ROTATE,
    SCALE
};

struct Transform
{
    TransformType type;
    float x = 0, y = 0, z = 0, angle = 0;
    float time = 0;
    bool align = false;
    std::vector<Point> curvePoints;
};

struct Material
{
    float diffuse[4] = {0.8f, 0.8f, 0.8f, 1.0f};
    float ambient[4] = {0.2f, 0.2f, 0.2f, 1.0f};
    float specular[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    float emissive[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    float shininess = 0.0f;
};

struct Model
{
    GLuint vertexVBO = 0;
    GLuint normalVBO = 0;
    GLuint texcoordVBO = 0;
    int vertexCount = 0;
    GLuint textureID = 0;
    Material material;
};

struct Group
{
    std::vector<Transform> transforms;
    std::vector<Model> models;
    std::vector<Group> children;
};

enum LightType
{
    LIGHT_POINT,
    LIGHT_DIRECTIONAL,
    LIGHT_SPOT
};

struct Light
{
    LightType type = LIGHT_POINT;
    float posX = 0, posY = 0, posZ = 0;
    float dirX = 0, dirY = -1, dirZ = 0;
    float cutoff = 180.0f;
};

// =========================================================================
// GLOBALS
// =========================================================================

struct WindowSettings
{
    int width = 800, height = 800;
} windowSettings;

struct CameraSettings
{
    float posX = 0, posY = 50, posZ = 100;
    float lookX = 0, lookY = 0, lookZ = 0;
    float upX = 0, upY = 1, upZ = 0;
    float fov = 60, nearPlane = 1, farPlane = 1000;
} cameraSettings;

struct Polar
{
    double radius, alpha, beta;
};
Polar camPos = {sqrt(75), M_PI_4, M_PI_4};

double polarX(Polar p) { return p.radius * cos(p.beta) * sin(p.alpha); }
double polarY(Polar p) { return p.radius * sin(p.beta); }
double polarZ(Polar p) { return p.radius * cos(p.beta) * cos(p.alpha); }

XMLDocument doc;
Group sceneRoot;
std::vector<Light> sceneLights;

// =========================================================================
// CAMERA / WINDOW
// =========================================================================

void changeSize(int width, int height)
{
    if (height == 0)
        height = 1;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0, 0, width, height);
    gluPerspective(cameraSettings.fov, (float)width / height,
                   cameraSettings.nearPlane, cameraSettings.farPlane);
    glMatrixMode(GL_MODELVIEW);
}

void draw_axis()
{
    glDisable(GL_LIGHTING);
    glBegin(GL_LINES);
    glColor3f(1, 0, 0);
    glVertex3f(-100, 0, 0);
    glVertex3f(100, 0, 0);
    glColor3f(0, 1, 0);
    glVertex3f(0, -100, 0);
    glVertex3f(0, 100, 0);
    glColor3f(0, 0, 1);
    glVertex3f(0, 0, -100);
    glVertex3f(0, 0, 100);
    glEnd();
    glEnable(GL_LIGHTING);
}

// =========================================================================
// CATMULL-ROM
// =========================================================================

void buildRotMatrix(float *x, float *y, float *z, float *m)
{
    m[0] = x[0];
    m[1] = x[1];
    m[2] = x[2];
    m[3] = 0;
    m[4] = y[0];
    m[5] = y[1];
    m[6] = y[2];
    m[7] = 0;
    m[8] = z[0];
    m[9] = z[1];
    m[10] = z[2];
    m[11] = 0;
    m[12] = 0;
    m[13] = 0;
    m[14] = 0;
    m[15] = 1;
}

void cross3(float *a, float *b, float *r)
{
    r[0] = a[1] * b[2] - a[2] * b[1];
    r[1] = a[2] * b[0] - a[0] * b[2];
    r[2] = a[0] * b[1] - a[1] * b[0];
}

void normalize3(float *a)
{
    float l = sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
    if (l > 0)
    {
        a[0] /= l;
        a[1] /= l;
        a[2] /= l;
    }
}

void getCatmullRomPoint(float t, Point p0, Point p1, Point p2, Point p3,
                        float *pos, float *deriv)
{
    float m[4][4] = {{-0.5f, 1.5f, -1.5f, 0.5f}, {1.0f, -2.5f, 2.0f, -0.5f}, {-0.5f, 0.0f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f}};
    float tv[4] = {t * t * t, t * t, t, 1};
    float td[4] = {3 * t * t, 2 * t, 1, 0};
    float pts[3][4] = {{(float)p0.getX(), (float)p1.getX(), (float)p2.getX(), (float)p3.getX()},
                       {(float)p0.getY(), (float)p1.getY(), (float)p2.getY(), (float)p3.getY()},
                       {(float)p0.getZ(), (float)p1.getZ(), (float)p2.getZ(), (float)p3.getZ()}};
    for (int i = 0; i < 3; i++)
    {
        float a[4] = {0};
        for (int j = 0; j < 4; j++)
            a[j] = m[j][0] * pts[i][0] + m[j][1] * pts[i][1] + m[j][2] * pts[i][2] + m[j][3] * pts[i][3];
        pos[i] = tv[0] * a[0] + tv[1] * a[1] + tv[2] * a[2] + tv[3] * a[3];
        deriv[i] = td[0] * a[0] + td[1] * a[1] + td[2] * a[2] + td[3] * a[3];
    }
}

void getGlobalCatmullRomPoint(float gt, float *pos, float *deriv,
                              const std::vector<Point> &pts)
{
    int N = pts.size();
    float t = gt * N;
    int idx = floor(t);
    t -= idx;
    int i0 = (idx + N - 1) % N, i1 = idx % N, i2 = (idx + 1) % N, i3 = (idx + 2) % N;
    getCatmullRomPoint(t, pts[i0], pts[i1], pts[i2], pts[i3], pos, deriv);
}

// =========================================================================
// TEXTURE LOADING
// =========================================================================

GLuint loadTexture(const char *filename)
{
    // Try direct path, then ../path
    int w, h, ch;
    stbi_set_flip_vertically_on_load(1);
    unsigned char *data = stbi_load(filename, &w, &h, &ch, 0);
    if (!data)
    {
        std::string alt = std::string("../") + filename;
        data = stbi_load(alt.c_str(), &w, &h, &ch, 0);
    }
    if (!data)
    {
        printf("Texture not found: %s (skipping)\n", filename);
        return 0;
    }

    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);

    GLenum fmt = (ch == 4) ? GL_RGBA : GL_RGB;
    gluBuild2DMipmaps(GL_TEXTURE_2D, fmt, w, h, fmt, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    stbi_image_free(data);
    printf("Loaded texture '%s' (%dx%d ch=%d) → GL#%u\n", filename, w, h, ch, id);
    return id;
}

// =========================================================================
// MODEL LOADING
// =========================================================================

Model vectorize(const char *filename, const char *texFile,
                const Material &mat)
{
    std::ifstream file(filename);
    if (!file.good())
    {
        file.open(std::string("../").append(filename).c_str());
        if (!file.good())
        {
            printf("Cannot open model: %s\n", filename);
            exit(1);
        }
    }

    std::string line;
    std::getline(file, line);
    unsigned long N = std::stoul(line);

    std::vector<float> verts, norms, texcs;
    verts.reserve(N * 3);
    norms.reserve(N * 3);
    texcs.reserve(N * 2);

    for (unsigned long i = 0; i < N; i++)
    {
        std::getline(file, line);
        float x, y, z, nx = 0, ny = 1, nz = 0, s = 0, t = 0;
        int cnt = sscanf(line.c_str(), "%f %f %f %f %f %f %f %f",
                         &x, &y, &z, &nx, &ny, &nz, &s, &t);
        (void)cnt;
        verts.push_back(x);
        verts.push_back(y);
        verts.push_back(z);
        norms.push_back(nx);
        norms.push_back(ny);
        norms.push_back(nz);
        texcs.push_back(s);
        texcs.push_back(t);
    }
    file.close();

    Model m;
    m.vertexCount = N;
    m.material = mat;

    // Vertex VBO
    glGenBuffers(1, &m.vertexVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m.vertexVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);

    // Normal VBO
    glGenBuffers(1, &m.normalVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m.normalVBO);
    glBufferData(GL_ARRAY_BUFFER, norms.size() * sizeof(float), norms.data(), GL_STATIC_DRAW);

    // Texcoord VBO
    glGenBuffers(1, &m.texcoordVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m.texcoordVBO);
    glBufferData(GL_ARRAY_BUFFER, texcs.size() * sizeof(float), texcs.data(), GL_STATIC_DRAW);

    // Texture
    if (texFile && texFile[0] != '\0')
        m.textureID = loadTexture(texFile);

    printf("Loaded '%s': %lu vertices\n", filename, N);
    return m;
}

// =========================================================================
// XML PARSING
// =========================================================================

static Material parseMaterial(XMLElement *modelEl)
{
    Material mat;
    XMLElement *colorEl = modelEl->FirstChildElement("color");
    if (!colorEl)
        return mat;

    auto readRGB = [&](const char *tag, float *out4)
    {
        XMLElement *e = colorEl->FirstChildElement(tag);
        if (!e)
            return;
        out4[0] = e->FloatAttribute("R", out4[0] * 255) / 255.0f;
        out4[1] = e->FloatAttribute("G", out4[1] * 255) / 255.0f;
        out4[2] = e->FloatAttribute("B", out4[2] * 255) / 255.0f;
        out4[3] = 1.0f;
    };
    readRGB("diffuse", mat.diffuse);
    readRGB("ambient", mat.ambient);
    readRGB("specular", mat.specular);
    readRGB("emissive", mat.emissive);

    XMLElement *sh = colorEl->FirstChildElement("shininess");
    if (sh)
        mat.shininess = sh->FloatAttribute("value", 0.0f);
    return mat;
}

Group parseGroup(XMLElement *groupEl)
{
    Group g;

    XMLElement *tr = groupEl->FirstChildElement("transform");
    if (tr)
    {
        XMLElement *op = tr->FirstChildElement();
        while (op)
        {
            std::string name = op->Name();
            Transform t;
            if (name == "translate")
            {
                t.type = TRANSLATE;
                if (op->Attribute("time"))
                {
                    t.time = op->FloatAttribute("time");
                    t.align = op->BoolAttribute("align");
                    XMLElement *pt = op->FirstChildElement("point");
                    while (pt)
                    {
                        t.curvePoints.push_back(
                            Point(pt->FloatAttribute("x"),
                                  pt->FloatAttribute("y"),
                                  pt->FloatAttribute("z")));
                        pt = pt->NextSiblingElement("point");
                    }
                }
                else
                {
                    t.x = op->FloatAttribute("x");
                    t.y = op->FloatAttribute("y");
                    t.z = op->FloatAttribute("z");
                }
            }
            else if (name == "rotate")
            {
                t.type = ROTATE;
                t.x = op->FloatAttribute("x");
                t.y = op->FloatAttribute("y");
                t.z = op->FloatAttribute("z");
                if (op->Attribute("time"))
                    t.time = op->FloatAttribute("time");
                else
                    t.angle = op->FloatAttribute("angle");
            }
            else if (name == "scale")
            {
                t.type = SCALE;
                t.x = op->FloatAttribute("x");
                t.y = op->FloatAttribute("y");
                t.z = op->FloatAttribute("z");
            }
            g.transforms.push_back(t);
            op = op->NextSiblingElement();
        }
    }

    XMLElement *modelsEl = groupEl->FirstChildElement("models");
    if (modelsEl)
    {
        XMLElement *modelEl = modelsEl->FirstChildElement("model");
        while (modelEl)
        {
            const char *file = modelEl->Attribute("file");
            if (file)
            {
                // Texture
                const char *texFile = "";
                XMLElement *texEl = modelEl->FirstChildElement("texture");
                if (texEl)
                    texFile = texEl->Attribute("file");

                // Material
                Material mat = parseMaterial(modelEl);

                g.models.push_back(vectorize(file, texFile, mat));
            }
            modelEl = modelEl->NextSiblingElement("model");
        }
    }

    XMLElement *child = groupEl->FirstChildElement("group");
    while (child)
    {
        g.children.push_back(parseGroup(child));
        child = child->NextSiblingElement("group");
    }
    return g;
}

bool loadScene(const char *filename)
{
    doc.LoadFile(filename);
    if (doc.ErrorID())
    {
        doc.LoadFile(std::string("../").append(filename).c_str());
        if (doc.ErrorID())
            return false;
    }

    XMLElement *world = doc.FirstChildElement("world");
    if (!world)
        return false;

    XMLElement *win = world->FirstChildElement("window");
    if (win)
    {
        win->QueryIntAttribute("width", &windowSettings.width);
        win->QueryIntAttribute("height", &windowSettings.height);
    }

    XMLElement *cam = world->FirstChildElement("camera");
    if (cam)
    {
        auto qf = [](XMLElement *e, const char *a, float *v)
        {if(e)e->QueryFloatAttribute(a,v); };
        XMLElement *pos = cam->FirstChildElement("position"),
                   *look = cam->FirstChildElement("lookAt"),
                   *up = cam->FirstChildElement("up"),
                   *proj = cam->FirstChildElement("projection");
        qf(pos, "x", &cameraSettings.posX);
        qf(pos, "y", &cameraSettings.posY);
        qf(pos, "z", &cameraSettings.posZ);
        qf(look, "x", &cameraSettings.lookX);
        qf(look, "y", &cameraSettings.lookY);
        qf(look, "z", &cameraSettings.lookZ);
        qf(up, "x", &cameraSettings.upX);
        qf(up, "y", &cameraSettings.upY);
        qf(up, "z", &cameraSettings.upZ);
        if (proj)
        {
            proj->QueryFloatAttribute("fov", &cameraSettings.fov);
            proj->QueryFloatAttribute("near", &cameraSettings.nearPlane);
            proj->QueryFloatAttribute("far", &cameraSettings.farPlane);
        }
        double r = sqrt(pow(cameraSettings.posX, 2) + pow(cameraSettings.posY, 2) + pow(cameraSettings.posZ, 2));
        camPos.radius = r;
        if (r > 0)
        {
            camPos.beta = asin(cameraSettings.posY / r);
            camPos.alpha = atan2(cameraSettings.posX, cameraSettings.posZ);
        }
    }

    // Parse lights
    XMLElement *lightsEl = world->FirstChildElement("lights");
    if (lightsEl)
    {
        XMLElement *lEl = lightsEl->FirstChildElement("light");
        while (lEl && sceneLights.size() < 8)
        {
            Light l;
            const char *type = lEl->Attribute("type");
            if (type)
            {
                if (strcmp(type, "directional") == 0)
                {
                    l.type = LIGHT_DIRECTIONAL;
                    l.dirX = lEl->FloatAttribute("dirX", 0);
                    l.dirY = lEl->FloatAttribute("dirY", -1);
                    l.dirZ = lEl->FloatAttribute("dirZ", 0);
                }
                else if (strcmp(type, "spot") == 0)
                {
                    l.type = LIGHT_SPOT;
                    l.posX = lEl->FloatAttribute("posX", 0);
                    l.posY = lEl->FloatAttribute("posY", 0);
                    l.posZ = lEl->FloatAttribute("posZ", 0);
                    l.dirX = lEl->FloatAttribute("dirX", 0);
                    l.dirY = lEl->FloatAttribute("dirY", -1);
                    l.dirZ = lEl->FloatAttribute("dirZ", 0);
                    l.cutoff = lEl->FloatAttribute("cutoff", 45);
                }
                else
                { // point
                    l.type = LIGHT_POINT;
                    l.posX = lEl->FloatAttribute("posX", 0);
                    l.posY = lEl->FloatAttribute("posY", 0);
                    l.posZ = lEl->FloatAttribute("posZ", 0);
                }
            }
            sceneLights.push_back(l);
            lEl = lEl->NextSiblingElement("light");
        }
    }

    XMLElement *rootGroup = world->FirstChildElement("group");
    if (rootGroup)
        sceneRoot = parseGroup(rootGroup);

    return true;
}

// =========================================================================
// DRAWING
// =========================================================================

void applyLights()
{
    for (int i = 0; i < (int)sceneLights.size() && i < 8; i++)
    {
        GLenum li = GL_LIGHT0 + i;
        glEnable(li);
        const Light &l = sceneLights[i];
        float white[4] = {1, 1, 1, 1}, blk[4] = {0, 0, 0, 1};
        glLightfv(li, GL_DIFFUSE, white);
        glLightfv(li, GL_SPECULAR, white);
        glLightfv(li, GL_AMBIENT, blk);

        if (l.type == LIGHT_DIRECTIONAL)
        {
            float pos[4] = {l.dirX, l.dirY, l.dirZ, 0};
            glLightfv(li, GL_POSITION, pos);
        }
        else if (l.type == LIGHT_SPOT)
        {
            float pos[4] = {l.posX, l.posY, l.posZ, 1};
            glLightfv(li, GL_POSITION, pos);
            float dir[3] = {l.dirX, l.dirY, l.dirZ};
            glLightfv(li, GL_SPOT_DIRECTION, dir);
            glLightf(li, GL_SPOT_CUTOFF, l.cutoff);
            glLightf(li, GL_SPOT_EXPONENT, 10.0f);
        }
        else
        {
            float pos[4] = {l.posX, l.posY, l.posZ, 1};
            glLightfv(li, GL_POSITION, pos);
        }
    }
}

void drawGroup(const Group &g, float t)
{
    glPushMatrix();

    for (const auto &tr : g.transforms)
    {
        if (tr.type == TRANSLATE)
        {
            if (tr.time > 0 && tr.curvePoints.size() >= 4)
            {
                float pos[3], deriv[3];
                float gt = fmod(t, tr.time) / tr.time;
                getGlobalCatmullRomPoint(gt, pos, deriv, tr.curvePoints);
                glTranslatef(pos[0], pos[1], pos[2]);
                if (tr.align)
                {
                    float x[3] = {deriv[0], deriv[1], deriv[2]}, y[3], z[3], m[16];
                    normalize3(x);
                    float up[3] = {0, 1, 0};
                    cross3(x, up, z);
                    normalize3(z);
                    cross3(z, x, y);
                    normalize3(y);
                    buildRotMatrix(x, y, z, m);
                    glMultMatrixf(m);
                }
            }
            else
            {
                glTranslatef(tr.x, tr.y, tr.z);
            }
        }
        else if (tr.type == ROTATE)
        {
            float angle = (tr.time > 0) ? (t * 360.0f / tr.time) : tr.angle;
            glRotatef(angle, tr.x, tr.y, tr.z);
        }
        else
        {
            glScalef(tr.x, tr.y, tr.z);
        }
    }

    for (const auto &m : g.models)
    {
        // Apply material
        glMaterialfv(GL_FRONT, GL_DIFFUSE, m.material.diffuse);
        glMaterialfv(GL_FRONT, GL_AMBIENT, m.material.ambient);
        glMaterialfv(GL_FRONT, GL_SPECULAR, m.material.specular);
        glMaterialfv(GL_FRONT, GL_EMISSION, m.material.emissive);
        glMaterialf(GL_FRONT, GL_SHININESS, m.material.shininess);

        // Texture
        if (m.textureID)
        {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, m.textureID);
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            glBindBuffer(GL_ARRAY_BUFFER, m.texcoordVBO);
            glTexCoordPointer(2, GL_FLOAT, 0, 0);
        }
        else
        {
            glDisable(GL_TEXTURE_2D);
        }

        // Normals & vertices
        glEnableClientState(GL_NORMAL_ARRAY);
        glBindBuffer(GL_ARRAY_BUFFER, m.normalVBO);
        glNormalPointer(GL_FLOAT, 0, 0);

        glEnableClientState(GL_VERTEX_ARRAY);
        glBindBuffer(GL_ARRAY_BUFFER, m.vertexVBO);
        glVertexPointer(3, GL_FLOAT, 0, 0);

        glDrawArrays(GL_TRIANGLES, 0, m.vertexCount);

        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_NORMAL_ARRAY);
        if (m.textureID)
            glDisable(GL_TEXTURE_2D);
    }

    for (const auto &child : g.children)
        drawGroup(child, t);

    glPopMatrix();
}

// =========================================================================
// RENDER LOOP
// =========================================================================

void renderScene()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    gluLookAt(polarX(camPos), polarY(camPos), polarZ(camPos),
              cameraSettings.lookX, cameraSettings.lookY, cameraSettings.lookZ,
              cameraSettings.upX, cameraSettings.upY, cameraSettings.upZ);

    applyLights();
    draw_axis();

    float timeInSeconds = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glEnableClientState(GL_VERTEX_ARRAY);

    drawGroup(sceneRoot, timeInSeconds);

    glDisableClientState(GL_VERTEX_ARRAY);

    glutSwapBuffers();
    glutPostRedisplay();
}

// =========================================================================
// INPUT
// =========================================================================

void keyboardFunc(unsigned char key, int x, int y)
{
    if (key == '+' && camPos.radius > 1)
        camPos.radius -= 1;
    else if (key == '-')
        camPos.radius += 1;
    glutPostRedisplay();
}

void specialKeysFunc(int key, int x, int y)
{
    switch (key)
    {
    case GLUT_KEY_LEFT:
        camPos.alpha -= M_PI / 16;
        break;
    case GLUT_KEY_RIGHT:
        camPos.alpha += M_PI / 16;
        break;
    case GLUT_KEY_DOWN:
        camPos.beta -= M_PI / 16;
        break;
    case GLUT_KEY_UP:
        camPos.beta += M_PI / 16;
        break;
    }
    if (camPos.beta < -M_PI_2 + 0.01)
        camPos.beta = -M_PI_2 + 0.01;
    if (camPos.beta > M_PI_2 - 0.01)
        camPos.beta = M_PI_2 - 0.01;
    glutPostRedisplay();
}

// =========================================================================
// MAIN
// =========================================================================

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("Uso: ./engine <scene.xml>\n");
        return 1;
    }

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowPosition(100, 100);
    glutInitWindowSize(800, 800);
    glutCreateWindow("CG26 - Fase 4: Lighting & Textures");

#ifndef __APPLE__
    glewExperimental = GL_TRUE;
    glewInit();
#endif

    if (!loadScene(argv[1]))
    {
        puts("Erro ao carregar XML!");
        return 1;
    }

    glutReshapeWindow(windowSettings.width, windowSettings.height);
    glutDisplayFunc(renderScene);
    glutIdleFunc(renderScene);
    glutKeyboardFunc(keyboardFunc);
    glutSpecialFunc(specialKeysFunc);
    glutReshapeFunc(changeSize);

    glEnable(GL_DEPTH_TEST);
    // glEnable(GL_CULL_FACE);
    glEnable(GL_LIGHTING);
    glEnable(GL_RESCALE_NORMAL);
    glEnable(GL_NORMALIZE);

    // Global ambient light (dim)
    float globalAmb[4] = {0.6f, 0.6f, 0.6f, 1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmb);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    glutMainLoop();
    return 0;
}
