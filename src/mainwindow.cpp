#include "mainwindow.h"

#include "pages.h"
#include "todaypage.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

MainWindow::MainWindow() {
    setWindowTitle(QStringLiteral("Tracker Horas"));
    resize(1060, 700);
    setMinimumSize(920, 620);

    m_todayPage = new TodayPage(&m_storage);
    m_historyPage = new HistoryPage(&m_storage);
    m_reportsPage = new ReportsPage(&m_storage);

    m_stack = new QStackedWidget;
    m_stack->addWidget(m_todayPage);
    m_stack->addWidget(m_historyPage);
    m_stack->addWidget(m_reportsPage);

    auto* sidebar = new QFrame;
    sidebar->setObjectName("sidebar");
    sidebar->setFixedWidth(232);
    auto* sideLayout = new QVBoxLayout(sidebar);
    sideLayout->setContentsMargins(16, 22, 16, 16);
    sideLayout->setSpacing(4);

    auto* appTitle = new QLabel(QStringLiteral("Tracker Horas"));
    appTitle->setObjectName("appTitle");
    auto* appSubtitle = new QLabel(QStringLiteral("controle de ponto e tarefas"));
    appSubtitle->setObjectName("appSubtitle");
    sideLayout->addWidget(appTitle);
    sideLayout->addWidget(appSubtitle);
    sideLayout->addSpacing(24);

    auto addNav = [this, sideLayout](const QString& text, int index) {
        auto* button = new QPushButton(text);
        button->setProperty("nav", true);
        button->setCheckable(true);
        button->setAutoExclusive(true);
        button->setCursor(Qt::PointingHandCursor);
        connect(button, &QPushButton::clicked, this, [this, index] {
            // Páginas de resumo são recalculadas ao entrar nelas.
            if (index == 1)
                m_historyPage->refresh();
            else if (index == 2)
                m_reportsPage->refresh();
            m_stack->setCurrentIndex(index);
        });
        sideLayout->addWidget(button);
        return button;
    };
    addNav(QStringLiteral("Hoje"), 0)->setChecked(true);
    addNav(QStringLiteral("Histórico"), 1);
    addNav(QStringLiteral("Relatórios"), 2);

    sideLayout->addStretch();
    auto* footer = new QLabel(QStringLiteral("v0.1 · dados salvos localmente"));
    footer->setObjectName("appSubtitle");
    footer->setWordWrap(true);
    sideLayout->addWidget(footer);

    auto* central = new QWidget;
    auto* layout = new QHBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(sidebar);
    layout->addWidget(m_stack, 1);
    setCentralWidget(central);
}
