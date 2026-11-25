// SPDX-License-Identifier: MIT
#pragma once
#include <QImage>
#include <QPainter>
#include <QStringList>
#include <QVector>
#include <QWidget>

#include "Calc.h"

// ---------------------------------------------------------------- Radar de la menace
struct RadarThresholds {
    double hp = 0.1, watch = 0.49, control = 1.0, danger = 3.0;
};

RadarThresholds thresholdsFrom(const Project &p);

// Dessin partagé entre le widget et l'export PNG / rapport.
void paintRadar(QPainter &p, const QRectF &area, const QVector<Calc::RadarItem> &items,
                const RadarThresholds &th, const QString &title);
QImage renderRadarImage(const QVector<Calc::RadarItem> &items, const RadarThresholds &th,
                        const QString &title, const QSize &size = QSize(1200, 900));

class RadarWidget : public QWidget
{
    Q_OBJECT
public:
    explicit RadarWidget(QWidget *parent = nullptr);
    void setData(const QVector<Calc::RadarItem> &items, const RadarThresholds &th, const QString &title);
    QImage toImage(const QSize &size = QSize(1200, 900)) const;
    QSize sizeHint() const override { return QSize(900, 680); }

protected:
    void paintEvent(QPaintEvent *) override;

private:
    QVector<Calc::RadarItem> items_;
    RadarThresholds th_;
    QString title_;
};

// ---------------------------------------------------------------- Matrice de risque 4x4
void paintRiskMatrix(QPainter &p, const QRectF &area, const QString &title,
                     const QStringList &levels /*16*/, const QStringList &texts /*16*/);

class RiskMatrixWidget : public QWidget
{
    Q_OBJECT
public:
    explicit RiskMatrixWidget(QWidget *parent = nullptr);
    void setData(const QString &title, const QStringList &levels, const QStringList &texts);
    QSize sizeHint() const override { return QSize(420, 380); }

protected:
    void paintEvent(QPaintEvent *) override;

private:
    QString title_;
    QStringList levels_, texts_;
};

// ---------------------------------------------------------------- Kill chain d'un scénario opérationnel
struct KillChainAction {
    QString phase, text;
    int likelihood = 0;
};

class KillChainWidget : public QWidget
{
    Q_OBJECT
public:
    explicit KillChainWidget(QWidget *parent = nullptr);
    void setData(const QString &title, int globalLikelihood, const QVector<KillChainAction> &actions);
    QSize sizeHint() const override { return QSize(900, 420); }

protected:
    void paintEvent(QPaintEvent *) override;

private:
    QString title_;
    int global_ = 0;
    QVector<KillChainAction> actions_;
};

// Rendus hors écran (utilisés par les exports PDF / DOCX)
QImage renderMatrixImage(const QString &title, const QStringList &levels, const QStringList &texts,
                         const QSize &size = QSize(640, 560));
QImage renderKillChainImage(const QString &title, int globalLikelihood, const QVector<KillChainAction> &actions,
                            int width = 1100);
