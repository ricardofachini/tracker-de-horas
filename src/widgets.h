#pragma once

#include "card.h"
#include "hourbank.h"

#include <QLabel>
#include <QStyle>

// Helpers de UI: os "papéis" (card, role, state) viram seletores no style.qss.

inline QFrame* makeCard(QWidget* parent = nullptr) {
    return new Card(parent);
}

inline QLabel* makeLabel(const QString& text, const char* role, QWidget* parent = nullptr) {
    auto* label = new QLabel(text, parent);
    label->setProperty("role", role);
    return label;
}

// Troca uma propriedade dinâmica e reaplica o stylesheet.
inline void setUiState(QWidget* widget, const char* property, const QString& value) {
    widget->setProperty(property, value);
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
}

// Papel visual de um saldo do banco de horas (propriedade "balance").
inline const char* balanceState(int seconds) {
    return seconds > 0 ? "credit" : seconds < 0 ? "debit" : "zero";
}

// Pílula de saldo: verde no crédito, vermelha no débito, neutra zerada;
// "partial" (âmbar) marca o dia em andamento, que ainda não entrou no banco.
inline QLabel* makeBalancePill(int seconds, bool partial = false, QWidget* parent = nullptr) {
    auto* pill = new QLabel(HourBank::formatBalance(seconds), parent);
    pill->setObjectName("balancePill");
    pill->setProperty("balance", partial ? "partial" : balanceState(seconds));
    pill->setAlignment(Qt::AlignCenter);
    pill->setMinimumWidth(92);
    return pill;
}
