#include <cmath>
#include <sstream>
#include "point.hpp"

Point::Point(double x, double y, double z)
{
    x_coord = x;
    y_coord = y;
    z_coord = z;
}

double Point::getX() const { return x_coord; }
double Point::getY() const { return y_coord; }
double Point::getZ() const { return z_coord; }