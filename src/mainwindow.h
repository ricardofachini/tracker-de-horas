#pragma once

#include "storage.h"
#include <QMainWindow>

class HistoryPage;
class ReportsPage;
class TodayPage;
class QStackedWidget;

class MainWindow : public QMainWindow {
public:
    MainWindow();

private:
    Storage m_storage;
    QStackedWidget* m_stack;
    TodayPage* m_todayPage;
    HistoryPage* m_historyPage;
    ReportsPage* m_reportsPage;
};
