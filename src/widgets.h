#pragma once

#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QLabel>
#include <QStyle>

// Helpers de UI: os "papéis" (card, role, state) viram seletores no style.qss.

inline QFrame* makeCard(QWidget* parent = nullptr) {
    auto* frame = new QFrame(parent);
    frame->setProperty("card", true);
    // Sombra suave: QSS não tem box-shadow, então usamos um efeito gráfico.
    // O frame vira "pai" do efeito, então o Qt destrói os dois juntos.
    auto* shadow = new QGraphicsDropShadowEffect(frame);
    shadow->setBlurRadius(24);
    shadow->setOffset(0, 4);
    shadow->setColor(QColor(0, 0, 0, 24));
    frame->setGraphicsEffect(shadow);
    return frame;
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
