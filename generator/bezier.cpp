#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <string>

struct Point3D {
    float x, y, z;
    Point3D(float x=0,float y=0,float z=0):x(x),y(y),z(z){}
};

const float BM[4][4] = {
    {-1, 3,-3, 1},
    { 3,-6, 3, 0},
    {-3, 3, 0, 0},
    { 1, 0, 0, 0}
};

// T(t) * BezierMatrix → result[4]
static void tmb(float t, float r[4]) {
    float t3=t*t*t, t2=t*t;
    for (int i=0;i<4;i++)
        r[i] = t3*BM[0][i] + t2*BM[1][i] + t*BM[2][i] + BM[3][i];
}

// dT/dt * BezierMatrix → result[4]
static void dtmb(float t, float r[4]) {
    float t2=t*t;
    for (int i=0;i<4;i++)
        r[i] = 3*t2*BM[0][i] + 2*t*BM[1][i] + BM[2][i];
}

static Point3D evalSurface(const std::vector<std::vector<Point3D>>& cg, float u, float v) {
    float U[4], V[4];
    tmb(u, U); tmb(v, V);
    Point3D p;
    for (int i=0;i<4;i++) for (int j=0;j<4;j++) {
        float c = U[i]*V[j];
        p.x += cg[i][j].x*c; p.y += cg[i][j].y*c; p.z += cg[i][j].z*c;
    }
    return p;
}

static Point3D dSurface_du(const std::vector<std::vector<Point3D>>& cg, float u, float v) {
    float dU[4], V[4];
    dtmb(u, dU); tmb(v, V);
    Point3D p;
    for (int i=0;i<4;i++) for (int j=0;j<4;j++) {
        float c = dU[i]*V[j];
        p.x += cg[i][j].x*c; p.y += cg[i][j].y*c; p.z += cg[i][j].z*c;
    }
    return p;
}

static Point3D dSurface_dv(const std::vector<std::vector<Point3D>>& cg, float u, float v) {
    float U[4], dV[4];
    tmb(u, U); dtmb(v, dV);
    Point3D p;
    for (int i=0;i<4;i++) for (int j=0;j<4;j++) {
        float c = U[i]*dV[j];
        p.x += cg[i][j].x*c; p.y += cg[i][j].y*c; p.z += cg[i][j].z*c;
    }
    return p;
}

static Point3D cross(const Point3D& a, const Point3D& b) {
    return {a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x};
}

static Point3D normalize(const Point3D& a) {
    float l = sqrt(a.x*a.x+a.y*a.y+a.z*a.z);
    if (l < 1e-6f) return {0,1,0};
    return {a.x/l, a.y/l, a.z/l};
}

struct BVertex { Point3D pos, norm; float s, t; };

static BVertex makeBV(const std::vector<std::vector<Point3D>>& cg, float u, float v) {
    BVertex bv;
    bv.pos  = evalSurface(cg, u, v);
    Point3D du = dSurface_du(cg, u, v);
    Point3D dv = dSurface_dv(cg, u, v);
    bv.norm = normalize(cross(du, dv));
    bv.s = u; bv.t = v;
    return bv;
}

static void writeBV(std::ofstream& f, const BVertex& bv) {
    f << bv.pos.x  << " " << bv.pos.y  << " " << bv.pos.z  << " "
      << bv.norm.x << " " << bv.norm.y << " " << bv.norm.z << " "
      << bv.s      << " " << bv.t      << "\n";
}

void bezier(const char* patchFile, const char* outputFile, int tessellationLevel)
{
    std::ifstream file(patchFile);
    if (!file.is_open()) { std::cerr << "Erro ao abrir patch: " << patchFile << "\n"; return; }

    int numPatches = 0;
    if (!(file >> numPatches)) return;

    std::vector<std::vector<int>> patchIdx(numPatches, std::vector<int>(16));
    for (int i = 0; i < numPatches; i++)
        for (int j = 0; j < 16; j++) {
            file >> patchIdx[i][j];
            if (j < 15) file.ignore(1, ',');
        }

    std::vector<Point3D> cp;
    float cx, cy, cz;
    std::string tmp;
    file >> tmp;
    if (tmp.find(',') == std::string::npos) file >> cx;
    else cx = std::stof(tmp);
    file.ignore(1,','); file >> cy;
    file.ignore(1,','); file >> cz;
    cp.push_back({cx,cy,cz});
    while (file >> cx) {
        file.ignore(1,','); file >> cy;
        file.ignore(1,','); file >> cz;
        cp.push_back({cx,cy,cz});
    }
    file.close();

    float step = 1.0f / tessellationLevel;
    int totalVerts = numPatches * tessellationLevel * tessellationLevel * 6;

    std::ofstream out(outputFile);
    if (!out.is_open()) { std::cerr << "Erro ao criar: " << outputFile << "\n"; return; }
    out << totalVerts << "\n";

    for (int p = 0; p < numPatches; p++) {
        std::vector<std::vector<Point3D>> cg(4, std::vector<Point3D>(4));
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                cg[i][j] = cp[patchIdx[p][i*4+j]];

        for (int i = 0; i < tessellationLevel; i++) {
            for (int j = 0; j < tessellationLevel; j++) {
                float u0=i*step, u1=u0+step, v0=j*step, v1=v0+step;
                BVertex p00=makeBV(cg,u0,v0), p01=makeBV(cg,u0,v1);
                BVertex p10=makeBV(cg,u1,v0), p11=makeBV(cg,u1,v1);
                writeBV(out, p00); writeBV(out, p10); writeBV(out, p01);
                writeBV(out, p01); writeBV(out, p10); writeBV(out, p11);
            }
        }
    }

    out.close();
    std::cout << "Bezier gerado: " << outputFile << " (" << totalVerts << " vertices)\n";
}
