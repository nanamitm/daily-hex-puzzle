#pragma once
#include <QWidget>
#include <QDate>
#include <QPolygonF>
#include <cstdint>

constexpr int HEX_GRID = 9;

class HexBoardWidget : public QWidget {
    Q_OBJECT
public:
    explicit HexBoardWidget(QWidget* parent = nullptr);

    void setVariant(int variant);          // 0=43v1, 1=43v2, 2=61-cell
    void setDate(QDate date, int weekday); // weekday 0=Sun..6=Sat
    void setBoard(const uint8_t cells[HEX_GRID][HEX_GRID]);
    void clearBoard();
    void setShowInnerLines(bool show);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    // pixels per hex step (same role as STEP in solverlib.js)
    static constexpr double STEP   = 52.0;
    static constexpr double RADIUS = STEP * 0.56; // hex circumradius

    // board maps for each variant (row, col) → label string
    // "" = off-board, "NON" = permanent wall, else = label text
    static const char* MAP43V1[HEX_GRID][HEX_GRID];
    static const char* MAP43V2[HEX_GRID][HEX_GRID];
    static const char* MAP61  [HEX_GRID][HEX_GRID];

    int    m_variant        = 0;
    QDate  m_date;
    int    m_weekday        = 0;
    bool   m_hasCells       = false;
    bool   m_showInnerLines = true;
    uint8_t m_cells[HEX_GRID][HEX_GRID]{};

    const char* mapAt(int r, int c) const;

    QPointF  hexCenter(int row, int col) const;
    QPolygonF hexPoly (int row, int col) const;

    // returns: -2=off-board, -1=date marker, 0=empty, 1-11=piece ID, 20=NON wall
    int     cellGroup (int r, int c) const;
    QString cellLabel (int r, int c) const;
};
