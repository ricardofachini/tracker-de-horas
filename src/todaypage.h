#pragma once

#include "model.h"
#include <QDateTime>
#include <QWidget>

class Storage;
class QLabel;
class QLineEdit;
class QListWidget;
class QProgressBar;
class QPushButton;

// Página principal: relógio de ponto ao vivo + tarefas do dia.
class TodayPage : public QWidget {
public:
    explicit TodayPage(Storage* storage, QWidget* parent = nullptr);

    // Reconstrói listas, botões e status. Público porque o MainWindow chama
    // após trocar o tema (ícones pintados) ou editar o dia de hoje pela Folha.
    void refresh();

private:
    DayRecord& today() const;
    void punch(PunchType type);
    void addTask();
    void setTaskDone(int index, bool done);
    void startTask(int index);  // preempção: pausa a tarefa em andamento
    void pauseTask(int index);
    void clearDoneTasks();
    void scheduleRefresh();  // adiado: seguro para handlers dentro das linhas
    void tick();             // atualiza relógio e contadores a cada segundo
    void handleDayChange(const QDateTime& now);  // virada de dia (meia-noite)

    Storage* m_storage;
    QDate m_shownDate;
    QDateTime m_lastTick;  // distingue meia-noite observada de retomada (suspend)

    QLabel* m_dateLabel;
    QLabel* m_clockLabel;
    QLabel* m_statusPill;
    QLabel* m_workedLabel;
    QProgressBar* m_journeyBar;
    QLabel* m_journeyCaption;
    bool m_journeyComplete = false;
    QPushButton* m_btnIn;
    QPushButton* m_btnBreak;
    QPushButton* m_btnResume;
    QPushButton* m_btnOut;
    QListWidget* m_punchList;
    QLabel* m_punchEmpty;
    QLineEdit* m_taskInput;
    QListWidget* m_taskList;
    QLabel* m_taskEmpty;
    QPushButton* m_btnClearDone;
    QLabel* m_runningTimeLabel = nullptr;  // tempo da tarefa em andamento
};
