#pragma once
#include <QIcon>
#include <QPixmap>
#include <QPainter>
#include <cmath>

// Draw the application icon: dark-navy hex background with 7 coloured
// mini-hexagons (matching the puzzle piece colours).
inline QIcon makeHexIcon()
{
    static const QColor MC[] = {
        {70,130,180},{255,160,50},{60,179,113},
        {220,80,80},{148,103,189},{64,200,200},{240,200,50}
    };
    // Fractional offsets of the 7 mini-hexes (6 around + 1 centre)
    static const double OX[] = { 0.0, 0.50, 0.50,  0.0,-0.50,-0.50, 0.0};
    static const double OY[] = {-0.5,-0.25, 0.25,  0.5, 0.25,-0.25, 0.0};

    auto drawPm = [&](int sz) {
        QPixmap pm(sz, sz);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing);

        const double cx = sz / 2.0;
        const double cy = sz / 2.0;
        const double R  = sz * 0.46;

        auto hexPoly = [](double cx, double cy, double r) {
            QPolygonF poly;
            for (int i = 0; i < 6; ++i) {
                double a = i * M_PI / 3.0;
                poly << QPointF(cx + r * std::sin(a), cy - r * std::cos(a));
            }
            return poly;
        };

        // Background hexagon
        QPolygonF bg = hexPoly(cx, cy, R);
        p.setBrush(QColor(30, 55, 120));
        p.setPen(Qt::NoPen);
        p.drawPolygon(bg);

        // Mini hexagons
        const double sr = R * 0.30;
        for (int k = 0; k < 7; ++k) {
            QPolygonF h = hexPoly(cx + OX[k]*R, cy + OY[k]*R, sr);
            p.setBrush(MC[k]);
            p.setPen(sz >= 32
                     ? QPen(QColor(20, 20, 20), sz * 0.018)
                     : Qt::NoPen);
            p.drawPolygon(h);
        }

        // Outer border
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(QColor(15, 35, 90), sz * 0.05));
        p.drawPolygon(bg);

        return pm;
    };

    QIcon icon;
    for (int sz : {16, 24, 32, 48, 64, 128, 256})
        icon.addPixmap(drawPm(sz));
    return icon;
}
