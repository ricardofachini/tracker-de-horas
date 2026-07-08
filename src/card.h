#pragma once

#include <QFrame>

class QGraphicsDropShadowEffect;

// QFrame com sombra suave que "levanta" quando o mouse passa por cima.
// Demonstra a outra metade do sistema de eventos do Qt: além de signals,
// widgets recebem eventos sobrescrevendo métodos virtuais (enterEvent etc.).
class Card : public QFrame {
public:
    explicit Card(QWidget* parent = nullptr);

protected:
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void changeEvent(QEvent* event) override;

private:
    void animateShadow(qreal blurRadius, bool hovered);

    QGraphicsDropShadowEffect* m_shadow;
};
