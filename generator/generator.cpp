#include <cstring>
#include <stdlib.h>
#include <iostream>

#include "plane.hpp"
#include "box.hpp"
#include "sphere.hpp"
#include "cone.hpp"
#include "bezier.hpp"
#include "ring.hpp" // Adicionado o Anel!

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cout << "Erro: Faltam argumentos! Uso: ./generator <forma> [parametros] <ficheiro.3d>\n";
        return 1;
    }

    if (strcmp(argv[1], "plane") == 0) {
        plane(argv[4], atof(argv[2]), atoi(argv[3]));
    }
    else if (strcmp(argv[1], "box") == 0) {
        box(argv[4], atof(argv[2]), atoi(argv[3]));
    }
    else if (strcmp(argv[1], "sphere") == 0) {
        sphere(argv[5], atof(argv[2]), atoi(argv[3]), atoi(argv[4]));
    }
    else if (strcmp(argv[1], "cone") == 0) {
        cone(argv[6], atof(argv[2]), atof(argv[3]), atoi(argv[4]), atoi(argv[5]));
    }
    else if (strcmp(argv[1], "ring") == 0) {
        // Uso: ./generator ring <inDiam> <outDiam> <slices> <outputFile>
        ring(argv[5], atof(argv[2]), atof(argv[3]), atoi(argv[4]));
    }
    else if (strcmp(argv[1], "patch") == 0) {
        bezier(argv[2], argv[4], atoi(argv[3]));
    }
    else {
        std::cout << "Erro: Forma '" << argv[1] << "' nao reconhecida.\n";
    }

    return 0;
}