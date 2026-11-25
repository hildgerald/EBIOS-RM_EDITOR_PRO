// SPDX-License-Identifier: MIT
#include "MainWindow.h"

#include "ArchitecturePage.h"
#include "ConclusionPage.h"
#include "HelpDialog.h"
#include "MetricsPage.h"
#include "OperationalDetailPage.h"
#include "PageFactory.h"
#include "RadarPage.h"
#include "ReportLayoutPage.h"
#include "ReportPage.h"
#include "SummaryPage.h"
#include "TreatmentPage.h"
#include "ui_MainWindow.h"

#ifndef APP_VERSION
#define APP_VERSION "1.2.0"
#endif

#include <QAction>
#include <QCloseEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QMenuBar>
#include <QMessageBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QStatusBar>
#include <QStyle>
#include <QToolBar>
#include <QTreeWidget>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow), project_(new Project(this))
{
    ui->setupUi(this);
    ui->splitter->setStretchFactor(1, 1);

    // La barre de menu reste dans la fenêtre : sous Linux, certains bureaux (Unity, KDE « menu global »)
    // la déplaceraient dans le bandeau supérieur de l'écran. Sous macOS, le menu natif est conservé.
#ifndef Q_OS_MACOS
    ui->menubar->setNativeMenuBar(false);
#endif
    ui->menubar->setVisible(true);

    auto group = [this](const QString &t, const QString &helpKey) {
        auto *it = new QTreeWidgetItem(ui->navTree, QStringList(t));
        it->setData(0, Qt::UserRole + 1, helpKey);
        QFont f = it->font(0);
        f.setBold(true);
        it->setFont(0, f);
        it->setFlags(it->flags() & ~Qt::ItemIsSelectable);
        return it;
    };

    auto *g0 = group(tr("Général"), "04_sommaire_metriques");
    addPage(g0, tr("Sommaire"), new SummaryPage(project_));
    addPage(g0, tr("Métriques"), new MetricsPage(project_));

    auto *g1 = group(tr("Atelier 1 – Cadrage et socle"), "05_atelier1");
    addPage(g1, tr("Valeurs métiers"), Pages::valeursMetiers(project_));
    addPage(g1, tr("Biens supports"), Pages::biensSupports(project_));
    addPage(g1, tr("Événements redoutés"), Pages::evenementsRedoutes(project_));
    addPage(g1, tr("Socle de sécurité"), Pages::socle(project_));
    addPage(g1, tr("Vulnérabilités"), Pages::vulnerabilites(project_));
    addPage(g1, tr("Architecture"), new ArchitecturePage(project_));

    auto *g2 = group(tr("Atelier 2 – Sources de risque"), "06_atelier2");
    addPage(g2, tr("Sources de risque"), Pages::sourcesRisque(project_));
    addPage(g2, tr("Objectifs visés"), Pages::objectifsVises(project_));
    addPage(g2, tr("Couples SR / OV"), Pages::couplesSrOv(project_));

    auto *g3 = group(tr("Atelier 3 – Scénarios stratégiques"), "07_atelier3");
    addPage(g3, tr("Parties prenantes"), Pages::partiesPrenantes(project_));
    addPage(g3, tr("Parties prenantes (projection)"), Pages::partiesPrenantesProjection(project_));
    radar_ = new RadarPage(project_);
    addPage(g3, tr("Radar de la menace"), radar_);
    addPage(g3, tr("Scénarios stratégiques"), Pages::scenariosStrategiques(project_));

    auto *g4 = group(tr("Atelier 4 – Scénarios opérationnels"), "08_atelier4");
    addPage(g4, tr("Scénarios opérationnels"), Pages::scenariosOperationnels(project_));
    addPage(g4, tr("Détail (actions élémentaires)"), new OperationalDetailPage(project_));

    auto *g5 = group(tr("Atelier 5 – Traitement du risque"), "09_atelier5");
    addPage(g5, tr("Scénarios de risque"), Pages::scenariosRisque(project_));
    addPage(g5, tr("Mesures complémentaires"), Pages::mesuresComplementaires(project_));
    addPage(g5, tr("Risques résiduels"), Pages::risquesResiduels(project_));
    addPage(g5, tr("Matrices de risque"), new TreatmentPage(project_));

    auto *g6 = group(tr("Rapport"), "10_rapport");
    addPage(g6, tr("Conclusion"), new ConclusionPage(project_));
    addPage(g6, tr("Mise en page (garde, sommaire)"), new ReportLayoutPage(project_), "11_miseenpage");
    report_ = new ReportPage(project_);
    addPage(g6, tr("Rapport (aperçu et exports)"), report_);

    ui->navTree->expandAll();

    // Menu « Ateliers » : accès direct à chaque groupe de pages (Ctrl+1 … Ctrl+7)
    for (int i = 0; i < ui->navTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem *grp = ui->navTree->topLevelItem(i);
        QAction *a = ui->menuGo->addAction(grp->text(0));
        a->setShortcut(QKeySequence(QStringLiteral("Ctrl+%1").arg(i + 1)));
        connect(a, &QAction::triggered, this, [this, grp] {
            if (grp->childCount() > 0) ui->navTree->setCurrentItem(grp->child(0));
        });
    }

    // Menu « Affichage » et barre d'outils
    connect(ui->actionExpandAll, &QAction::triggered, ui->navTree, &QTreeWidget::expandAll);
    connect(ui->actionCollapseAll, &QAction::triggered, ui->navTree, &QTreeWidget::collapseAll);
    connect(ui->actionFullScreen, &QAction::toggled, this, [this](bool on) {
        if (on) showFullScreen();
        else showNormal();
    });
    ui->menuView->addSeparator();
    ui->menuView->addAction(ui->toolBar->toggleViewAction());     // afficher / masquer la barre d'outils
    ui->toolBar->toggleViewAction()->setText(tr("Barre d'outils"));
    ui->actionNew->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
    ui->actionOpen->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
    ui->actionSave->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    ui->actionHelp->setIcon(style()->standardIcon(QStyle::SP_DialogHelpButton));
    connect(ui->navTree, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem *cur, QTreeWidgetItem *) { onNavChanged(cur); });
    ui->navTree->setCurrentItem(g0->child(0));

    // Actions définies dans ui/MainWindow.ui (menus, raccourcis) : seul le câblage est ici.
    connect(ui->actionNew, &QAction::triggered, this, &MainWindow::newProject);
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::openProject);
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::saveProject);
    connect(ui->actionSaveAs, &QAction::triggered, this, &MainWindow::saveProjectAs);
    connect(ui->actionQuit, &QAction::triggered, this, &QWidget::close);
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::about);
    connect(ui->actionHelp, &QAction::triggered, this, &MainWindow::showContextHelp);
    connect(ui->actionHelpQuickStart, &QAction::triggered, this, [this] { showHelp("01_demarrage"); });
    connect(ui->actionHelpShortcuts, &QAction::triggered, this, [this] { showHelp("13_raccourcis"); });
    connect(ui->actionHelpGlossary, &QAction::triggered, this, [this] { showHelp("14_glossaire"); });
    connect(ui->actionExportMd, &QAction::triggered, this, [this] { report_->exportMarkdown(this); });
    connect(ui->actionExportPdf, &QAction::triggered, this, [this] { report_->exportPdf(this); });
    connect(ui->actionExportDocx, &QAction::triggered, this, [this] { report_->exportDocx(this); });
    connect(ui->actionExportRadar, &QAction::triggered, this, [this] { radar_->exportPng(this); });

    connect(project_, &Project::changed, this, &MainWindow::updateTitle);
    updateTitle();
    ui->statusbar->showMessage(tr("Prêt"));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::addPage(QTreeWidgetItem *parent, const QString &title, QWidget *page, const QString &helpKey)
{
    const int idx = ui->stack->addWidget(page);
    auto *it = new QTreeWidgetItem(parent, QStringList(title));
    it->setData(0, Qt::UserRole, idx);
    // Rubrique d'aide : celle de la page, à défaut celle du groupe
    it->setData(0, Qt::UserRole + 1, helpKey.isEmpty() ? parent->data(0, Qt::UserRole + 1).toString() : helpKey);
}

