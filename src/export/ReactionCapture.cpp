#include "export/ReactionCapture.h"
#include "export/ImageExporter.h"
#include <QPainter>
#include <QQuaternion>
#include <QtMath>
#include <cmath>

namespace youngcraft {
QStringList ReactionCapture::companionPaths(const QString &path) {
    const QString suffix = QStringLiteral("-freq-ts-freq.out");
    if (!path.endsWith(suffix, Qt::CaseInsensitive)) return {};
    const auto stem = path.left(path.size() - 4);
    return {stem + QStringLiteral("-ircfwd-opt-freq.out"), stem + QStringLiteral("-ircrev-opt-freq.out")};
}

bool ReactionCapture::alignTo(Molecule &moving, const Molecule &target) {
    if (moving.isEmpty() || moving.atoms.size() != target.atoms.size()) return false;
    QVector3D a, b;
    int count = 0;
    for (int i=0; i<moving.atoms.size(); ++i) {
        if (moving.atoms[i].atomicNumber != target.atoms[i].atomicNumber) return false;
        if (target.atoms[i].atomicNumber == 1) continue;
        a += moving.atoms[i].position; b += target.atoms[i].position; ++count;
    }
    const bool all = count < 3;
    if (all) {
        a={};b={};count=0;
        for (int i=0; i<moving.atoms.size(); ++i) {
            a += moving.atoms[i].position; b += target.atoms[i].position; ++count;
        }
    }
    a /= float(count); b /= float(count);
    double s[3][3] = {};
    for (int i=0; i<moving.atoms.size(); ++i) {
        if (!all && target.atoms[i].atomicNumber == 1) continue;
        auto x = moving.atoms[i].position-a, y = target.atoms[i].position-b;
        for (int j=0;j<3;++j) for(int k=0;k<3;++k) s[j][k] += double(x[j])*y[k];
    }
    const auto &sxx=s[0][0], &sxy=s[0][1], &sxz=s[0][2], &syx=s[1][0], &syy=s[1][1], &syz=s[1][2], &szx=s[2][0], &szy=s[2][1], &szz=s[2][2];
    double n[4][4] = {{sxx+syy+szz, syz-szy, szx-sxz, sxy-syx},
        {syz-szy,sxx-syy-szz,sxy+syx,szx+sxz},
        {szx-sxz,sxy+syx,-sxx+syy-szz,syz+szy},
        {sxy-syx,szx+sxz,syz+szy,-sxx-syy+szz}};
    double v[4][4] = {}; for(int i=0;i<4;++i) v[i][i]=1;
    for(int iter=0;iter<64;++iter) {
        int p=0,q=1;
        for(int i=0;i<4;++i) for(int j=i+1;j<4;++j) if(std::abs(n[i][j])>std::abs(n[p][q])) {p=i;q=j;}
        if(std::abs(n[p][q])<1e-12) break;
        const double angle=0.5*std::atan2(2*n[p][q],n[q][q]-n[p][p]);
        const double c=std::cos(angle), t=std::sin(angle);
        const double pp=n[p][p],qq=n[q][q],pq=n[p][q];
        for(int k=0;k<4;++k) if(k!=p && k!=q) {
            const double kp=n[k][p],kq=n[k][q];
            n[k][p]=n[p][k]=c*kp-t*kq; n[k][q]=n[q][k]=t*kp+c*kq;
        }
        n[p][p]=c*c*pp-2*c*t*pq+t*t*qq; n[q][q]=t*t*pp+2*c*t*pq+c*c*qq;
        n[p][q]=n[q][p]=0;
        for(int k=0;k<4;++k) {const double kp=v[k][p],kq=v[k][q];v[k][p]=c*kp-t*kq;v[k][q]=t*kp+c*kq;}
    }
    int best=0;for(int i=1;i<4;++i) if(n[i][i]>n[best][best]) best=i;
    QQuaternion rotation(float(v[0][best]),float(v[1][best]),float(v[2][best]),float(v[3][best]));
    rotation.normalize();
    for(int i=0;i<moving.atoms.size();++i) {
        moving.atoms[i].position=b+rotation.rotatedVector(moving.atoms[i].position-a);
        moving.atoms[i].visible=target.atoms[i].visible;
        moving.atoms[i].colorOverride=target.atoms[i].colorOverride;
        moving.atoms[i].radiusScale=target.atoms[i].radiusScale;
    }
    return true;
}

QVector<QImage> ReactionCapture::equalPanels(const QImage &left, const QVector<QImage> &frames,
                                            const QImage &right, int width, double cm) {
    if(left.isNull() || right.isNull() || frames.isEmpty() || width<1 || cm<=0) return {};
    const auto animation=ImageExporter::cropAnimation(frames);
    const QImage l=ImageExporter::cropToContent(left),r=ImageExporter::cropToContent(right);
    // The TS defines the common canvas so its tight animation crop is retained.
    // Fit endpoints inside that canvas without stretching their geometry.
    const double aspect=double(animation.first().height())/animation.first().width();
    const QSize size(width,qMax(1,qCeil(width*aspect)));
    auto panel=[&](const QImage &im) {
        QImage out(size,QImage::Format_ARGB32);out.fill(Qt::transparent);
        const auto scaled=im.scaled(size,Qt::KeepAspectRatio,Qt::SmoothTransformation);
        QPainter p(&out);p.drawImage((size.width()-scaled.width())/2,(size.height()-scaled.height())/2,scaled);p.end();
        out.setDotsPerMeterX(qRound(width*100.0/cm));out.setDotsPerMeterY(out.dotsPerMeterX());return out;
    };
    QVector<QImage> result{panel(l)};for(const auto &frame:animation) result.push_back(panel(frame));result.push_back(panel(r));return result;
}
}
