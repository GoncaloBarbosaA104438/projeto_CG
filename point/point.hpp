#ifndef POINT_H
#define POINT_H

class Point
{

private:
    double x_coord;
    double y_coord;
    double z_coord;

public:
    Point(double x, double y, double z);

    double getX() const;
    double getY() const;
    double getZ() const;
};

#endif