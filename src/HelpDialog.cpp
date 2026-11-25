// SPDX-License-Identifier: MIT
#include "HelpDialog.h"

#include "ui_HelpDialog.h"

#include <QDesktopServices>
#include <QFile>
#include <QListWidget>
#include <QShortcut>
#include <QTextBrowser>
#include <QTextCursor>

HelpDialog::HelpDialog(QWidget *parent) : QDialog(parent), ui(new Ui::HelpDialog)
{
    ui->setupUi(this);
    ui->splitter->setStretchFactor(1, 1);
    setWindowFlag(Qt::WindowMaximizeButtonHint, true);

    loadChapters();

    connect(ui->chapterList, &QListWidget::currentRowChanged, this, &HelpDialog::onChapterChanged);
    connect(ui->searchEdit, &QLineEdit::textChanged, this, &HelpDialog::onSearchChanged);
    connect(ui->searchEdit, &QLineEdit::returnPressed, this, &HelpDialog::findNext);
    connect(ui->findNextButton, &QPushButton::clicked, this, &HelpDialog::findNext);
    connect(ui->findPrevButton, &QPushButton::clicked, this, &HelpDialog::findPrevious);
    connect(ui->helpBrowser, &QTextBrowser::anchorClicked, this, &HelpDialog::onAnchorClicked);
    connect(ui->backButton, &QPushButton::clicked, this, &HelpDialog::previousChapter);
    connect(ui->forwardButton, &QPushButton::clicked, this, &HelpDialog::nextChapter);
    connect(ui->closeButton, &QPushButton::clicked, this, &QDialog::close);

    auto *f3 = new QShortcut(QKeySequence(Qt::Key_F3), this);
    connect(f3, &QShortcut::activated, this, &HelpDialog::findNext);
    auto *find = new QShortcut(QKeySequence::Find, this);
    connect(find, &QShortcut::activated, this, [this] { ui->searchEdit->setFocus(); ui->searchEdit->selectAll(); });

    if (ui->chapterList->count() > 0) ui->chapterList->setCurrentRow(0);
}

HelpDialog::~HelpDialog()
{
    delete ui;
}

void HelpDialog::loadChapters()
{
    QFile idx(QStringLiteral(":/help/index.txt"));
    if (!idx.open(QIODevice::ReadOnly)) {
        ui->helpBrowser->setPlainText(tr("Les rubriques d'aide sont introuvables dans l'exécutable."));
        return;
    }
    const QStringList lines = QString::fromUtf8(idx.readAll()).split('\n', Qt::SkipEmptyParts);
    for (const QString &raw : lines) {
        const QString line = raw.trimmed();
        const int sep = line.indexOf('|');
        if (sep <= 0) continue;
        Chapter c;
        c.key = line.left(sep).trimmed();
        c.title = line.mid(sep + 1).trimmed();
        QFile f(QStringLiteral(":/help/%1.md").arg(c.key));
        if (f.open(QIODevice::ReadOnly)) c.markdown = QString::fromUtf8(f.readAll());
        else c.markdown = QStringLiteral("# %1\n\n_(rubrique manquante)_").arg(c.title);
        chapters_.append(c);
        ui->chapterList->addItem(c.title);
    }
}

void HelpDialog::showChapter(const QString &key)
{
    ui->searchEdit->clear();
    for (int i = 0; i < chapters_.size(); ++i) {
        if (chapters_[i].key == key) {
            ui->chapterList->setCurrentRow(i);
            return;
        }
    }
    if (!chapters_.isEmpty()) ui->chapterList->setCurrentRow(0);
}

void HelpDialog::onChapterChanged(int row)
{
    if (row < 0 || row >= chapters_.size()) return;
    ui->helpBrowser->setMarkdown(chapters_[row].markdown);
    const QString t = ui->searchEdit->text().trimmed();
    if (!t.isEmpty()) {
        ui->helpBrowser->moveCursor(QTextCursor::Start);
        ui->helpBrowser->find(t);
    }
    ui->backButton->setEnabled(neighbour(row, -1) >= 0);
    ui->forwardButton->setEnabled(neighbour(row, +1) >= 0);
}

void HelpDialog::onSearchChanged(const QString &text)
{
    const QString t = text.trimmed();
    int visible = 0;
    for (int i = 0; i < chapters_.size(); ++i) {
        const bool match = t.isEmpty() || chapters_[i].markdown.contains(t, Qt::CaseInsensitive) ||
                           chapters_[i].title.contains(t, Qt::CaseInsensitive);
        ui->chapterList->item(i)->setHidden(!match);
        if (match) ++visible;
    }
    ui->resultLabel->setText(t.isEmpty() ? QString() : (visible ? tr("%1 rubrique(s)").arg(visible) : tr("Aucun résultat")));
    const int cur = ui->chapterList->currentRow();
    if (cur < 0 || ui->chapterList->item(cur)->isHidden()) {
        for (int i = 0; i < chapters_.size(); ++i) {
            if (!ui->chapterList->item(i)->isHidden()) {
                ui->chapterList->setCurrentRow(i);
                return;
            }
        }
    } else {
        onChapterChanged(cur);       // ré-affiche et surligne la première occurrence
    }
}

void HelpDialog::findNext()
{
    const QString t = ui->searchEdit->text().trimmed();
    if (t.isEmpty()) return;
    if (!ui->helpBrowser->find(t)) {
        ui->helpBrowser->moveCursor(QTextCursor::Start);
        ui->helpBrowser->find(t);
    }
}

void HelpDialog::findPrevious()
{
    const QString t = ui->searchEdit->text().trimmed();
    if (t.isEmpty()) return;
    if (!ui->helpBrowser->find(t, QTextDocument::FindBackward)) {
        ui->helpBrowser->moveCursor(QTextCursor::End);
        ui->helpBrowser->find(t, QTextDocument::FindBackward);
    }
}

void HelpDialog::onAnchorClicked(const QUrl &url)
{
    if (url.scheme() == QLatin1String("help")) showChapter(url.path());
    else QDesktopServices::openUrl(url);
}

int HelpDialog::neighbour(int from, int step) const
{
    for (int i = from + step; i >= 0 && i < chapters_.size(); i += step)
        if (!ui->chapterList->item(i)->isHidden()) return i;
    return -1;
}

void HelpDialog::previousChapter()
{
    const int i = neighbour(ui->chapterList->currentRow(), -1);
    if (i >= 0) ui->chapterList->setCurrentRow(i);
}

void HelpDialog::nextChapter()
{
    const int i = neighbour(ui->chapterList->currentRow(), +1);
    if (i >= 0) ui->chapterList->setCurrentRow(i);
}
