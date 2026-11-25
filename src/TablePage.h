// SPDX-License-Identifier: MIT
#pragma once
#include <functional>

#include <QJsonObject>
#include <QPushButton>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QWidget>

#include "Project.h"

class QTableWidgetItem;
namespace Ui { class TablePage; }

// Description d'une colonne d'un tableau EBIOS RM.
struct Column {
    enum Type {
        Text,       // texte court, édition directe
        MultiText,  // texte long, édition par double-clic (boîte de dialogue)
        Combo,      // liste de valeurs fixes
        Check,      // case à cocher
        Ref,        // référence vers un enregistrement d'une autre table (liste déroulante)
        MultiRef,   // références multiples (boîte de dialogue à cases à cocher)
        Computed    // calculée, lecture seule
    };
    using ComputeFn = std::function<QString(const QJsonObject &, const Project &)>;

    QString key, title;
    Type type = Text;
    int width = 110;
    QStringList options;     // Combo
    QStringList refTables;   // Ref / MultiRef
    ComputeFn compute;       // Computed

    static Column text(const QString &key, const QString &title, int w = 140);
    static Column multi(const QString &key, const QString &title, int w = 240);
    static Column combo(const QString &key, const QString &title, const QStringList &opts, int w = 110);
    static Column check(const QString &key, const QString &title, int w = 56);
    static Column ref(const QString &key, const QString &title, const QString &table, int w = 230);
    static Column multiref(const QString &key, const QString &title, const QString &table, int w = 200);
    static Column computed(const QString &key, const QString &title, ComputeFn fn, int w = 110);
};

// Page générique : titre, aide, barre d'outils, tableau éditable relié à une table du Project.
class TablePage : public QWidget
{
    Q_OBJECT
public:
    TablePage(Project *project, const QString &tableName, const QString &title, const QString &help,
              const QVector<Column> &columns, const QString &idPrefix, QWidget *parent = nullptr);
    ~TablePage() override;

    // Filtre fixe + valeurs imposées aux nouvelles lignes (utilisé pour les actions d'un SO).
    void setFixedValues(const QJsonObject &fixed);
    void setStructureEditable(bool editable);                    // masque Ajouter / Supprimer / Monter / Descendre
    void setFooter(std::function<QString(const Project &)> fn);
    void addToolButton(const QString &text, std::function<void()> fn);
    void setTitleVisible(bool visible);

public slots:
    void rebuild();

protected:
    void showEvent(QShowEvent *) override;
    bool eventFilter(QObject *obj, QEvent *ev) override;

private slots:
    void onItemChanged(QTableWidgetItem *item);
    void onCellDoubleClicked(int row, int col);
    void onProjectChanged();
    void addRow();
    void deleteRows();
    void moveRow(int delta);
    void applyFilter();

private:
    bool matchesFixed(const QJsonObject &rec) const;
    void fillRow(int row, const QJsonObject &rec);
    void refreshComputed();
    void setValue(int arrayIndex, const QString &key, const QJsonValue &v);
    QString nextId() const;
    QStringList idsFor(const Column &c) const;
    QString labelFor(const Column &c, const QString &id) const;
    void updateFooter();
    static QString shorten(const QString &t);

    Project *project_;
    QString table_, idPrefix_;   // table_ = nom de la table dans le Project
    QVector<Column> cols_;
    QJsonObject fixed_;
    QVector<int> rowIndex_;           // ligne affichée -> index dans le tableau JSON
    std::function<QString(const Project &)> footerFn_;

    Ui::TablePage *ui = nullptr;      // interface définie dans ui/TablePage.ui (modifiable avec Qt Designer)

    bool loading_ = false;
    bool inEdit_ = false;
    bool stale_ = true;
};
