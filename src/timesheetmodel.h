#pragma once

#include <QAbstractTableModel>
#include <QDate>

class Storage;
struct DayRecord;

// Modelo (padrão model/view do Qt) que expõe um mês do Storage como tabela:
// uma linha por dia, uma coluna por informação. A QTableView pergunta tudo
// via data()/headerData() — o modelo não guarda dados, só traduz o Storage.
class TimesheetModel : public QAbstractTableModel {
    Q_OBJECT

public:
    enum Column { Day, In, Out, Breaks, Worked, Tasks, ColumnCount };

    explicit TimesheetModel(Storage* storage, QObject* parent = nullptr);

    void setMonth(const QDate& firstDay);
    QDate month() const { return m_month; }
    int monthTotalSeconds() const;
    QDate dateForRow(int row) const;

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

private:
    QString cellText(const DayRecord* day, const QDate& date, Column column) const;

    Storage* m_storage;
    QDate m_month;  // sempre o dia 1 do mês exibido
};
