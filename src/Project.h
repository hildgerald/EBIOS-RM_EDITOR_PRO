// SPDX-License-Identifier: MIT
#pragma once
#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QStringList>

// Modèle de données de l'analyse EBIOS RM.
// Tout est stocké dans un unique QJsonObject (fichier *.ebios = JSON) :
//   { "version":1, "sommaire":{...}, "metrics":{...}, "architecture":{...},
//     "tables": { "vm":[...], "bs":[...], "er":[...], ... } }
class Project : public QObject
{
    Q_OBJECT
public:
    explicit Project(QObject *parent = nullptr);

    void reset();
    bool load(const QString &path, QString *error = nullptr);
    bool save(const QString &path, QString *error = nullptr);

    QString path() const { return path_; }
    bool isDirty() const { return dirty_; }

    // Tables (listes d'enregistrements)
    QJsonArray table(const QString &name) const;
    void setTable(const QString &name, const QJsonArray &rows);

    // Sections (objets uniques : sommaire, metrics, architecture)
    QJsonObject section(const QString &name) const;
    void setSection(const QString &name, const QJsonObject &obj);

    // Accès par identifiant
    QJsonObject find(const QString &table, const QString &id) const;
    QStringList ids(const QString &table) const;
    QString name(const QString &table, const QString &id) const;   // libellé seul
    QString label(const QString &table, const QString &id) const;  // "ID — libellé"
    static QString labelKey(const QString &table);

    // Métriques
    QString gravityLabel(int g) const;
    QString likelihoodLabel(int v) const;
    double zone(const QString &key) const;          // z_hp, z_watch, z_control, z_danger
    QString matrixCell(int g, int v) const;         // Faible / Moyen / Élevé

    static QJsonObject defaultMetrics();

signals:
    void changed();

private:
    void touch();

    QJsonObject root_;
    QString path_;
    bool dirty_ = false;
};
