#include "card.h"

#include "theme.h"

#include <QEvent>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>

// No escuro a sombra precisa de mais alfa para continuar visível.
static QColor shadowColor(bool hovered) {
    const int alpha = Theme::isDark() ? (hovered ? 130 : 90) : (hovered ? 45 : 24);
    return QColor(0, 0, 0, alpha);
}

Card::Card(QWidget* parent) : QFrame(parent) {
    setProperty("card", true);
    m_shadow = new QGraphicsDropShadowEffect(this);
    m_shadow->setBlurRadius(24);
    m_shadow->setOffset(0, 4);
    m_shadow->setColor(shadowColor(false));
    setGraphicsEffect(m_shadow);
}

void Card::enterEvent(QEnterEvent* event) {
    QFrame::enterEvent(event);
    animateShadow(40, true);
}

void Card::leaveEvent(QEvent* event) {
    QFrame::leaveEvent(event);
    animateShadow(24, false);
}

// Reaplicar o stylesheet (troca de tema) dispara um StyleChange em cada
// widget — aproveitamos para atualizar a cor da sombra.
void Card::changeEvent(QEvent* event) {
    QFrame::changeEvent(event);
    if (event->type() == QEvent::StyleChange)
        m_shadow->setColor(shadowColor(underMouse()));
}

void Card::animateShadow(qreal blurRadius, bool hovered) {
    // "blurRadius" é uma Q_PROPERTY do efeito, então dá para animar
    // exatamente como fizemos com "opacity" no fade das páginas.
    auto* animation = new QPropertyAnimation(m_shadow, "blurRadius", this);
    animation->setDuration(160);
    animation->setEndValue(blurRadius);
    animation->setEasingCurve(QEasingCurve::OutCubic);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
    m_shadow->setColor(shadowColor(hovered));
}
