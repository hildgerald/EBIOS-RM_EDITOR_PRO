// SPDX-License-Identifier: MIT
#include "TablePage.h"

#include "Calc.h"
#include "ui_TablePage.h"

#include <algorithm>
#include <functional>

#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QEvent>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QScrollBar>
#include <QTableWidget>
#include <QVBoxLayout>

static QString splitJoin(const QString &v)
{
    return Calc::splitIds(v).join(QStringLiteral(", "));
}

// ------------------------------------------------------------------ Column helpers
Column Column::text(const QString &k, const QString &t, int w)
{
    Column c; c.key = k; c.title = t; c.type = Text; c.width = w; return c;
}
Column Column::multi(const QString &k, const QString &t, int w)
{
    Column c; c.key = k; c.title = t; c.type = MultiText; c.width = w; return c;
}
Column Column::combo(const QString &k, const QString &t, const QStringList &o, int w)
{
    Column c; c.key = k; c.title = t; c.type = Combo; c.options = o; c.width = w; return c;
}
Column Column::check(const QString &k, const QString &t, int w)
{
    Column c; c.key = k; c.title = t; c.type = Check; c.width = w; return c;
}
Column Column::ref(const QString &k, const QString &t, const QString &table, int w)
{
    Column c; c.key = k; c.title = t; c.type = Ref; c.refTables = QStringList{table}; c.width = w; return c;
}
Column Column::multiref(const QString &k, const QString &t, const QString &table, int w)
{
    Column c; c.key = k; c.title = t; c.type = MultiRef; c.refTables = QStringList{table}; c.width = w; return c;
}
Column Column::computed(const QString &k, const QString &t, ComputeFn fn, int w)
{
    Column c; c.key = k; c.title = t; c.type = Computed; c.compute = std::move(fn); c.width = w; return c;
}

// ------------------------------------------------------------------ TablePage
TablePage::TablePage(Project *project, const QString &tableName, const QString &title, const QString &help,
                     const QVector<Column> &columns, const QString &idPrefix, QWidget *parent)
    : QWidget(parent), project_(project), table_(tableName), idPrefix_(idPrefix), cols_(columns)
{
    ui = new Ui::TablePage;
    ui->setupUi(this);

    ui->titleLabel->setText(title);
    ui->helpLabel->setText(help);
    ui->titleLabel->setVisible(!title.isEmpty());
    ui->helpLabel->setVisible(!help.isEmpty());

    ui->tableWidget->setColumnCount(cols_.size());
    QStringList heads;
    for (const Column &c : cols_) heads << c.title;
    ui->tableWidget->setHorizontalHeaderLabels(heads);
    for (int i = 0; i < cols_.size(); ++i) ui->tableWidget->setColumnWidth(i, cols_[i].width);

    connect(ui->addButton, &QPushButton::clicked, this, &TablePage::addRow);
    connect(ui->deleteButton, &QPushButton::clicked, this, &TablePage::deleteRows);
    connect(ui->upButton, &QPushButton::clicked, this, [this] { moveRow(-1); });
    connect(ui->downButton, &QPushButton::clicked, this, [this] { moveRow(+1); });
    connect(ui->filterEdit, &QLineEdit::textChanged, this, &TablePage::applyFilter);
    connect(ui->tableWidget, &QTableWidget::itemChanged, this, &TablePage::onItemChanged);
    connect(ui->tableWidget, &QTableWidget::cellDoubleClicked, this, &TablePage::onCellDoubleClicked);
    connect(project_, &Project::changed, this, &TablePage::onProjectChanged);
}

TablePage::~TablePage()
{
    delete ui;
}

void TablePage::setFixedValues(const QJsonObject &fixed)
{
    fixed_ = fixed;
    for (int i = 0; i < cols_.size(); ++i)
        if (fixed_.contains(cols_[i].key)) ui->tableWidget->setColumnHidden(i, true);
    rebuild();
}

void TablePage::setStructureEditable(bool editable)
{
    ui->addButton->setVisible(editable);
    ui->deleteButton->setVisible(editable);
    ui->upButton->setVisible(editable);
    ui->downButton->setVisible(editable);
}

void TablePage::setFooter(std::function<QString(const Project &)> fn)
{
    footerFn_ = std::move(fn);
    ui->footerLabel->setVisible(bool(footerFn_));
    updateFooter();
}

void TablePage::addToolButton(const QString &text, std::function<void()> fn)
{
    auto *b = new QPushButton(text);
    connect(b, &QPushButton::clicked, this, [fn] { fn(); });
    ui->toolbarLayout->insertWidget(ui->toolbarLayout->count() - 2, b);   // avant le stretch et le compteur
}

void TablePage::setTitleVisible(bool visible)
{
    ui->titleLabel->setVisible(visible);
    ui->helpLabel->setVisible(visible);
}

