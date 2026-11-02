#pragma once

#include "model/Molecule.h"
#include <cmath>

namespace youngcraft {

struct CubeGrid {
    QString filePath;
    QString comment1;
    QString comment2;
    QVector3D origin;
    QVector3D axisX;
    QVector3D axisY;
    QVector3D axisZ;
    int nx = 0;
    int ny = 0;
    int nz = 0;
    QVector<float> values;
    Molecule molecule;
    float minValue = 0.0f;
    float maxValue = 0.0f;
    bool valid = false;
    QString error;

    // Trilinear interpolation in an arbitrary (including skew) voxel basis.
    float sample(const QVector3D &point) const {
        const float det = QVector3D::dotProduct(axisX, QVector3D::crossProduct(axisY, axisZ));
        if (!valid || std::abs(det) < 1e-15f) return float(kNaN);
        const auto p = point - origin;
        const float x = QVector3D::dotProduct(p, QVector3D::crossProduct(axisY,axisZ))/det;
        const float y = QVector3D::dotProduct(p, QVector3D::crossProduct(axisZ,axisX))/det;
        const float z = QVector3D::dotProduct(p, QVector3D::crossProduct(axisX,axisY))/det;
        if (x < -1e-4f || y < -1e-4f || z < -1e-4f || x > nx-1+1e-4f || y > ny-1+1e-4f || z > nz-1+1e-4f) return float(kNaN);
        const float cx=qBound(0.f,x,float(nx-1)), cy=qBound(0.f,y,float(ny-1)), cz=qBound(0.f,z,float(nz-1));
        const int ix=int(cx), iy=int(cy), iz=int(cz);
        const float dx=cx-ix,dy=cy-iy,dz=cz-iz; float sum=0;
        for(int a=0;a<2;++a)for(int b=0;b<2;++b)for(int c=0;c<2;++c)
            sum += (a?dx:1-dx)*(b?dy:1-dy)*(c?dz:1-dz)*value(qMin(ix+a,nx-1),qMin(iy+b,ny-1),qMin(iz+c,nz-1));
        return sum;
    }
    float value(int x, int y, int z) const {
        if (x < 0 || y < 0 || z < 0 || x >= nx || y >= ny || z >= nz) {
            return 0.0f;
        }
        return values[(z * ny + y) * nx + x];
    }
};

class CubeParser {
public:
    static CubeGrid parse(const QString &filePath);
};

} // namespace youngcraft
