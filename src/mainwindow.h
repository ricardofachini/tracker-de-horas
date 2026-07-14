#pragma once

#include "icons.h"
#include "storage.h"

#include <QList>
#include <QMainWindow>
#include <QPair>

class BankPage;
class HistoryPage;
class ReportsPage;
class TimesheetPage;
class TodayPage;
class QPushButton;
class QStackedWidget;

class MainWindow : public QMainWindow {
public:
    MainWindow();

private:
    void refreshThemeIcons();  // recria ícones (as cores vêm do tema ativo)

    Storage m_storage;
    QStackedWidget* m_stack;
    TodayPage* m_todayPage;
    TimesheetPage* m_sheetPage;
    BankPage* m_bankPage;
    HistoryPage* m_historyPage;
    ReportsPage* m_reportsPage;
    QList<QPair<QPushButton*, NavGlyph>> m_navButtons;
    QPushButton* m_themeButton;
};
