#include "render/IsosurfaceBuilder.h"

#include <QVector>

#include <algorithm>
#include <cmath>

namespace youngcraft {

namespace {

struct Sample {
    QVector3D position;
    float value = 0.0f;
    QVector3D normal;
};

Sample interpolate(const Sample &a, const Sample &b, float level) {
    const float denominator = b.value - a.value;
    const float factor = std::abs(denominator)<1e-12f ? .5f : std::clamp((level-a.value)/denominator,0.f,1.f);
    return {a.position+(b.position-a.position)*factor,level,(a.normal+(b.normal-a.normal)*factor).normalized()};
}
void appendTriangle(SurfaceMesh &mesh, Sample a, Sample b, Sample c) {
    QVector3D normal=QVector3D::crossProduct(b.position-a.position,c.position-a.position);
    if(normal.lengthSquared()<1e-12f)return;
    if(QVector3D::dotProduct(normal,a.normal+b.normal+c.normal)<0)std::swap(b,c);
    mesh.positions << a.position << b.position << c.position;
    mesh.normals << a.normal << b.normal << c.normal;
}

} // namespace

SurfaceMesh IsosurfaceBuilder::build(const CubeGrid &grid, float isovalue, bool positive,
                                     int maxTriangles) {
    SurfaceMesh mesh;
    if (!grid.valid || grid.nx < 2 || grid.ny < 2 || grid.nz < 2 || maxTriangles <= 0) {
        return mesh;
    }

    const float level = std::max(1.0e-7f, std::abs(isovalue));
    const qint64 totalCells = qint64(grid.nx - 1) * (grid.ny - 1) * (grid.nz - 1);
    const qint64 maximumCells = 2500000;
    int stride = 1;
    while ((qint64((grid.nx - 1 + stride - 1) / stride) *
            ((grid.ny - 1 + stride - 1) / stride) *
            ((grid.nz - 1 + stride - 1) / stride)) > maximumCells) {
        ++stride;
    }
    mesh.positions.reserve(std::min<qint64>(qint64(maxTriangles) * 3, totalCells * 3));
    mesh.normals.reserve(mesh.positions.capacity());

    // The six tetrahedra share the long diagonal (0, 6) of each voxel.
    constexpr int tetrahedra[6][4] = {
        {0,5,1,6}, {0,1,2,6}, {0,2,3,6},
        {0,3,7,6}, {0,7,4,6}, {0,4,5,6}};
    constexpr int edges[6][2] = {
        {0, 1}, {1, 2}, {2, 0}, {0, 3}, {1, 3}, {2, 3}};

    const auto crossX=QVector3D::crossProduct(grid.axisY,grid.axisZ);
    const auto crossY=QVector3D::crossProduct(grid.axisZ,grid.axisX);
    const auto crossZ=QVector3D::crossProduct(grid.axisX,grid.axisY);
    const float det=QVector3D::dotProduct(grid.axisX,crossX);
    if(std::abs(det)<1e-15f)return mesh;
    auto normalAt=[&](int x,int y,int z) {
        const int xm=qMax(0,x-1),xp=qMin(grid.nx-1,x+1),ym=qMax(0,y-1),yp=qMin(grid.ny-1,y+1),zm=qMax(0,z-1),zp=qMin(grid.nz-1,z+1);
        const float dx=(grid.value(xp,y,z)-grid.value(xm,y,z))/float(xp-xm);
        const float dy=(grid.value(x,yp,z)-grid.value(x,ym,z))/float(yp-ym);
        const float dz=(grid.value(x,y,zp)-grid.value(x,y,zm))/float(zp-zm);
        return ((crossX*dx+crossY*dy+crossZ*dz)*((positive?-1.f:1.f)/det)).normalized();
    };
    for (int z = 0; z < grid.nz - 1; z += stride) {
        const int z1 = std::min(z + stride, grid.nz - 1);
        for (int y = 0; y < grid.ny - 1; y += stride) {
            const int y1 = std::min(y + stride, grid.ny - 1);
            for (int x = 0; x < grid.nx - 1; x += stride) {
                const int x1 = std::min(x + stride, grid.nx - 1);
                const int ix[8] = {x, x1, x1, x, x, x1, x1, x};
                const int iy[8] = {y, y, y1, y1, y, y, y1, y1};
                const int iz[8] = {z, z, z, z, z1, z1, z1, z1};
                Sample corners[8];
                for (int i = 0; i < 8; ++i) {
                    corners[i].position = grid.origin + grid.axisX * float(ix[i]) +
                                          grid.axisY * float(iy[i]) + grid.axisZ * float(iz[i]);
                    const float value = grid.value(ix[i], iy[i], iz[i]);
                    corners[i].value = positive ? value : -value;
                    corners[i].normal = normalAt(ix[i],iy[i],iz[i]);
                }

                for (const auto &tetra : tetrahedra) {
                    Sample points[4] = {corners[tetra[0]], corners[tetra[1]],
                                        corners[tetra[2]], corners[tetra[3]]};
                    QVector<Sample> intersections;
                    intersections.reserve(4);
                    for (const auto &edge : edges) {
                        const Sample &a = points[edge[0]];
                        const Sample &b = points[edge[1]];
                        if ((a.value >= level) != (b.value >= level)) {
                            intersections.push_back(interpolate(a, b, level));
                        }
                    }
                    if (intersections.size() == 3) {
                        appendTriangle(mesh, intersections[0], intersections[1], intersections[2]);
                    } else if (intersections.size() == 4) {
                        // Edge enumeration is not polygon order: sort the planar quad
                        // before triangulating, otherwise a bow-tie leaves holes.
                        QVector3D center;for(const auto &point:intersections)center+=point.position;center*=.25f;
                        const auto u=(intersections[0].position-center).normalized();
                        auto n=QVector3D::crossProduct(intersections[1].position-intersections[0].position,intersections[2].position-intersections[0].position).normalized();
                        const auto v=QVector3D::crossProduct(n,u);
                        std::sort(intersections.begin(),intersections.end(),[&](const Sample &a,const Sample &b){const auto da=a.position-center,db=b.position-center;return std::atan2(QVector3D::dotProduct(da,v),QVector3D::dotProduct(da,u))<std::atan2(QVector3D::dotProduct(db,v),QVector3D::dotProduct(db,u));});
                        appendTriangle(mesh, intersections[0], intersections[1], intersections[2]);
                        appendTriangle(mesh, intersections[0], intersections[2], intersections[3]);
                    }
                    if (mesh.positions.size() >= maxTriangles * 3) {
                        mesh.truncated = true;
                        return mesh;
                    }
                }
            }
        }
    }
    return mesh;
}

} // namespace youngcraft
