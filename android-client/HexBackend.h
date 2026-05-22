#pragma once
#include <QObject>
#include <QDate>
#include <QVariantList>
#include <QTimer>
#include <vector>
#include "../solver-ffi/hex_solver.h"
#include "HexSolverWorker.h"

class HexBackend : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool         solving       READ solving       NOTIFY solvingChanged)
    Q_PROPERTY(int          solutionCount READ solutionCount NOTIFY boardChanged)
    Q_PROPERTY(int          currentIndex  READ currentIndex  NOTIFY boardChanged)
    Q_PROPERTY(QString      statusText    READ statusText    NOTIFY statusTextChanged)
    Q_PROPERTY(QString      solLabel      READ solLabel      NOTIFY boardChanged)
    Q_PROPERTY(QVariantList boardData     READ boardData     NOTIFY boardChanged)
    Q_PROPERTY(QVariantList boardLabels   READ boardLabels   NOTIFY boardChanged)
    Q_PROPERTY(int          variant       READ variant  WRITE setVariant  NOTIFY variantChanged)
    Q_PROPERTY(bool         findAll       READ findAll  WRITE setFindAll  NOTIFY findAllChanged)
    Q_PROPERTY(bool         allowFlip     READ allowFlip WRITE setAllowFlip NOTIFY allowFlipChanged)
    Q_PROPERTY(bool         slideshow     READ slideshow WRITE setSlideshow NOTIFY slideshowChanged)

public:
    explicit HexBackend(QObject* parent = nullptr);

    bool         solving()       const { return m_solving; }
    int          solutionCount() const { return (int)m_solutions.size(); }
    int          currentIndex()  const { return m_idx; }
    QString      statusText()    const { return m_statusText; }
    QString      solLabel()      const { return m_solLabel; }
    QVariantList boardData()     const { return m_boardData; }
    QVariantList boardLabels()   const { return m_boardLabels; }
    int          variant()       const { return m_variant; }
    bool         findAll()       const { return m_findAll; }
    bool         allowFlip()     const { return m_allowFlip; }
    bool         slideshow()     const { return m_slideshow; }

    void setVariant(int v);
    void setFindAll(bool v);
    void setAllowFlip(bool v);
    void setSlideshow(bool on);

    Q_INVOKABLE void solve(int year, int month, int day);
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void prevSolution();
    Q_INVOKABLE void nextSolution();

signals:
    void solvingChanged();
    void boardChanged();
    void statusTextChanged();
    void variantChanged();
    void findAllChanged();
    void allowFlipChanged();
    void slideshowChanged();

private slots:
    void onSolved();
    void doSolve();
    void onSlideshowTick();

private:
    void updateBoardData(const HexBoard* board, QDate date);
    void showSolution(int idx);
    void initLcg();
    int  nextLcgIdx();

    bool         m_solving     = false;
    int          m_idx         = 0;
    QString      m_statusText;
    QString      m_solLabel;
    QVariantList m_boardData;
    QVariantList m_boardLabels;

    int  m_variant   = 0;
    bool m_findAll   = false;
    bool m_allowFlip = true;
    bool m_slideshow = false;

    QDate m_displayDate;
    QDate m_pendingDate;
    bool  m_hasPending     = false;
    bool  m_pendingFindAll = false;

    quint32 m_lcgState = 0;
    quint32 m_lcgM     = 1;

    std::vector<HexBoard> m_solutions;
    QDate                 m_solvedDate;
    HexSolverWorker*      m_worker        = nullptr;
    QTimer*               m_slideshowTimer = nullptr;
};
