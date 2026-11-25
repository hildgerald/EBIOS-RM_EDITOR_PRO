// SPDX-License-Identifier: MIT
#include "OperationalDetailPage.h"

#include "Calc.h"
#include "TablePage.h"
#include "Widgets.h"
#include "ui_OperationalDetailPage.h"

#include <QJsonArray>
#include <QListWidget>
#include <QVBoxLayout>

using Calc::s;

OperationalDetailPage::OperationalDetailPage(Project *p, QWidget *parent)
    : QWidget(parent), ui(new Ui::OperationalDetailPage), project_(p)
{
    ui->setupUi(this);
    ui->splitterMain->setStretchFactor(1, 1);
    ui->splitterRight->setStretchFactor(0, 3);
    ui->splitterRight->setStretchFactor(1, 2);

    // Le tableau des actions est un TablePage (colonnes décrites en C++) inséré dans le conteneur « actionsHost » du .ui.
    const QStringList phases = {"Connaître", "Entrer", "Explorer", "Exploiter"};
    const QVector<Column> cols = {
        Column::text("id", tr("ID"), 70),
        Column::text("so", tr("SO"), 60),
        Column::combo("phase", tr("Phase"), phases, 100),
        Column::multi("action", tr("Action élémentaire"), 420),
        Column::combo("vraisemblance", tr("Vraisemblance"), {"1", "2", "3", "4"}, 100),
        Column::multiref("bs", tr("Biens supports"), "bs", 220),
    };
    actions_ = new TablePage(p, "actions", QString(), QString(), cols, "AC-");
    actions_->setTitleVisible(false);
    ui->actionsLayout->addWidget(actions_);

    connect(ui->listWidget, &QListWidget::currentRowChanged, this, &OperationalDetailPage::onSelect);
    connect(project_, &Project::changed, this, &OperationalDetailPage::refreshList);
    refreshList();
}

OperationalDetailPage::~OperationalDetailPage()
{
    delete ui;
}

void OperationalDetailPage::refreshList()
{
    refreshing_ = true;
    const QString keep = current_;
    ui->listWidget->clear();
    int sel = -1;
    for (const QJsonValue &jv : project_->table("so")) {
        const QJsonObject o = jv.toObject();
        const QString id = s(o, "id");
        const int lv = Calc::soLikelihood(id, *project_);
        auto *it = new QListWidgetItem(QStringLiteral("%1 — %2  [V=%3]").arg(id, s(o, "titre").simplified().left(60),
                                                                             lv > 0 ? QString::number(lv) : QStringLiteral("?")));
        it->setData(Qt::UserRole, id);
        ui->listWidget->addItem(it);
        if (id == keep) sel = ui->listWidget->count() - 1;
    }
    if (sel < 0 && ui->listWidget->count() > 0) sel = 0;
    ui->listWidget->setCurrentRow(sel);
    refreshing_ = false;
    if (sel >= 0) {
        current_ = ui->listWidget->item(sel)->data(Qt::UserRole).toString();
        // Le tableau d'actions se met à jour tout seul ; on ne le reconstruit que si le scénario affiché change
        // (sinon on détruirait un widget en cours d'édition).
        if (current_ != shown_) {
            actions_->setFixedValues(QJsonObject{{"so", current_}});
            shown_ = current_;
        }
    } else {
        current_.clear();
    }
    refreshChain();
}

void OperationalDetailPage::onSelect()
{
    if (refreshing_) return;
    QListWidgetItem *it = ui->listWidget->currentItem();
    if (!it) return;
    current_ = it->data(Qt::UserRole).toString();
    if (current_ != shown_) {
        actions_->setFixedValues(QJsonObject{{"so", current_}});
        shown_ = current_;
    }
    refreshChain();
}

void OperationalDetailPage::refreshChain()
{
    if (current_.isEmpty()) {
        ui->chainWidget->setData(QString(), 0, {});
        return;
    }
    QVector<KillChainAction> acts;
    for (const QJsonValue &jv : project_->table("actions")) {
        const QJsonObject a = jv.toObject();
        if (s(a, "so") != current_) continue;
        acts.append({s(a, "phase"), s(a, "action"), Calc::toInt(s(a, "vraisemblance"))});
    }
    const QJsonObject so = project_->find("so", current_);
    ui->chainWidget->setData(current_ + QStringLiteral(" : ") + s(so, "titre").simplified(), Calc::soLikelihood(current_, *project_), acts);
}