void TablePage::showEvent(QShowEvent *)
{
    if (stale_) rebuild();
}

void TablePage::onProjectChanged()
{
    if (inEdit_) return;
    if (isVisible()) rebuild();
    else stale_ = true;
}

bool TablePage::matchesFixed(const QJsonObject &rec) const
{
    for (auto it = fixed_.begin(); it != fixed_.end(); ++it)
        if (Calc::str(rec.value(it.key())) != Calc::str(it.value())) return false;
    return true;
}

QString TablePage::shorten(const QString &t)
{
    QString s = t.simplified();
    if (s.size() > 110) s = s.left(110) + QStringLiteral("…");
    return s;
}

QStringList TablePage::idsFor(const Column &c) const
{
    QStringList out;
    for (const QString &t : c.refTables) out << project_->ids(t);
    return out;
}

QString TablePage::labelFor(const Column &c, const QString &id) const
{
    for (const QString &t : c.refTables)
        if (!project_->find(t, id).isEmpty()) return project_->label(t, id);
    return id;
}

void TablePage::rebuild()
{
    loading_ = true;
    const QJsonArray arr = project_->table(table_);
    rowIndex_.clear();
    for (int i = 0; i < arr.size(); ++i)
        if (matchesFixed(arr.at(i).toObject())) rowIndex_.append(i);

    const int vpos = ui->tableWidget->verticalScrollBar()->value();
    ui->tableWidget->setRowCount(0);                    // supprime aussi les widgets de cellule
    ui->tableWidget->setRowCount(rowIndex_.size());
    for (int r = 0; r < rowIndex_.size(); ++r)
        fillRow(r, arr.at(rowIndex_[r]).toObject());
    loading_ = false;

    applyFilter();
    updateFooter();
    ui->tableWidget->verticalScrollBar()->setValue(vpos);
    stale_ = false;
}

void TablePage::fillRow(int row, const QJsonObject &rec)
{
    const int arrayIndex = rowIndex_[row];
    for (int c = 0; c < cols_.size(); ++c) {
        const Column &col = cols_[c];
        const QString val = Calc::s(rec, col.key);

        switch (col.type) {
        case Column::Text:
            ui->tableWidget->setItem(row, c, new QTableWidgetItem(val));
            break;

        case Column::MultiText: {
            auto *it = new QTableWidgetItem(shorten(val));
            it->setToolTip(val);
            it->setFlags(it->flags() & ~Qt::ItemIsEditable);
            ui->tableWidget->setItem(row, c, it);
            break;
        }

        case Column::Check: {
            auto *it = new QTableWidgetItem;
            it->setFlags((it->flags() | Qt::ItemIsUserCheckable) & ~Qt::ItemIsEditable);
            const QJsonValue jv = rec.value(col.key);
            const bool on = jv.isBool() ? jv.toBool() : (val == "true" || val == "Oui" || val == "oui" || val == "X");
            it->setCheckState(on ? Qt::Checked : Qt::Unchecked);
            ui->tableWidget->setItem(row, c, it);
            break;
        }

        case Column::Computed: {
            const QString text = col.compute ? col.compute(rec, *project_) : QString();
            auto *it = new QTableWidgetItem(text);
            it->setFlags(it->flags() & ~Qt::ItemIsEditable);
            it->setToolTip(text);
            const QColor bg = Calc::levelColor(text);
            it->setBackground(bg.isValid() ? bg : QColor("#f2f2f2"));
            ui->tableWidget->setItem(row, c, it);
            break;
        }

        case Column::Combo:
        case Column::Ref: {
            auto *cb = new QComboBox;
            cb->addItem(QString(), QString());
            if (col.type == Column::Combo) {
                for (const QString &o : col.options) cb->addItem(o, o);
            } else {
                for (const QString &id : idsFor(col)) cb->addItem(labelFor(col, id), id);
            }
            int idx = cb->findData(val);
            if (idx < 0 && !val.isEmpty()) {            // valeur orpheline (référence supprimée…)
                cb->addItem(val + QStringLiteral(" (?)"), val);
                idx = cb->count() - 1;
            }
            cb->setCurrentIndex(idx < 0 ? 0 : idx);
            cb->setFocusPolicy(Qt::StrongFocus);
            cb->installEventFilter(this);
            const QString key = col.key;
            connect(cb, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                    [this, cb, arrayIndex, key](int) {
                        if (loading_) return;
                        setValue(arrayIndex, key, cb->currentData().toString());
                    });
            ui->tableWidget->setCellWidget(row, c, cb);
            break;
        }

        case Column::MultiRef: {
            auto *btn = new QPushButton(val.isEmpty() ? QStringLiteral("—") : splitJoin(val));
            btn->setStyleSheet("text-align:left; padding:2px 6px;");
            btn->setToolTip(tr("Cliquer pour choisir les éléments associés"));
            const QString key = col.key;
            const Column colCopy = col;
            connect(btn, &QPushButton::clicked, this, [this, btn, arrayIndex, key, colCopy]() {
                const QJsonArray arr = project_->table(table_);
                if (arrayIndex < 0 || arrayIndex >= arr.size()) return;
                const QStringList current = Calc::splitIds(Calc::s(arr.at(arrayIndex).toObject(), key));

                QDialog dlg(this);
                dlg.setWindowTitle(colCopy.title);
                dlg.resize(560, 420);
                auto *l = new QVBoxLayout(&dlg);
                auto *list = new QListWidget;
                QStringList known;
                for (const QString &id : idsFor(colCopy)) {
                    auto *it = new QListWidgetItem(labelFor(colCopy, id));
                    it->setData(Qt::UserRole, id);
                    it->setFlags(it->flags() | Qt::ItemIsUserCheckable);
                    it->setCheckState(current.contains(id) ? Qt::Checked : Qt::Unchecked);
                    list->addItem(it);
                    known << id;
                }
                for (const QString &id : current) {
                    if (known.contains(id)) continue;
                    auto *it = new QListWidgetItem(id + QStringLiteral(" (introuvable)"));
                    it->setData(Qt::UserRole, id);
                    it->setFlags(it->flags() | Qt::ItemIsUserCheckable);
                    it->setCheckState(Qt::Checked);
                    list->addItem(it);
                }
                l->addWidget(list);
                auto *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
                connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
                connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
                l->addWidget(bb);
                if (dlg.exec() != QDialog::Accepted) return;

                QStringList chosen;
                for (int i = 0; i < list->count(); ++i)
                    if (list->item(i)->checkState() == Qt::Checked)
                        chosen << list->item(i)->data(Qt::UserRole).toString();
                const QString nv = chosen.join(",");
                setValue(arrayIndex, key, nv);
                btn->setText(nv.isEmpty() ? QStringLiteral("—") : splitJoin(nv));
            });
            ui->tableWidget->setCellWidget(row, c, btn);
            break;
        }
        }
    }
}

