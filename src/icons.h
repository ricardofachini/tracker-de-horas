#pragma once

#include "theme.h"

#include <QIcon>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QtMath>

// Ícones desenhados em código com QPainter: sem arquivos de imagem nem
// dependência do tema de ícones do sistema.

enum class NavGlyph { Today, Sheet, History, Reports };

inline QPixmap paintGlyph(NavGlyph glyph, const QColor& color, int size = 20) {
    // Desenha em 2x e informa o devicePixelRatio para ficar nítido em HiDPI.
    const qreal dpr = 2.0;
    QPixmap pixmap(int(size * dpr), int(size * dpr));
    pixmap.setDevicePixelRatio(dpr);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(color, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

    switch (glyph) {
    case NavGlyph::Today:  // relógio
        painter.drawEllipse(QRectF(3, 3, 14, 14));
        painter.drawLine(QPointF(10, 6.5), QPointF(10, 10));
        painter.drawLine(QPointF(10, 10), QPointF(13, 12));
        break;
    case NavGlyph::Sheet:  // calendário
        painter.drawRoundedRect(QRectF(3, 4.5, 14, 12), 2, 2);
        painter.drawLine(QPointF(6.5, 2.5), QPointF(6.5, 6));
        painter.drawLine(QPointF(13.5, 2.5), QPointF(13.5, 6));
        painter.drawLine(QPointF(3, 8.5), QPointF(17, 8.5));
        break;
    case NavGlyph::History:  // linhas de lista
        painter.drawLine(QPointF(4, 5.5), QPointF(16, 5.5));
        painter.drawLine(QPointF(4, 10), QPointF(16, 10));
        painter.drawLine(QPointF(4, 14.5), QPointF(11, 14.5));
        break;
    case NavGlyph::Reports:  // barras de gráfico
        painter.drawLine(QPointF(5, 16), QPointF(5, 11));
        painter.drawLine(QPointF(10, 16), QPointF(10, 6));
        painter.drawLine(QPointF(15, 16), QPointF(15, 9));
        break;
    }
    return pixmap;
}

// QIcon com dois "estados": Off (botão normal) e On (botão checked).
// As cores vêm do tema ativo — por isso os ícones são recriados no toggle.
inline QIcon navIcon(NavGlyph glyph) {
    QIcon icon;
    icon.addPixmap(paintGlyph(glyph, Theme::iconMuted()), QIcon::Normal, QIcon::Off);
    icon.addPixmap(paintGlyph(glyph, Theme::accentStrong()), QIcon::Normal, QIcon::On);
    return icon;
}

// Ícone do botão de tema: mostra o modo de destino (lua no claro, sol no escuro).
inline QIcon themeToggleIcon() {
    const QColor color = Theme::iconMuted();
    const qreal dpr = 2.0;
    QPixmap pixmap(40, 40);
    pixmap.setDevicePixelRatio(dpr);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    if (Theme::isDark()) {  // sol
        painter.setPen(QPen(color, 1.8, Qt::SolidLine, Qt::RoundCap));
        painter.drawEllipse(QRectF(6.5, 6.5, 7.0, 7.0));
        for (int i = 0; i < 8; ++i) {
            const qreal angle = qDegreesToRadians(i * 45.0);
            const QPointF direction(qCos(angle), qSin(angle));
            painter.drawLine(QPointF(10, 10) + direction * 6.0,
                             QPointF(10, 10) + direction * 8.2);
        }
    } else {  // lua: círculo cheio menos um círculo deslocado
        QPainterPath moon;
        moon.addEllipse(QRectF(4, 4, 12, 12));
        QPainterPath bite;
        bite.addEllipse(QRectF(7.5, 1.5, 12, 12));
        painter.fillPath(moon.subtracted(bite), color);
    }
    return QIcon(pixmap);
}
