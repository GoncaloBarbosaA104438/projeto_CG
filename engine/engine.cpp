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

XMLDocument doc;

std::vector<std::vector<Point>> solids;

struct Polar
{
    double radius;
    double alpha;
    double beta;
};

Polar camPos = {sqrt(75), M_PI_4, M_PI_4};

double polarX(Polar polar) { return polar.radius * cos(polar.beta) * sin(polar.alpha); }
double polarY(Polar polar) { return polar.radius * sin(polar.beta); }
double polarZ(Polar polar) { return polar.radius * cos(polar.beta) * cos(polar.alpha); }

void changeSize(int width, int height)
{
    if (height == 0)
        height = 1;

    float ratio = width * 1.0 / height;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0, 0, width, height);
    gluPerspective(45.0f, ratio, 1.0f, 1000.0f);
    glMatrixMode(GL_MODELVIEW);
}

void draw_axis()
{
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

std::vector<Point> vectorize(const char *filename)
{
    std::ifstream file(filename);
    if (!file.good())
    {
        file.open(std::string("../").append(filename).c_str());
        if (!file.good())
        {
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
    for (unsigned long i = 0; i < N; i++)
    {
        std::getline(file, line);
        double a, b, c;
        int matches = sscanf(line.c_str(), "%lf %lf %lf", &a, &b, &c);
        if (matches != 3)
        {
            printf("ERROR - invalid number of points in vertex %s\n", line.c_str());
            exit(1);
        }
        solid.push_back(Point(a, b, c));
    }

    file.close();
    printf("Finished reading %s\n", filename);
    return solid;
}

void renderScene()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();
    gluLookAt(polarX(camPos), polarY(camPos), polarZ(camPos),
              0.0, 0.0, 0.0,
              0.0f, camPos.beta > M_PI_2 ? -1.0f : 1.0f, 0.0f);

    draw_axis();

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    puts("Rendering...");
    glColor3f(1.0f, 1.0f, 1.0f);

    glBegin(GL_TRIANGLES);
    for (const auto &solid : solids)
    {
        for (const auto &point : solid)
        {
            glVertex3f(point.getX(), point.getY(), point.getZ());
        }
    }
    glEnd();

    puts("Render complete.");
    glutSwapBuffers();
}

void keyboardFunc(unsigned char key, int x, int y)
{
    switch (key)
    {
    case '+':
        if (camPos.radius > 1)
            camPos.radius -= 1;
        break;
    case '-':
        camPos.radius += 1;
        break;
    }
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

    if (camPos.alpha < 0)
        camPos.alpha += M_PI * 2;
    else if (camPos.alpha > M_PI * 2)
        camPos.alpha -= M_PI * 2;

    if (camPos.beta < -M_PI_2)
        camPos.beta += M_PI * 2;
    else if (camPos.beta > (3 * M_PI_2))
        camPos.beta -= M_PI * 2;

    glutPostRedisplay();
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

    XMLElement *model = world->FirstChildElement("group")->FirstChildElement("models")->FirstChildElement("model");
    while (model)
    {
        if (!strcmp(model->Name(), "model"))
            solids.push_back(vectorize(model->Attribute("file")));
        model = model->NextSiblingElement();
    }
    return true;
}

int main(int argc, char **argv)
{
    if (argc != 2)
        return 1;

    if (!loadScene(argv[1]))
    {
        puts("Erro ao carregar a cena XML!");
        return 1;
    }

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowPosition(100, 100);
    glutInitWindowSize(800, 800);
    glutCreateWindow("CG26");

    glutDisplayFunc(renderScene);
    glutKeyboardFunc(keyboardFunc);
    glutSpecialFunc(specialKeysFunc);
    glutReshapeFunc(changeSize);

    glEnable(GL_DEPTH_TEST);
    // glEnable(GL_CULL_FACE);

    glutMainLoop();

    return 0;
}