void TablePage::refreshComputed()
{
    const bool wasLoading = loading_;
    loading_ = true;
    const QJsonArray arr = project_->table(table_);
    for (int r = 0; r < rowIndex_.size(); ++r) {
        if (rowIndex_[r] >= arr.size()) continue;
        const QJsonObject rec = arr.at(rowIndex_[r]).toObject();
        for (int c = 0; c < cols_.size(); ++c) {
            if (cols_[c].type != Column::Computed) continue;
            QTableWidgetItem *it = ui->tableWidget->item(r, c);
            if (!it) continue;
            const QString text = cols_[c].compute ? cols_[c].compute(rec, *project_) : QString();
            it->setText(text);
            it->setToolTip(text);
            const QColor bg = Calc::levelColor(text);
            it->setBackground(bg.isValid() ? bg : QColor("#f2f2f2"));
        }
    }
    loading_ = wasLoading;
    updateFooter();
}

void TablePage::updateFooter()
{
    ui->countLabel->setText(tr("%1 élément(s)").arg(rowIndex_.size()));
    if (footerFn_) ui->footerLabel->setText(footerFn_(*project_));
}

void TablePage::setValue(int arrayIndex, const QString &key, const QJsonValue &v)
{
    QJsonArray arr = project_->table(table_);
    if (arrayIndex < 0 || arrayIndex >= arr.size()) return;
    QJsonObject o = arr.at(arrayIndex).toObject();
    o[key] = v;
    arr.replace(arrayIndex, o);
    inEdit_ = true;
    project_->setTable(table_, arr);
    inEdit_ = false;
    refreshComputed();
}

void TablePage::onItemChanged(QTableWidgetItem *item)
{
    if (loading_ || !item) return;
    const int row = item->row(), col = item->column();
    if (row < 0 || row >= rowIndex_.size() || col < 0 || col >= cols_.size()) return;
    const Column &c = cols_[col];
    if (c.type == Column::Text)
        setValue(rowIndex_[row], c.key, item->text());
    else if (c.type == Column::Check)
        setValue(rowIndex_[row], c.key, item->checkState() == Qt::Checked);
}

