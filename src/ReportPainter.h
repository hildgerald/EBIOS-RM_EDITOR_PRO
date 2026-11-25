// SPDX-License-Identifier: MIT
#pragma once
#include <QImage>
#include <QPainter>
#include <QRectF>
#include <QString>
#include <QVector>

#include "ReportAssembler.h"

// Dessin « à la main » des pages du PDF qui ne sont pas du texte courant : page de garde, sommaire, pied de page.
// Toutes les dimensions sont en millimètres convertis par pxPerMm, donc indépendantes de la résolution du support
// (PDF à 1200 dpi, aperçu à l'écran, etc.).
namespace ReportPainter {

struct TocEntry {
    int level = 2;      // 2 = chapitre, 3 = section ; 0 = figure
    QString text;
    int page = 0;       // numéro de page affiché
};

void paintCover(QPainter &p, const QRectF &page, const ReportAssembler::CoverInfo &c, double pxPerMm);

// Nombre de lignes par colonne et par page de sommaire.
int tocCapacity(const QRectF &page, double pxPerMm);

// pageIndex : rang de la page de sommaire (0..pageCount-1)
void paintToc(QPainter &p, const QRectF &page, double pxPerMm, int pageIndex, int pageCount,
              const QVector<TocEntry> &toc, const QVector<TocEntry> &figures);

// Pied de page : titre à gauche, « Page n / total » à droite, en bas de page (hauteur footerMm).
void paintFooter(QPainter &p, const QRectF &page, double pxPerMm, double footerMm, const QString &left, int pageNo, int total);

// Aperçu de la page de garde (A4 paysage) pour la page de réglages.
QImage coverPreview(const ReportAssembler::CoverInfo &c, int widthPx = 840);

} // namespace ReportPainter
