#pragma once

#include <QWidget>

class Storage;
class TimesheetModel;
class QLabel;
class QPushButton;

// Página "Folha": planilha do mês com entrada, saída e horas por dia.
class TimesheetPage : public QWidget {
public:
    explicit TimesheetPage(Storage* storage, QWidget* parent = nullptr);

    void refresh();  // re-lê o mês exibido (chamado ao entrar na página)

private:
    void setMonth(const QDate& firstDay);

    TimesheetModel* m_model;
    QLabel* m_monthLabel;
    QLabel* m_totalLabel;
    QPushButton* m_nextButton;
};
