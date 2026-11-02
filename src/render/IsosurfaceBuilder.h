#pragma once

#include "io/CubeParser.h"

namespace youngcraft {

struct SurfaceMesh {
    QVector<QVector3D> positions;
    QVector<QVector3D> normals;
    bool truncated = false;

    bool isEmpty() const { return positions.isEmpty(); }
    int vertexCount() const { return positions.size(); }
};

class IsosurfaceBuilder {
public:
    // A compact marching-tetrahedra implementation keeps the viewer free of
    // a general-purpose volume-rendering dependency.  Positive and negative
    // lobes are built separately so they can be styled independently.
    static SurfaceMesh build(const CubeGrid &grid, float isovalue, bool positive,
                             int maxTriangles = 250000);
};

} // namespace youngcraft
