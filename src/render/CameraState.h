#pragma once

#include <QQuaternion>
#include <QVector3D>

namespace youngcraft {

// Shared in physical units so differently sized molecules keep the same scale.
struct CameraState {
    QVector3D centerOffset;
    QQuaternion rotation;
    float distance = 8.0f;
    float orthoScale = 5.0f;
    bool perspective = false;
    float perspectiveFov = 45.0f;
};

} // namespace youngcraft
