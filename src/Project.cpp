// SPDX-License-Identifier: MIT
#include "Project.h"

#include <QDate>
#include <QFile>
#include <QJsonDocument>
#include <QSaveFile>

Project::Project(QObject *parent) : QObject(parent)
{
    reset();
}

QJsonObject Project::defaultMetrics()
{
    const QString F = QStringLiteral("Faible"), M = QStringLiteral("Moyen"), V = QStringLiteral("Élevé");
    // ligne = gravité 1..4, colonne = vraisemblance 1..4 (valeurs lues dans la feuille « Traitement du Risque »)
    const QStringList mat = {F, F, M, M,
                             F, F, M, V,
                             F, M, V, V,
                             M, M, V, V};
    QJsonObject m;
    m["g"] = QJsonArray{"Négligeable", "Significatif", "Important", "Catastrophique"};
    m["v"] = QJsonArray{"Invraisemblable", "Moyennement vraisemblable", "Vraisemblable", "Très vraisemblable"};
    m["z_hp"] = 0.1;
    m["z_watch"] = 0.49;
    m["z_control"] = 1.0;
    m["z_danger"] = 3.0;
    m["matrix"] = QJsonArray::fromStringList(mat);
    return m;
}

void Project::reset()
{
    root_ = QJsonObject();
    root_["version"] = 1;

    QJsonObject somm;
    somm["titre"] = QStringLiteral("Nouvelle analyse EBIOS RM");
    somm["version"] = QStringLiteral("V1.0");
    somm["date"] = QDate::currentDate().toString(Qt::ISODate);
    root_["sommaire"] = somm;
    root_["metrics"] = defaultMetrics();
    root_["architecture"] = QJsonObject{{"description", ""}, {"image", ""}};
    root_["conclusion"] = QJsonObject{{"texte", ""}};
    root_["rapport"] = QJsonObject();    // mise en page : page de garde, logo, sommaire, figures (valeurs par défaut = tout activé)
    root_["tables"] = QJsonObject();

    path_.clear();
    dirty_ = false;
    emit changed();
}

bool Project::load(const QString &path, QString *error)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        if (error) *error = f.errorString();
        return false;
    }
    QJsonParseError pe;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &pe);
    if (doc.isNull() || !doc.isObject()) {
        if (error) *error = pe.errorString();
        return false;
    }
    root_ = doc.object();

    // Compléter les sections manquantes avec les valeurs par défaut
    QJsonObject metrics = defaultMetrics();
    const QJsonObject loaded = root_.value("metrics").toObject();
    for (auto it = loaded.begin(); it != loaded.end(); ++it)
        metrics[it.key()] = it.value();
    root_["metrics"] = metrics;
    if (!root_.contains("sommaire")) root_["sommaire"] = QJsonObject();
    if (!root_.contains("architecture")) root_["architecture"] = QJsonObject{{"description", ""}, {"image", ""}};
    if (!root_.contains("conclusion")) root_["conclusion"] = QJsonObject{{"texte", ""}};
    if (!root_.contains("rapport")) root_["rapport"] = QJsonObject();
    if (!root_.contains("tables")) root_["tables"] = QJsonObject();

    path_ = path;
    dirty_ = false;
    emit changed();
    return true;
}

bool Project::save(const QString &path, QString *error)
{
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        if (error) *error = f.errorString();
        return false;
    }
    f.write(QJsonDocument(root_).toJson(QJsonDocument::Indented));
    if (!f.commit()) {
        if (error) *error = f.errorString();
        return false;
    }
    path_ = path;
    dirty_ = false;
    emit changed();
    return true;
}

void Project::touch()
{
    dirty_ = true;
    emit changed();
}

QJsonArray Project::table(const QString &name) const
{
    return root_.value("tables").toObject().value(name).toArray();
}

void Project::setTable(const QString &name, const QJsonArray &rows)
{
    QJsonObject tables = root_.value("tables").toObject();
    tables[name] = rows;
    root_["tables"] = tables;
    touch();
}

QJsonObject Project::section(const QString &name) const
{
    return root_.value(name).toObject();
}

void Project::setSection(const QString &name, const QJsonObject &obj)
{
    root_[name] = obj;
    touch();
}

QJsonObject Project::find(const QString &table, const QString &id) const
{
    if (id.isEmpty()) return QJsonObject();
    const QJsonArray rows = this->table(table);
    for (const QJsonValue &v : rows) {
        const QJsonObject o = v.toObject();
        if (o.value("id").toString() == id) return o;
    }
    return QJsonObject();
}

QStringList Project::ids(const QString &table) const
{
    QStringList out;
    const QJsonArray rows = this->table(table);
    for (const QJsonValue &v : rows) {
        const QString id = v.toObject().value("id").toString();
        if (!id.isEmpty()) out << id;
    }
    return out;
}

QString Project::labelKey(const QString &table)
{
    if (table == "vm" || table == "er" || table == "ss" || table == "so" || table == "risques")
        return QStringLiteral("titre");
    if (table == "couples") return QStringLiteral("objectif");
    return QStringLiteral("nom");
}

QString Project::name(const QString &table, const QString &id) const
{
    return find(table, id).value(labelKey(table)).toString();
}

QString Project::label(const QString &table, const QString &id) const
{
    QString n = name(table, id).simplified();
    if (n.size() > 70) n = n.left(70) + QStringLiteral("…");
    return n.isEmpty() ? id : id + QStringLiteral(" — ") + n;
}

QString Project::gravityLabel(int g) const
{
    const QJsonArray a = root_.value("metrics").toObject().value("g").toArray();
    if (g < 1 || g > a.size()) return QString();
    return a.at(g - 1).toString();
}

QString Project::likelihoodLabel(int v) const
{
    const QJsonArray a = root_.value("metrics").toObject().value("v").toArray();
    if (v < 1 || v > a.size()) return QString();
    return a.at(v - 1).toString();
}

double Project::zone(const QString &key) const
{
    return root_.value("metrics").toObject().value(key).toDouble();
}

QString Project::matrixCell(int g, int v) const
{
    if (g < 1 || g > 4 || v < 1 || v > 4) return QString();
    const QJsonArray m = root_.value("metrics").toObject().value("matrix").toArray();
    const int idx = (g - 1) * 4 + (v - 1);
    return idx < m.size() ? m.at(idx).toString() : QString();
}
