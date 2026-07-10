#pragma once

#include <QDate>
#include <QList>
#include <QWidget>

// Gráfico de barras das horas trabalhadas em uma semana (seg–dom),
// desenhado com QPainter no estilo do app. As cores vêm do Theme em
// tempo de pintura, então a troca de tema repinta certo sozinha.
class WeekChart : public QWidget {
public:
    explicit WeekChart(QWidget* parent = nullptr);

    // `monday` é a segunda-feira da semana; `workedSeconds` traz 7 valores.
    void setWeek(const QDate& monday, const QList<int>& workedSeconds);

    QSize minimumSizeHint() const override { return {360, 220}; }

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    QRectF plotRect() const;      // área útil, descontadas as margens
    int topSeconds() const;       // teto da escala vertical
    int indexAt(const QPointF& pos) const;  // coluna sob o cursor (-1 fora)

    QDate m_monday;
    QList<int> m_seconds;
    int m_hoverIndex = -1;
};
