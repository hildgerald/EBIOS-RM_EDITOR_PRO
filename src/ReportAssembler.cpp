// SPDX-License-Identifier: MIT
#include "ReportAssembler.h"

#include "Calc.h"
#include "ReportGenerator.h"

#include <QDir>
#include <QFile>
#include <QJsonObject>
#include <QUrl>

namespace ReportAssembler {

ReportOptions optionsFrom(const Project &p)
{
    const QJsonObject o = p.section("rapport");
    ReportOptions r;
    r.cover = o.value("garde").toBool(true);
    r.toc = o.value("sommaire").toBool(true);
    r.figures = o.value("figures").toBool(true);
    r.scenarioFigures = o.value("figures_so").toBool(true);
    return r;
}

CoverInfo coverFrom(const Project &p)
{
    const QJsonObject o = p.section("rapport");
    const QJsonObject somm = p.section("sommaire");
    CoverInfo c;
    const QByteArray b64 = Calc::s(o, "logo").toLatin1();
    if (!b64.isEmpty()) c.logo.loadFromData(QByteArray::fromBase64(b64));
    c.logoWidthMm = o.contains("logo_largeur") ? o.value("logo_largeur").toDouble(45.0) : 45.0;
    const QString al = Calc::s(o, "logo_align");
    c.logoAlign = al == "centre" ? 1 : (al == "droite" ? 2 : 0);
    c.title = Calc::s(somm, "titre").isEmpty() ? QStringLiteral("Analyse de risques EBIOS RM") : Calc::s(somm, "titre");
    c.subtitle = Calc::s(o, "sous_titre");
    c.organisation = Calc::s(o, "organisation");
    c.classification = Calc::s(o, "classification");
    c.version = Calc::s(somm, "version");
    c.date = Calc::s(somm, "date");
    c.author = Calc::s(somm, "auteur");
    c.owner = Calc::s(somm, "responsable");
    return c;
}

namespace {

Md::Block heading(int level, const QString &text)
{
    Md::Block b;
    b.type = Md::Block::Heading;
    b.level = level;
    Md::Run r;
    r.text = text;
    b.runs.append(r);
    return b;
}

Md::Block paragraph(const QString &text)
{
    Md::Block b;
    b.type = Md::Block::Paragraph;
    Md::Run r;
    r.text = text;
    r.italic = true;
    b.runs.append(r);
    return b;
}

struct Item { QString file, title; };

// Ajoute une planche (1 ou 2 figures) si au moins une image existe.
void addRow(Md::Document &d, const QString &assets, const QString &rowTitle, const QVector<Item> &items)
{
    Md::Block b;
    b.type = Md::Block::FigureRow;
    b.rowTitle = rowTitle;
    for (const Item &it : items) {
        const QString path = QDir(assets).filePath(it.file);
        if (!QFile::exists(path)) continue;
        Md::Figure f;
        f.src = QUrl::fromLocalFile(path).toString(QUrl::FullyEncoded);
        f.title = it.title;
        b.figures.append(f);
    }
    if (!b.figures.isEmpty()) d.blocks.append(b);
}

} // namespace

bool prepare(const Project &p, const QString &workDir, Assembled *out, QString *error)
{
    Assembled a;
    a.opt = optionsFrom(p);
    a.cover = coverFrom(p);
    a.title = a.cover.title;

    const QString assets = QDir(workDir).filePath("a");
    if (!ReportGenerator::writeAssets(p, assets, QSize(1200, 900), a.opt.figures, a.opt.scenarioFigures)) {
        if (error) *error = QObject::tr("Impossible de générer les images du rapport.");
        return false;
    }

    const QString md = ReportGenerator::generate(p, ReportGenerator::dirUrl(assets), ReportGenerator::Mode::Rich, !a.opt.figures);
    Md::Document body = Md::parse(md);

    // La page de garde remplace le titre et le tableau d'identification du corps.
    if (a.opt.cover && !body.blocks.isEmpty() && body.blocks.first().type == Md::Block::Heading && body.blocks.first().level == 1) {
        body.blocks.removeFirst();
        if (!body.blocks.isEmpty() && body.blocks.first().type == Md::Block::Table) body.blocks.removeFirst();
    }

    Md::Document d;
    if (a.opt.figures) {
        d.blocks.append(heading(2, QStringLiteral("Figures")));
        d.blocks.append(paragraph(QStringLiteral("Cette partie rassemble les figures principales de l'analyse : une planche par page.")));
        addRow(d, assets, QStringLiteral("Cartographie de la menace"),
               {{"radar_actuel.png", QStringLiteral("Radar de la menace – situation actuelle")},
                {"radar_projection.png", QStringLiteral("Radar de la menace – après mesures complémentaires")}});
        addRow(d, assets, QStringLiteral("Matrices de risque"),
               {{"matrice_initiale.png", QStringLiteral("Matrice des risques initiaux")},
                {"matrice_residuelle.png", QStringLiteral("Matrice des risques résiduels")}});
        addRow(d, assets, QStringLiteral("Architecture du système"),
               {{"architecture.png", QStringLiteral("Architecture du système étudié")}});
    }
    d.blocks += body.blocks;

    if (a.opt.scenarioFigures) {
        Md::Document annex;
        for (const QJsonValue &sv : p.table("so")) {
            const QJsonObject so = sv.toObject();
            const QString id = Calc::s(so, "id");
            addRow(annex, assets, id + QStringLiteral(" – ") + Calc::s(so, "titre").simplified(),
                   {{"so_" + id + ".png", id + QStringLiteral(" : ") + Calc::s(so, "titre").simplified()}});
        }
        if (!annex.blocks.isEmpty()) {
            d.blocks.append(heading(2, QStringLiteral("Annexe – Schémas des scénarios opérationnels")));
            d.blocks.append(paragraph(QStringLiteral("Chaque scénario est décomposé en actions élémentaires ; le chiffre de chaque case est sa vraisemblance (1 à 4).")));
            d.blocks += annex.blocks;
        }
    }

    // Numérotation des figures dans l'ordre d'apparition
    int n = 0;
    for (Md::Block &b : d.blocks)
        if (b.type == Md::Block::FigureRow)
            for (Md::Figure &f : b.figures) f.number = ++n;

    a.doc = d;
    *out = a;
    return true;
}

QVector<Md::Figure> collectFigures(const Md::Document &doc)
{
    QVector<Md::Figure> out;
    for (const Md::Block &b : doc.blocks)
        if (b.type == Md::Block::FigureRow) out += b.figures;
    return out;
}

} // namespace ReportAssembler
