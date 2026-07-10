#pragma once

#include <QDate>
#include <QWidget>

class Storage;
class WeekChart;
class QLabel;
class QPushButton;
class QVBoxLayout;

// Lista somente-leitura dos dias já registrados.
class HistoryPage : public QWidget {
public:
    explicit HistoryPage(Storage* storage, QWidget* parent = nullptr);
    void refresh();

private:
    Storage* m_storage;
    QVBoxLayout* m_list;
};

// Resumo de horas (hoje, semana, mês) + gráfico de horas por semana.
class ReportsPage : public QWidget {
public:
    explicit ReportsPage(Storage* storage, QWidget* parent = nullptr);
    void refresh();

private:
    void setWeek(const QDate& monday);  // alimenta o gráfico e o rótulo

    Storage* m_storage;
    QLabel* m_todayValue;
    QLabel* m_weekValue;
    QLabel* m_monthValue;
    QLabel* m_avgValue;
    QDate m_weekStart;  // segunda-feira da semana exibida no gráfico
    WeekChart* m_chart;
    QLabel* m_weekLabel;
    QPushButton* m_nextWeekButton;
};