void TablePage::onCellDoubleClicked(int row, int col)
{
    if (row < 0 || row >= rowIndex_.size() || col < 0 || col >= cols_.size()) return;
    const Column &c = cols_[col];
    if (c.type != Column::MultiText) return;

    const QJsonArray arr = project_->table(table_);
    const int ai = rowIndex_[row];
    if (ai >= arr.size()) return;

    QDialog dlg(this);
    dlg.setWindowTitle(c.title);
    dlg.resize(720, 420);
    auto *l = new QVBoxLayout(&dlg);
    auto *edit = new QPlainTextEdit(Calc::s(arr.at(ai).toObject(), c.key));
    l->addWidget(edit);
    auto *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    l->addWidget(bb);
    if (dlg.exec() != QDialog::Accepted) return;

    const QString text = edit->toPlainText();
    setValue(ai, c.key, text);
    if (QTableWidgetItem *it = ui->tableWidget->item(row, col)) {
        loading_ = true;
        it->setText(shorten(text));
        it->setToolTip(text);
        loading_ = false;
    }
}

QString TablePage::nextId() const
{
    int maxN = 0;
    const QRegularExpression re(QStringLiteral("(\\d+)$"));
    for (const QJsonValue &v : project_->table(table_)) {
        const QString id = Calc::s(v.toObject(), "id");
        if (!id.startsWith(idPrefix_)) continue;
        const auto m = re.match(id);
        if (m.hasMatch()) maxN = std::max(maxN, m.captured(1).toInt());
    }
    return QStringLiteral("%1%2").arg(idPrefix_).arg(maxN + 1, 2, 10, QLatin1Char('0'));
}

void TablePage::addRow()
{
    QJsonArray arr = project_->table(table_);
    QJsonObject rec = fixed_;
    rec["id"] = nextId();
    for (const Column &c : cols_)
        if (c.type == Column::Check && !rec.contains(c.key)) rec[c.key] = false;
    arr.append(rec);
    inEdit_ = true;
    project_->setTable(table_, arr);
    inEdit_ = false;
    rebuild();
    ui->tableWidget->scrollToBottom();
    if (ui->tableWidget->rowCount() > 0) ui->tableWidget->selectRow(ui->tableWidget->rowCount() - 1);
}

void TablePage::deleteRows()
{
    const QModelIndexList sel = ui->tableWidget->selectionModel()->selectedRows();
    if (sel.isEmpty()) return;
    if (QMessageBox::question(this, tr("Supprimer"),
                              tr("Supprimer %1 ligne(s) sélectionnée(s) ?\nLes références existantes vers ces éléments ne sont pas modifiées.")
                                  .arg(sel.size())) != QMessageBox::Yes)
        return;
    QVector<int> idx;
    for (const QModelIndex &m : sel)
        if (m.row() >= 0 && m.row() < rowIndex_.size()) idx << rowIndex_[m.row()];
    std::sort(idx.begin(), idx.end(), std::greater<int>());
    QJsonArray arr = project_->table(table_);
    for (int i : idx)
        if (i >= 0 && i < arr.size()) arr.removeAt(i);
    inEdit_ = true;
    project_->setTable(table_, arr);
    inEdit_ = false;
    rebuild();
}

void TablePage::moveRow(int delta)
{
    const QModelIndexList sel = ui->tableWidget->selectionModel()->selectedRows();
    if (sel.size() != 1) return;
    const int r = sel.first().row(), r2 = r + delta;
    if (r < 0 || r2 < 0 || r >= rowIndex_.size() || r2 >= rowIndex_.size()) return;
    QJsonArray arr = project_->table(table_);
    const int a = rowIndex_[r], b = rowIndex_[r2];
    const QJsonValue va = arr.at(a), vb = arr.at(b);
    arr.replace(a, vb);
    arr.replace(b, va);
    inEdit_ = true;
    project_->setTable(table_, arr);
    inEdit_ = false;
    rebuild();
    ui->tableWidget->selectRow(r2);
}

void TablePage::applyFilter()
{
    const QString f = ui->filterEdit->text().trimmed();
    const QJsonArray arr = project_->table(table_);
    for (int r = 0; r < ui->tableWidget->rowCount(); ++r) {
        bool show = true;
        if (!f.isEmpty() && r < rowIndex_.size() && rowIndex_[r] < arr.size()) {
            const QString blob = QString::fromUtf8(QJsonDocument(arr.at(rowIndex_[r]).toObject()).toJson(QJsonDocument::Compact));
            show = blob.contains(f, Qt::CaseInsensitive);
        }
        ui->tableWidget->setRowHidden(r, !show);
    }
}

bool TablePage::eventFilter(QObject *obj, QEvent *ev)
{
    // Empêche la molette de modifier une liste déroulante non sélectionnée : elle fait défiler le tableau.
    if (ev->type() == QEvent::Wheel) {
        auto *cb = qobject_cast<QComboBox *>(obj);
        if (cb && !cb->hasFocus()) {
            QCoreApplication::sendEvent(ui->tableWidget->verticalScrollBar(), ev);
            return true;
        }
    }
    return QWidget::eventFilter(obj, ev);
}
