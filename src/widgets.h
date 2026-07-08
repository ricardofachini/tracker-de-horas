#pragma once

#include "card.h"

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
