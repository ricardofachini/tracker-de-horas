#include "csvexport.h"

#include "storage.h"
#include "widgets.h"

#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLocale>
#include <QPushButton>
#include <QStandardPaths>
#include <QVBoxLayout>

namespace {

// Aspas conforme RFC 4180 (adaptado ao separador ';' usado em pt-BR).
QString csvField(QString value) {
    if (value.contains(QLatin1Char(';')) || value.contains(QLatin1Char('"'))
        || value.contains(QLatin1Char('\n'))) {
        value.replace(QLatin1Char('"'), QLatin1String("\"\""));
        return QLatin1Char('"') + value + QLatin1Char('"');
    }
    return value;
}

// "Escrever relatório (1h 05min) ✓"
QString taskSummary(const Task& task, const QTime& now) {
    QString text = task.text;
    const int spent = task.spentSeconds(now);
    if (spent > 0)
        text += QStringLiteral(" (%1)").arg(formatDuration(spent));
    if (task.done)
        text += QStringLiteral(" ✓");
    return text;
}

// Aviso simples no estilo do app (sucesso ou erro da exportação).
void infoDialog(QWidget* parent, const QString& title, const QString& detail) {
    QDialog dialog(parent);
    dialog.setWindowTitle(title);
    dialog.setModal(true);
    dialog.setMinimumWidth(380);

    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(24, 22, 24, 22);
    layout->setSpacing(8);
    layout->addWidget(makeLabel(title, "h2"));
    auto* detailLabel = makeLabel(detail, "muted");
    detailLabel->setWordWrap(true);
    layout->addWidget(detailLabel);
    layout->addSpacing(12);

    auto* buttons = new QHBoxLayout;
    buttons->addStretch();
    auto* ok = new QPushButton(QStringLiteral("OK"));
    ok->setProperty("kind", "primary");
    ok->setCursor(Qt::PointingHandCursor);
    ok->setDefault(true);
    buttons->addWidget(ok);
    layout->addLayout(buttons);

    QObject::connect(ok, &QPushButton::clicked, &dialog, &QDialog::accept);
    dialog.exec();
}

}  // namespace

QString monthCsv(const Storage& storage, const QDate& month) {
    const QDate first(month.year(), month.month(), 1);
    const QDate today = QDate::currentDate();
    const QLocale locale;

    QString out = QStringLiteral("Data;Dia;Entrada;Saída;Pausas;Trabalhadas;Tarefas\r\n");
    int totalSeconds = 0;
    for (int i = 0; i < first.daysInMonth(); ++i) {
        const QDate date = first.addDays(i);
        const DayRecord* day = storage.find(date);
        // Como na Folha: o dia de hoje ainda aberto conta até a hora atual.
        const QTime now = date == today ? QTime::currentTime() : QTime();

        const QTime in = day ? day->firstIn() : QTime();
        const QTime lastOut = day ? day->lastOut() : QTime();
        const int breaks = day ? day->breakSeconds(now) : 0;
        const int worked = day ? day->workedSeconds(now) : 0;
        totalSeconds += worked;

        QStringList tasks;
        if (day)
            for (const Task& task : day->tasks)
                tasks.append(taskSummary(task, now));

        QStringList fields;
        fields << date.toString(QStringLiteral("dd/MM/yyyy"))
               << locale.dayName(date.dayOfWeek(), QLocale::LongFormat)
               << (in.isValid() ? in.toString(QStringLiteral("HH:mm")) : QString())
               << (lastOut.isValid() ? lastOut.toString(QStringLiteral("HH:mm")) : QString())
               << (breaks > 0 ? formatDuration(breaks, true) : QString())
               << (worked > 0 ? formatDuration(worked, true) : QString())
               << tasks.join(QStringLiteral(" | "));
        for (QString& field : fields)
            field = csvField(field);
        out += fields.join(QLatin1Char(';')) + QStringLiteral("\r\n");
    }
    out += QStringLiteral("Total do mês;;;;;%1;\r\n").arg(formatDuration(totalSeconds, true));
    return out;
}

bool exportMonthCsv(QWidget* parent, const Storage& storage, const QDate& month) {
    const QString suggested = QStringLiteral("%1/folha-ponto-%2.csv")
        .arg(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
             month.toString(QStringLiteral("yyyy-MM")));
    QString path = QFileDialog::getSaveFileName(parent,
                                                QStringLiteral("Exportar folha de ponto"),
                                                suggested,
                                                QStringLiteral("Planilha CSV (*.csv)"));
    if (path.isEmpty())  // usuário cancelou
        return false;
    if (!path.endsWith(QLatin1String(".csv"), Qt::CaseInsensitive))
        path += QLatin1String(".csv");

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        infoDialog(parent, QStringLiteral("Não foi possível exportar"),
                   QStringLiteral("Erro ao gravar \"%1\": %2")
                       .arg(QDir::toNativeSeparators(path), file.errorString()));
        return false;
    }
    file.write("\xEF\xBB\xBF");  // BOM: faz o Excel reconhecer UTF-8
    file.write(monthCsv(storage, month).toUtf8());
    file.close();

    infoDialog(parent, QStringLiteral("Folha exportada"),
               QStringLiteral("Arquivo salvo em %1.").arg(QDir::toNativeSeparators(path)));
    return true;
}
