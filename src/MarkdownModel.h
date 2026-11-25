// SPDX-License-Identifier: MIT
#pragma once
#include <QColor>
#include <QString>
#include <QVector>

// Modèle intermédiaire minimal du rapport, obtenu en analysant le Markdown produit par ReportGenerator.
// Il sert de source commune aux exports PDF (via HTML) et DOCX.
namespace Md {

struct Run {
    QString text;
    bool bold = false;
    bool italic = false;
    bool lineBreak = false;   // saut de ligne (text vide)
};
using Runs = QVector<Run>;

struct Figure {
    QString src;              // source de l'image (URL file:///… ou chemin)
    QString title;            // titre sans numéro
    int number = 0;           // numéro de figure (attribué à l'assemblage)
    QString caption() const { return QStringLiteral("Figure %1 – %2").arg(number).arg(title); }
};

struct Cell {
    Runs runs;
    QColor fill;              // invalide = pas de fond
};

struct Block {
    enum Type { Heading, Paragraph, Bullet, Table, Image, Rule, FigureRow };
    Type type = Paragraph;
    int level = 0;            // niveau de titre (1..4) ou d'imbrication d'une puce
    QString marker;           // puce : "•" ou "1."
    Runs runs;                // titre, paragraphe, puce
    QVector<Cell> head;       // tableau : en-têtes
    QVector<QVector<Cell>> rows;
    QString imageSrc, alt;    // image : source telle qu'écrite dans le Markdown
    QString rowTitle;         // planche de figures : titre de la page
    QVector<Figure> figures;  // planche de figures : 1 ou 2 figures côte à côte
};

struct Document {
    QVector<Block> blocks;
};

Document parse(const QString &markdown);

QString plain(const Runs &runs);                       // texte brut (sauts de ligne -> espace)
QString localPath(const QString &imageSrc);            // chemin local d'une source d'image (file:///…, %20…)
QVector<double> columnWeights(const Block &table);     // largeurs relatives (somme = 1)
bool startsNewPage(const QString &headingText);        // chapitres qui démarrent une nouvelle page

// Rendu HTML (sous-ensemble compris par QTextDocument) pour l'export PDF.
QString toHtml(const Document &doc);

} // namespace Md
