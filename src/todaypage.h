#pragma once

#include "model.h"
#include <QWidget>

class Storage;
class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QProgressBar;
class QPushButton;
class QTimer;

// Página principal: relógio de ponto ao vivo + tarefas do dia.
class TodayPage : public QWidget {
public:
    explicit TodayPage(Storage* storage, QWidget* parent = nullptr);

private:
    DayRecord& today() const;
    void punch(PunchType type);
    void addTask();
    void toggleTask(QListWidgetItem* item);
    void clearDoneTasks();
    void refresh();  // reconstrói listas, botões e status
    void tick();     // atualiza relógio e contador a cada segundo

    Storage* m_storage;
    QDate m_shownDate;
    bool m_updating = false;

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
};
