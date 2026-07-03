#pragma once

#include <QWidget>

class Storage;
class QLabel;
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

// Resumo de horas (hoje, semana, mês). Gráficos ficam para depois.
class ReportsPage : public QWidget {
public:
    explicit ReportsPage(Storage* storage, QWidget* parent = nullptr);
    void refresh();

private:
    Storage* m_storage;
    QLabel* m_todayValue;
    QLabel* m_weekValue;
    QLabel* m_monthValue;
    QLabel* m_avgValue;
};