void MainWindow::onNavChanged(QTreeWidgetItem *cur)
{
    if (!cur) return;
    const QVariant v = cur->data(0, Qt::UserRole);
    if (v.isValid()) ui->stack->setCurrentIndex(v.toInt());
}

void MainWindow::updateTitle()
{
    const QString file = project_->path().isEmpty() ? tr("(sans titre)") : QFileInfo(project_->path()).fileName();
    setWindowTitle(QStringLiteral("%1%2 — EBIOS RM Studio").arg(file, project_->isDirty() ? QStringLiteral(" *") : QString()));
}

bool MainWindow::maybeSave()
{
    if (!project_->isDirty()) return true;
    const auto r = QMessageBox::question(this, tr("Modifications non enregistrées"),
                                         tr("Enregistrer les modifications avant de continuer ?"),
                                         QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (r == QMessageBox::Cancel) return false;
    if (r == QMessageBox::Save) return saveProject();
    return true;
}

void MainWindow::newProject()
{
    if (!maybeSave()) return;
    project_->reset();
}

void MainWindow::openProject()
{
    if (!maybeSave()) return;
    const QString path = QFileDialog::getOpenFileName(this, tr("Ouvrir un projet"), QString(), tr("Projet EBIOS (*.ebios *.json)"));
    if (!path.isEmpty()) openFile(path);
}

void MainWindow::openFile(const QString &path)
{
    QString err;
    if (!project_->load(path, &err))
        QMessageBox::warning(this, tr("Ouverture"), tr("Impossible d'ouvrir le fichier :\n%1").arg(err));
    else
        ui->statusbar->showMessage(tr("Projet chargé : %1").arg(path), 4000);
}

bool MainWindow::saveProject()
{
    if (project_->path().isEmpty()) return saveProjectAs();
    QString err;
    if (!project_->save(project_->path(), &err)) {
        QMessageBox::warning(this, tr("Enregistrement"), tr("Échec : %1").arg(err));
        return false;
    }
    ui->statusbar->showMessage(tr("Projet enregistré"), 3000);
    return true;
}

bool MainWindow::saveProjectAs()
{
    QString path = QFileDialog::getSaveFileName(this, tr("Enregistrer le projet"), QStringLiteral("analyse.ebios"), tr("Projet EBIOS (*.ebios)"));
    if (path.isEmpty()) return false;
    if (QFileInfo(path).suffix().isEmpty()) path += ".ebios";
    QString err;
    if (!project_->save(path, &err)) {
        QMessageBox::warning(this, tr("Enregistrement"), tr("Échec : %1").arg(err));
        return false;
    }
    ui->statusbar->showMessage(tr("Projet enregistré"), 3000);
    return true;
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    if (maybeSave()) e->accept();
    else e->ignore();
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("À propos d'EBIOS RM Studio"),
        tr("<h3>EBIOS RM Studio</h3>"
           "<p>Version %1</p>"
           "<p>Analyse de risques selon la méthode EBIOS Risk Manager (ANSSI) : 5 ateliers, radar de la menace, "
           "matrices de risque, rapport Markdown, PDF et Word.</p>"
           "<p><b>Licence MIT</b> – logiciel libre : voir le fichier <i>LICENSE</i>.</p>"
           "<p>Ce logiciel utilise <b>Qt %2</b> (modules Core, Gui et Widgets), sous licence "
           "<b>GNU LGPL v3</b> – <a href=\"https://www.qt.io/licensing/\">qt.io/licensing</a>. "
           "Qt est lié dynamiquement et peut être remplacé par une version modifiée. "
           "Voir <i>THIRD_PARTY_NOTICES.md</i>.</p>"
           "<p>Aide : touche F1.</p>")
            .arg(QStringLiteral(APP_VERSION), QString::fromLatin1(qVersion())));
}

void MainWindow::showHelp(const QString &key)
{
    if (!helpDialog_) helpDialog_ = new HelpDialog(this);
    helpDialog_->showChapter(key);
    helpDialog_->show();
    helpDialog_->raise();
    helpDialog_->activateWindow();
}

void MainWindow::showContextHelp()
{
    QString key = QStringLiteral("00_accueil");
    if (QTreeWidgetItem *it = ui->navTree->currentItem()) {
        const QString k = it->data(0, Qt::UserRole + 1).toString();
        if (!k.isEmpty()) key = k;
    }
    showHelp(key);
}
