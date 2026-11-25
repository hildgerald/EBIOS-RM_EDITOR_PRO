// SPDX-License-Identifier: MIT
#pragma once
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QString>

// Évite de réaffecter un texte identique (ce qui déplacerait le curseur pendant la saisie).
inline void setIfDifferent(QLineEdit *e, const QString &t)
{
    if (e->text() != t) e->setText(t);
}
inline void setIfDifferent(QPlainTextEdit *e, const QString &t)
{
    if (e->toPlainText() != t) e->setPlainText(t);
}
