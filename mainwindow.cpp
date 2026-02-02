/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version. See COPYING file for more details.
 *
 */

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "editwindow.h"
#include "settingswindow.h"
#include "licensewindow.h"
#include "aboutwindow.h"

//#include <QTextCodec>
#include <QDate>
#include <QDebug>
#include <QFileInfo>
#include <QColorDialog>
#include <QFontDialog>
#include <QSettings>
#include <QScreen>
#include <QFont>
#include <QDesktopServices>
#include <QProcess>
#include <QMessageBox>
#include <QIcon>
#include <QScrollBar>
#include <QPainter>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    rndRuns(QRandomGenerator::global()->generate()),
    gSettings("TheirBirthdaySoft","TheirBirthday"),
    pathMan(),
    regexpDt("^([0-9]{2})/([0-9]{2})/?([0-9]{0,4})")
{
    ui->setupUi(this);
    lastDate = QDate::currentDate(); //QDateTime::fromSecsSinceEpoch(0);
    connect(ui->actionExit, SIGNAL(triggered()), this, SLOT(close()));
    //задаём кастомное контекстное меню в полуокнах
    ui->plainTEditEvents->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->plainTEditEvents, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showContextMenuEvents(const QPoint&)));
    ui->plainTEditDates->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->plainTEditDates, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showContextMenuDates(const QPoint&)));
    ui->plainTEditRuns->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->plainTEditRuns, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showContextMenuRuns(const QPoint&)));
    ui->plainTEditRuns->setLineWrapMode(QPlainTextEdit::NoWrap);
    QAction *toggleToolbarAction = ui->mainToolBar->toggleViewAction();
    toggleToolbarAction->setText(tr("Tool Bar"));
    //QAction *toggleMenuBarAction = new QAction(tr("Menu Bar"), this);
    //ui->mainToolBar->addAction(toggleMenuBarAction);
    //connect(toggleMenuBarAction, &QAction::triggered, this, &MainWindow::on_actionMenuBar_triggered);
    //ui->menuView->addAction(toggleToolbarAction);
    //connect(toggleToolbarAction, &QAction::triggered, this, &MainWindow::on_actionToolBar_triggered);
    connect(ui->mainToolBar, &QToolBar::visibilityChanged, this, &MainWindow::handleToolBarVisibilityChange);
    gMenuBar = gSettings.value("/MenuBar", false).toBool();
    if (!gMenuBar) {
        ui->menuBar->hide();
        ui->actionMenuBar->setChecked(false);
    }
    gToolBar = gSettings.value("/ToolBar", false).toBool();
    if (!gToolBar) ui->mainToolBar->hide();
    ui->centralWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->centralWidget, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(onHeaderContextMenu(const QPoint&)));
    //обрабатываем иконку трея
    trayIcon = new QSystemTrayIcon(this);
    gTrayIconDate = gSettings.value("/TrayIconDate", true).toBool();
    dateIcon = QIcon(":/new/files/tray.ico");
    if (!gTrayIconDate) {
        QIcon bIcon = QIcon(":/new/files/TheirBirthday.ico");
        trayIcon->setIcon(bIcon);
    }
    trayIcon->setToolTip("TheirBirthday");
    //контекстное меню трея
    QMenu * menu = new QMenu(this);
    QAction * viewWindow = new QAction(tr("Развернуть окно"), this);
    QAction * quitAction = new QAction(tr("Выход"), this);
    quitAction->setIcon(ui->actionExit->icon());

    connect(viewWindow, SIGNAL(triggered()), this, SLOT(show()));
    connect(quitAction, SIGNAL(triggered()), this, SLOT(on_actionExit_triggered()));//SLOT(close()));

    menu->addAction(viewWindow);
    menu->addAction(quitAction);

    trayIcon->setContextMenu(menu);
    trayIcon->show();

    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));

    //цвет выделения "сегодняшнего" текста
    //gColorTodayText = QColor(Qt::green).lighter(125);
    gColorTodayText = QColor(Qt::darkRed);
    //цвет для текста остальных дней
    //gColorOtherText = QColor(Qt::blue).lighter(175);
    gColorOtherText = QColor(Qt::black);
    //Напоминать за 14 дней по умолчанию
    gDays = gSettings.value("/Days", 14).toInt();
    //Разделитель для отображения
    gDelimiter = gSettings.value("/Delimiter", "/").toString();
    //Сворачивать в трей
    gTray = gSettings.value("/Tray", false).toBool();
    if(gTray) this->setWindowFlags(windowFlags() | Qt::Tool);
    //мало ли чего там с сеттингов считалось...
    if (gDays < 1 || gDays > 364) gDays = 14;
    if (gDelimiter.length() != 1) gDelimiter = "/";
    // Прочтены ли файлы
    if (!pathMan.ok())
    {
        QMessageBox::critical(this, tr("Ошибка"), pathMan.errString());
        return;
    }
    //снимаем комменты с прошедших событий
    unCommentEvents();
    //запоминаем наши даты и события
    setLstEvents();
    setLstDates();
    setLstRuns();

    setWindowFont();
    setGColor();
    setWindowSize();

    refreshWindows();
    refreshTitle();
    if (!qlRuns.isEmpty())
        refreshRuns();
    startTimer(30000);
}

void MainWindow::onHeaderContextMenu(const QPoint &pos)
{
    if (!gMenuBar) {
        QMenu menu(this);
        menu.addAction(ui->menuFile->menuAction());
        menu.addAction(ui->menuView->menuAction());
        menu.addAction(ui->menuHelp->menuAction());
        // Connect actions to your specific slots

        // Determine which section was clicked (optional)
        auto *header = qobject_cast<QWidget*>(sender());
        if (header) {
            //int sectionIndex = header->logicalIndexAt(pos);
            // Use the sectionIndex to customize the menu for a specific column/row

            // Show the menu at the global position of the event
            menu.exec(header->mapToGlobal(pos));
        }
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::showEvent(QShowEvent* event) {
    QMainWindow::showEvent(event);
    if (event->type() == QEvent::Show)
        scrollToStartEdit();//QTimer::singleShot(100, this, &MainWindow::scrollToStartEditRuns);
}

void MainWindow::scrollToStartEdit() {
    //qDebug() << "scrollToStartEdit";
    ui->plainTEditDates->verticalScrollBar()->setValue(0);
    ui->plainTEditEvents->verticalScrollBar()->setValue(0);
    ui->plainTEditRuns->horizontalScrollBar()->setValue(0);
}

void MainWindow::setWindowFont()
{
    //считываем данные шрифта
    QString fntFamily = gSettings.value("/Font", "Arial").toString();
    int fntSize = gSettings.value("/FontSize", 8).toInt();
    bool fntItalic = gSettings.value("/FontItalic", false).toBool();
    int fntBold = gSettings.value("/FontBold", -1).toInt();

    //устанавливаем шрифт в окнах
    QFont fnt(fntFamily, fntSize, fntBold, fntItalic);
    ui->plainTEditDates->setFont(fnt);
    ui->plainTEditEvents->setFont(fnt);
    ui->plainTEditRuns->setFont(fnt);
}

void MainWindow::setGColor()
{
    //считываем данные цвета
    int r = gSettings.value("/Red", 0).toInt();
    int g = gSettings.value("/Green", 0).toInt();
    int b = gSettings.value("/Blue", 0).toInt();

    //устанавливаем цвет выделения
    if (r != 0 || g != 0 || b != 0)
    {
        gColorTodayText = QColor::fromRgb(r, g, b);
        gsColorTodayText = "#" + (r==0?"00":QString::number( r, 16 )) + (g==0?"00":QString::number( g, 16 ))+ (b==0?"00":QString::number( b, 16 ));
    }

    int r3 = gSettings.value("/Red3", 0).toInt();
    int g3 = gSettings.value("/Green3", 0).toInt();
    int b3 = gSettings.value("/Blue3", 0).toInt();

    //устанавливаем цвет выделения
    if (r3 != 0 || g3 != 0 || b3 != 0)
    {
        gColorOtherText = QColor::fromRgb(r3, g3, b3);
        gsColorOtherText = "#" + (r3==0?"00":QString::number( r3, 16 )) + (g3==0?"00":QString::number( g3, 16 ))+ (b3==0?"00":QString::number( b3, 16 ));
    }
}

void MainWindow::setWindowSize()
{
    //считываем размеры окна
    int frmWidth = gSettings.value("/Width", 578).toInt();
    int frmHeight = gSettings.value("/Height", 363).toInt();

    //определяем размеры экрана
    QScreen* screen = QApplication::screens().at(0);
    QSize size = screen->availableSize();

    int iX = size.width()/2 - frmWidth/2;
    int iY = size.height()/2 - frmHeight/2;
    //проверяем на допустимость значений
    if (iX < 0) iX = 0;
    if (iY < 0) iY = 0;
    //устанавливаем размеры окна, в центре экрана
    this->setGeometry(iX , iY, frmWidth, frmHeight);
}
//заполняем полуокно по переданному имени файла
void MainWindow::setLst(const QString& path, const PathManager::FileType &type)
{
    //QRegExp regexpDt("^[0-9]{2}/[0-9]{2}/?[0-9]{0,4}");
    QRegExp regexpDOW0("^[А-Я][а-я][0-9]");
    QRegExp regexpDOW("^[А-Я][а-я][0-9]/[0-9]{2}");
    QFile fl(path);
    if (!QFile::exists(path))
    {
        fl.open(QIODevice::WriteOnly | QIODevice::Append);
        fl.close();
    }

    if (fl.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        //QTextCodec *codec = QTextCodec::codecForName("CP1251");
        while(!fl.atEnd())
        {
            //QByteArray sBStr = fl.readLine();
            QString sStr = fl.readLine();//codec->toUnicode(sBStr);
            if (type == PathManager::FileType::Runs)
            {
                if (!sStr.startsWith(";"))
                    qlRuns << sStr;
            } else if (sStr.contains(regexpDt) || sStr.contains(regexpDOW) || sStr.contains(regexpDOW0))
            {
                if (type == PathManager::FileType::Events) //path.endsWith("events.txt"))
                    qlEvents << sStr;
                else qlDates << sStr;
            }
        }
        fl.close();
    }
}
//Заполняем Даты (верхнее окно)
void MainWindow::setLstDates()
{
    setLst(pathMan.datesFilePath(), PathManager::FileType::Dates);
}
//Заполняем События (среднеее окно)
void MainWindow::setLstEvents()
{
    setLst(pathMan.eventsFilePath(), PathManager::FileType::Events);
}
//Заполняем Афоризмы (нижнее окно)
void MainWindow::setLstRuns()
{
    setLst(pathMan.runsFilePath(), PathManager::FileType::Runs);
}
void MainWindow::getCurrentLStrPrefix(QStringList &slb, const QList<QString> &pql, const QDate &currentDate, QString prefix)
{
    //QRegExp regexpDt("^([0-9]{2})/([0-9]{2})/?([0-9]{0,4})");
    foreach(QString fs, pql)
    {
        //QString sDate = fs.left(10);
        //QDate dDate = QDate::fromString(sDate, "dd/MM/yyyy");
        auto matchDate = regexpDt.match(fs);
        if (matchDate.hasMatch() &&
            matchDate.captured(1).toInt() == currentDate.day() &&
            matchDate.captured(2).toInt() == currentDate.month()) {
            QString st = "";
            int iy = matchDate.captured(3).isEmpty() ? 0 : matchDate.captured(3).toInt();
            if (iy > 0) {
                iy = currentDate.year() - iy;
                st = prefix + fs.mid(matchDate.captured(0).length()).trimmed() + " (" + QString::number(iy) + tr(" годовщина") + ")";
            } else
                st = prefix + fs.mid(matchDate.captured(0).length()).trimmed();
            slb.append(st);// + "\n";
        }
    }
}
//формируем строки "Вчера"
QStringList MainWindow::getResultYesterdayLStr(QList<QString> pql, bool isEvent)
{
    QStringList slb;
    QRegExp regexpDigit("[0-9][0-9]");
    int gdowom = getDayOfWeekOfMonth(-1);
    QDate currentDate = QDate::currentDate();
    QString eventPrefix = isEvent ? tr("поздравили ") : "";

    getCurrentLStrPrefix(slb, pql, currentDate.addDays(-1), tr("Вчера ") + eventPrefix);
    //обрабатываем дни недели: строки вида Вс1/11 - первое воскресенье ноября
    foreach(QString fs, pql)
    {
        QString sDayOfWeekOfMonth = fs.left(6);
        int dowom = sDayOfWeekOfMonth.mid(2,1).toInt();

        if (dowom == 0) continue;
        if (dowom != gdowom) continue;
        if (sDayOfWeekOfMonth.mid(3,1) != "/") continue;
        QString sMonthDigits = sDayOfWeekOfMonth.right(2);
        if (!sMonthDigits.contains(regexpDigit)) continue;
        int mnth = sMonthDigits.toInt();
        if (mnth != 0 && mnth != currentDate.month()) continue;

        QString sDayOfWeek = fs.left(2);
        bool append = false;
        if (sDayOfWeek == tr("Пн") && currentDate.dayOfWeek() == 2)
            append = true;
        else if (sDayOfWeek == tr("Вт") && currentDate.dayOfWeek() == 3)
            append = true;
        else if (sDayOfWeek == tr("Ср") && currentDate.dayOfWeek() == 4)
            append = true;
        else if (sDayOfWeek == tr("Чт") && currentDate.dayOfWeek() == 5)
            append = true;
        else if (sDayOfWeek == tr("Пт") && currentDate.dayOfWeek() == 6)
            append = true;
        else if (sDayOfWeek == tr("Сб") && currentDate.dayOfWeek() == 7)
            append = true;
        else if (sDayOfWeek == tr("Вс") && currentDate.dayOfWeek() == 1)
            append = true;
        if (append)
            slb.append(tr("Вчера ") + eventPrefix + fs.replace(sDayOfWeekOfMonth, "").trimmed());// + "\n";
    }
    //обрабатываем дни недели: строки вида Пн0, Вт0, Ср0, Чт0, Пт0, Сб0, Вс0
    foreach(QString fs, pql)
    {
        QString sDayOfWeek = fs.left(3);
        bool append = false;
        if (sDayOfWeek == tr("Пн0") && currentDate.dayOfWeek() == 2)
            append = true;
        else if (sDayOfWeek == tr("Вт0") && currentDate.dayOfWeek() == 3)
            append = true;
        else if (sDayOfWeek == tr("Ср0") && currentDate.dayOfWeek() == 4)
            append = true;
        else if (sDayOfWeek == tr("Чт0") && currentDate.dayOfWeek() == 5)
            append = true;
        else if (sDayOfWeek == tr("Пт0") && currentDate.dayOfWeek() == 6)
            append = true;
        else if (sDayOfWeek == tr("Сб0") && currentDate.dayOfWeek() == 7)
            append = true;
        else if (sDayOfWeek == tr("Вс0") && currentDate.dayOfWeek() == 1)
            append = true;
        if (append)
            slb.append(tr("Вчера ") + eventPrefix + fs.replace(sDayOfWeek, "").trimmed());// + "\n";
    }
    //if (!sb.isEmpty()) return sb.left(sb.length() -1);
    return slb;
}
//формируем строки "Сегодня"
QStringList MainWindow::getResultTodayLStr(QList<QString> pql, bool isEvent)
{
    QStringList slb;
    QRegExp regexpDigit("[0-9][0-9]");
    int gdowom = getDayOfWeekOfMonth(0);
    QDate currentDate = QDate::currentDate();
    QString eventPrefix = isEvent ? tr("поздравляем ") : "";

    getCurrentLStrPrefix(slb, pql, currentDate, tr("Сегодня ") + eventPrefix);
    //обрабатываем дни недели: строки вида Вс1/11
    foreach(QString fs, pql)
    {
        QString sDayOfWeekOfMonth = fs.left(6);
        int dowom = sDayOfWeekOfMonth.mid(2,1).toInt();

        if (dowom == 0) continue;
        if (dowom != gdowom) continue;
        if (sDayOfWeekOfMonth.mid(3,1) != "/") continue;
        QString sMonthDigits = sDayOfWeekOfMonth.right(2);
        if (!sMonthDigits.contains(regexpDigit)) continue;
        int mnth = sMonthDigits.toInt();
        if (mnth != 0 && mnth != currentDate.month()) continue;

        QString sDayOfWeek = fs.left(2);
        bool append = false;
        if (sDayOfWeek == tr("Пн") && currentDate.dayOfWeek() == 1)
            append = true;
        else if (sDayOfWeek == tr("Вт") && currentDate.dayOfWeek() == 2)
            append = true;
        else if (sDayOfWeek == tr("Ср") && currentDate.dayOfWeek() == 3)
            append = true;
        else if (sDayOfWeek == tr("Чт") && currentDate.dayOfWeek() == 4)
            append = true;
        else if (sDayOfWeek == tr("Пт") && currentDate.dayOfWeek() == 5)
            append = true;
        else if (sDayOfWeek == tr("Сб") && currentDate.dayOfWeek() == 6)
            append = true;
        else if (sDayOfWeek == tr("Вс") && currentDate.dayOfWeek() == 7)
            append = true;
        if (append)
            slb.append(tr("Сегодня ") + eventPrefix + fs.replace(sDayOfWeekOfMonth, "").trimmed());// + "\n";
    }
    //обрабатываем дни недели: строки вида Пн0, Вт0, Ср0, Чт0, Пт0, Сб0, Вс0
    foreach(QString fs, pql)
    {
        QString sDayOfWeek = fs.left(3);
        bool append = false;
        if (sDayOfWeek == tr("Пн0") && currentDate.dayOfWeek() == 1)
            append = true;
        else if (sDayOfWeek == tr("Вт0") && currentDate.dayOfWeek() == 2)
            append = true;
        else if (sDayOfWeek == tr("Ср0") && currentDate.dayOfWeek() == 3)
            append = true;
        else if (sDayOfWeek == tr("Чт0") && currentDate.dayOfWeek() == 4)
            append = true;
        else if (sDayOfWeek == tr("Пт0") && currentDate.dayOfWeek() == 5)
            append = true;
        else if (sDayOfWeek == tr("Сб0") && currentDate.dayOfWeek() == 6)
            append = true;
        else if (sDayOfWeek == tr("Вс0") && currentDate.dayOfWeek() == 7)
            append = true;
        if (append)
            slb.append(tr("Сегодня ") + eventPrefix + fs.replace(sDayOfWeek, "").trimmed());// + "\n";
    }

    //if (sb != "") return sb.left(sb.length() -1);
    return slb;
}
//формируем строки "Завтра"
QStringList MainWindow::getResultTomorrowLStr(QList<QString> pql, bool isEvent)
{
    QStringList slb;
    QRegExp regexpDigit("[0-9][0-9]");
    int gdowom = getDayOfWeekOfMonth(1);
    QDate currentDate = QDate::currentDate();
    QString eventPrefix = isEvent ? tr("поздравим ") : "";

    getCurrentLStrPrefix(slb, pql, currentDate.addDays(1), tr("Завтра ") + eventPrefix);

    foreach(QString fs, pql)
    {
        QString sDayOfWeekOfMonth = fs.left(6);
        int dowom = sDayOfWeekOfMonth.mid(2,1).toInt();

        if (dowom == 0) continue;
        if (dowom != gdowom) continue;
        if (sDayOfWeekOfMonth.mid(3,1) != "/") continue;
        QString sMonthDigits = sDayOfWeekOfMonth.right(2);
        if (!sMonthDigits.contains(regexpDigit)) continue;
        int mnth = sMonthDigits.toInt();
        if (mnth != 0 && mnth != currentDate.month()) continue;

        QString sDayOfWeek = fs.left(2);
        bool append = false;
        if (sDayOfWeek == tr("Пн") && currentDate.dayOfWeek() == 7)
            append = true;
        else if (sDayOfWeek == tr("Вт") && currentDate.dayOfWeek() == 1)
            append = true;
        else if (sDayOfWeek == tr("Ср") && currentDate.dayOfWeek() == 2)
            append = true;
        else if (sDayOfWeek == tr("Чт") && currentDate.dayOfWeek() == 3)
            append = true;
        else if (sDayOfWeek == tr("Пт") && currentDate.dayOfWeek() == 4)
            append = true;
        else if (sDayOfWeek == tr("Сб") && currentDate.dayOfWeek() == 5)
            append = true;
        else if (sDayOfWeek == tr("Вс") && currentDate.dayOfWeek() == 6)
            append = true;
        if (append)
            slb.append(tr("Завтра ") + eventPrefix + fs.replace(sDayOfWeekOfMonth, "").trimmed());// + "\n";
    }
    //обрабатываем дни недели: строки вида Пн0, Вт0, Ср0, Чт0, Пт0, Сб0, Вс0
    foreach(QString fs, pql)
    {
        QString sDayOfWeek = fs.left(3);
        bool append = false;
        if (sDayOfWeek == tr("Пн0") && currentDate.dayOfWeek() == 7)
            append = true;
        else if (sDayOfWeek == tr("Вт0") && currentDate.dayOfWeek() == 1)
            append = true;
        else if (sDayOfWeek == tr("Ср0") && currentDate.dayOfWeek() == 2)
            append = true;
        else if (sDayOfWeek == tr("Чт0") && currentDate.dayOfWeek() == 3)
            append = true;
        else if (sDayOfWeek == tr("Пт0") && currentDate.dayOfWeek() == 4)
            append = true;
        else if (sDayOfWeek == tr("Сб0") && currentDate.dayOfWeek() == 5)
            append = true;
        else if (sDayOfWeek == tr("Вс0") && currentDate.dayOfWeek() == 6)
            append = true;
        if (append)
            slb.append(tr("Завтра ") + eventPrefix + fs.replace(sDayOfWeek, "").trimmed());// + "\n";
    }

    //if (sb != "") return sb.left(sb.length() -1);
    return slb;
}

//разбираем, "день", "дня" или "дней", в зависимости от количества pdays
QString MainWindow::getDaysStr(int pdays)
{
    int remains1 = pdays;
    if (pdays >= 100)
        remains1 = pdays % 100;//остаток от деления на 100

    if (remains1 >= 11 && remains1 <=14) return tr("дней");

    int remains2 = remains1;

    if (remains1 >= 10)
        remains2 = remains1 % 10;//остаток от деления на 10

    switch (remains2)
    {
        case 1: return tr("день");
        case 2: case 3: case 4: return tr("дня");
        default: return tr("дней");
    }
}
//формируем строки "Через N дней"
QStringList MainWindow::getResultLStr(QList<QString> pql, int pdays, bool isEvent)
{
    if (pdays == -1)
        return getResultYesterdayLStr(pql, isEvent);
    if (pdays == 0)
        return getResultTodayLStr(pql, isEvent);
    if (pdays == 1)
        return getResultTomorrowLStr(pql, isEvent);

    QStringList slb;
    QDate currentDate = QDate::currentDate();
    QString eventPrefix = isEvent ? tr("поздравим ") : "";
    foreach(QString fs, pql)
    {
        //QString sDate = fs.left(10);
        //QDate dDate = QDate::fromString(sDate, "dd/MM/yyyy");
        auto matchDate = regexpDt.match(fs);
        auto pDate = currentDate.addDays(pdays);
        if (matchDate.captured(1).toInt() == pDate.day() &&
            matchDate.captured(2).toInt() == pDate.month())
        {
            QString st = "";
            //QStringList slDayMonth = fs.left(5).split("/");
            QString sLocale = QLocale::system().name();
            int iy = matchDate.captured(3).isEmpty() ? 0 : matchDate.captured(3).toInt();
            if (iy > 0)
            {
                iy = currentDate.year() - iy;//dDate.year();
                if (sLocale == "en_US")
                {
                    //slb.append(tr("Через ") + QString::number(pdays) + tr(" дней (") + slDayMonth[1] + "/" + slDayMonth[0] + ") " + fs.mid(matchDate.captured(0).length()).trimmed() + " (" + QString::number(iy) + tr(" годовщина")+")\n";
                    st = tr("Через ") + QString::number(pdays) + tr(" дней (") + matchDate.captured(2) + gDelimiter + matchDate.captured(1) + ") " + eventPrefix + fs.mid(matchDate.captured(0).length()).trimmed() + " (" + QString::number(iy) + tr(" годовщина")+")";
                }
                else
                    //slb.append(tr("Через ") + QString::number(pdays) + " " + getDaysStr(pdays) + " (" + sDate.left(5).replace("/", ".") + ") " + fs.mid(matchDate.captured(0).length()).trimmed() + " (" + QString::number(iy) + tr(" годовщина")+")\n";
                    st = tr("Через ") + QString::number(pdays) + " " + getDaysStr(pdays) + " (" + fs.left(5).replace("/", gDelimiter) + ") " + eventPrefix + fs.mid(matchDate.captured(0).length()).trimmed() + " (" + QString::number(iy) + tr(" годовщина")+")";
                slb.append(st);// + "\n";
            }
            else
            {
                if (sLocale == "en_US")
                {
                    st = tr("Через ") + QString::number(pdays) + tr(" дней (") + matchDate.captured(2) + gDelimiter + matchDate.captured(1) + ") " + eventPrefix + fs.mid(matchDate.captured(0).length()).trimmed();
                }
                else
                    st = tr("Через ") + QString::number(pdays) + " " + getDaysStr(pdays) + " (" + fs.left(5).replace("/", gDelimiter) + ") " + eventPrefix + fs.mid(matchDate.captured(0).length()).trimmed();
                slb.append(st);// + "\n";
            }
            //заполняем список ql3
            //if (pdays == 2 || pdays == 3)
                //ql3.append(st);
        }
    }
    //if (sb != "") return sb.left(sb.length() -1);
    return slb;
}
//Подсвечиваем цветом сегодняшнее
/*void MainWindow::findTodayStrs(QPlainTextEdit *pte)
{
    pte->moveCursor(QTextCursor::Start);

    QTextCursor cur = pte->textCursor();
    QList<QTextEdit::ExtraSelection> lSel;

    QTextCursor findCur;
    //подсвечиваем сегодняшние
    foreach(QString fs, qlToday)
    {
        findCur = pte->document()->find(fs, cur);
        if(findCur != cur)
        {
            QTextEdit::ExtraSelection xtra;
            xtra.format.setBackground(gColorTodayText);
            xtra.cursor = findCur;
            lSel.append(xtra);
            pte->setExtraSelections(lSel);
        }
        cur = findCur;
    }
    //подсвечиваем на 3 дня вперёд
    foreach(QString fs, ql3)
    {
        findCur = pte->document()->find(fs, cur);
        if(findCur != cur)
        {
            QTextEdit::ExtraSelection xtra;
            xtra.format.setBackground(gColorOtherText);
            xtra.cursor = findCur;
            lSel.append(xtra);
            pte->setExtraSelections(lSel);
        }
        cur = findCur;
    }
}*/

//Обновляем главное окно
void MainWindow::refreshWindows()
{
    qDebug() << "refreshWindows ";
    ui->plainTEditEvents->setPlainText("");
    ui->plainTEditDates->setPlainText("");
    currentDatesCount = 0;
    currentEventsCount = 0;
    //QString sbEv = "", sbDt = "";

    //for(int i = -1; i < gDays; i++)
        //sbDt += getResultStr(qlDates, i);

    for(int i = -1; i < gDays; i++)
    {
        QStringList resDates = getResultLStr(qlDates, i);
        if (resDates.isEmpty()) continue;

        if (i == 0)
        {
            auto sresDates = resDates.join("</font></div><div><font color=\"" + gsColorTodayText + "\">");
            ui->plainTEditDates->appendHtml("<div><font color=\"" + gsColorTodayText + "\">" + sresDates + "</font></div>");
            currentDatesCount+=resDates.size();
        }
        else
        {
            auto sresDates = resDates.join("</font></div><div><font color=\"" + gsColorOtherText + "\">");
            ui->plainTEditDates->appendHtml("<div><font color=\"" + gsColorOtherText + "\">" + sresDates + "</font></div>");
        }
    }
    for(int i = -1; i < gDays; i++)
    {
        QStringList resEvents = getResultLStr(qlEvents, i, true);
        if (resEvents.isEmpty()) continue;

        if (i == 0)
        {
            auto sresEvents = resEvents.join("</font></div><div><font color=\"" + gsColorTodayText + "\">");
            ui->plainTEditEvents->appendHtml("<div><font color=\"" + gsColorTodayText + "\">" + sresEvents + "</font></div>");
            currentEventsCount+=resEvents.size();
        }
        else
        {
            auto sresEvents = resEvents.join("</font></div><div><font color=\"" + gsColorOtherText + "\">");
            ui->plainTEditEvents->appendHtml("<div><font color=\"" + gsColorOtherText + "\">" + sresEvents + "</font></div>");
        }
        //sbEv += getResultStr(qlEvents, i);
    }
    //qDebug() << "toolTip " << QString("Даты: %1, События: %2").arg(currentDatesCount).arg(currentEventsCount);
    trayIcon->setToolTip(QString("Даты: %1\nСобытия: %2").arg(currentDatesCount).arg(currentEventsCount));
    if (gTrayIconDate) {
        refreshTrayIcon();
    }
    //ui->plainTEditEvents->setPlainText(sbEv);
    //ui->plainTEditDates->setPlainText(sbDt);
    //подсвечиваем строки "сегодня"
    //findTodayStrs(ui->plainTEditEvents);
    //findTodayStrs(ui->plainTEditDates);
}
//Обновляем заголовок окна
QDateTime MainWindow::refreshTitle()
{
    QDateTime currentTime = QDateTime::currentDateTime();
    QString wtitle = QLocale::system().toString(currentTime, "dddd, d MMMM yyyy hh:mm");
    wtitle = wtitle.first(1).toUpper() + wtitle.mid(1);
    this->setWindowTitle(wtitle);
    return currentTime;
}

void MainWindow::refreshTrayIcon()
{
    QPixmap pixmap(24,24);
    //pixmap.fill(Qt::white); // заливаем пиксмап
    QPainter painter(&pixmap);
    //QPainter painter(this);
    QRect iconRect(0, 0, 24, 24); // Position and size for the icon
    // Draw the icon
    //QIcon nIcon(dateIcon);
    dateIcon.paint(&painter, iconRect); // Or use myIcon.pixmap() and painter->drawPixmap()
    painter.setRenderHint(QPainter::Antialiasing);

    // Prepare the date string
    //QDate today = QDate::currentDate();
    QString dateString = lastDate.toString("dd"); // Format the date

    // Set font and color for the text
    QFont font = painter.font();
    font.setPointSize(11); // Adjust size as needed
    painter.setFont(font);
    if (currentEventsCount == 0)
        painter.setPen(Qt::black);
    else painter.setPen(Qt::darkRed);
    // Draw the date text over or near the icon
    painter.drawText(iconRect.adjusted(0, 0, 0, 0), // Position text above icon
                     Qt::AlignCenter,
                     dateString);
    painter.end();
    QIcon nIcon(pixmap);
    trayIcon->setIcon(nIcon);
}

//Обновляем окно афоризмов
void MainWindow::refreshRuns(bool changeText)
{
    if (changeText) {
        qsizetype i = rndRuns.generate64() % qlRuns.size();
        QString snr = qlRuns.at(i);
        //удаляем из общего списка выбранный
        qlRuns.erase(qlRuns.begin() + i, qlRuns.begin() + i + 1);
        ui->plainTEditRuns->setPlainText("");
        ui->plainTEditRuns->appendHtml("<div><font color=\"" + gsColorOtherText + "\">" + snr + "</font></div>");
        //qDebug() << "qlRuns size " << qlRuns.size();
        if (qlRuns.isEmpty() /*&& !sr.isEmpty()*/) {
            //перезагружаем общий списк
            setLstRuns();
        }
    } else {
        QString sr = ui->plainTEditRuns->toPlainText();
        ui->plainTEditRuns->setPlainText("");
        ui->plainTEditRuns->appendHtml("<div><font color=\"" + gsColorOtherText + "\">" + sr + "</font></div>");
        //qDebug() << "refreshRuns " << sr;
    }
    //ui->plainTEditRuns->scroll(0,0);
    auto sb = ui->plainTEditRuns->horizontalScrollBar();
    sb->setValue(0);
}
//Выбор цвета
void MainWindow::on_actionColor_triggered()
{
    QColor tempColor = QColorDialog::getColor(gColorTodayText, this, tr("Color of Today dates"));
    if (tempColor.isValid())
    {
        gColorTodayText = tempColor;
        gSettings.setValue("/Red", tempColor.red());
        gSettings.setValue("/Green", tempColor.green());
        gSettings.setValue("/Blue", tempColor.blue());

        setGColor();

        refreshWindows();
    }
}
//Выбор шрифта
void MainWindow::on_actionFont_triggered()
{
    bool bOk;
    QFont fnt = QFontDialog::getFont(&bOk, this);
    if (bOk)
    {//Сохраняем данные
        gSettings.setValue("/Font", fnt.family());
        gSettings.setValue("/FontSize", fnt.pointSize());
        gSettings.setValue("/FontItalic", fnt.italic());
        gSettings.setValue("/FontBold", fnt.bold()?fnt.Bold:-1);//без таких ухищрений Bold не работает
        ui->plainTEditDates->setFont(fnt);
        ui->plainTEditEvents->setFont(fnt);
        ui->plainTEditRuns->setFont(fnt);
    }
}
//Обновляем окно каждую минуту
void MainWindow::timerEvent(QTimerEvent *)
{
    QDate currentDate = refreshTitle().date();
    //auto diff = currentTime.toMSecsSinceEpoch() - lastUpdate.toMSecsSinceEpoch();
    //if (diff > 60000) {
    if (currentDate.day() != lastDate.day()) {
        refreshWindows();
        lastDate = currentDate;
    }
    if (!qlRuns.isEmpty())
        refreshRuns();
}
//обрабатываем изменение размера окна
void MainWindow::resizeEvent(QResizeEvent *event)
{
    gSettings.setValue("/Width", event->size().width());
    gSettings.setValue("/Height", event->size().height());
}
//обрабатываем закрытие окна
void MainWindow::closeEvent(QCloseEvent *event)
{
    if(this->isVisible() && gTray)
    {
        event->ignore();
        this->hide();
    }
}
//суть редактирования для обоих файлов, и Dates, и Events
void MainWindow::callDatesEventsFile(QList<QString>& pLst, QString pFilePath)
{
    //ui->menuBar->hide();
    //disconnect(ui->plainTEditEvents, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showContextMenuEvents(const QPoint&)));
    //disconnect(ui->plainTEditDates, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showContextMenuDates(const QPoint&)));

    EditWindow *pew = new EditWindow(this, pFilePath);
    //if (gTray) pew->setWindowFlag(Qt::Tool, true);
    pew->exec();
    delete pew;

    //connect(ui->plainTEditDates, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showContextMenuDates(const QPoint&)));
    //connect(ui->plainTEditEvents, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showContextMenuEvents(const QPoint&)));
    //ui->menuBar->show();

    pLst.clear();
}
//Редактировать
void MainWindow::on_actionEdit_triggered()
{
    //QProcess prc;
    //prc.start("xdg-open", QStringList() << pathMan.eventsFilePath());
    //prc.waitForFinished();
    callDatesEventsFile(qlEvents, pathMan.eventsFilePath());

    setLstEvents();
    refreshWindows();
}

/*void MainWindow::on_actionToolBar_triggered()
{
    qDebug() << "actionToolBar_triggered";
    if (ui->mainToolBar->isVisible())
        ui->mainToolBar->hide();
    else
        ui->mainToolBar->show();
}*/

void MainWindow::on_actionExit_triggered()
{
    //this->close();
    QCoreApplication::quit();
}
//Выбор параметров "Напоминать за Х дней" и "Разделитель"
void MainWindow::on_actionSettings_triggered()
{
    SettingsWindow *psw = new SettingsWindow(this, gDays, gDelimiter, gTray, gTrayIconDate);
    //if (gTray) psw->setWindowFlag(Qt::Tool, true);
    if(psw->exec() == QDialog::Rejected)
    {
        delete psw;
        return;
    }

    gDays = psw->getDays();
    if (gDays < 1 || gDays > 364) gDays = 14;
    //Сохраняем наш параметр в глобальных настройках
    gSettings.setValue("/Days", gDays);

    gDelimiter = psw->getDelimiter();
    if (gDelimiter.length() != 1) gDelimiter = "/";
    gSettings.setValue("/Delimiter", gDelimiter);

    if(gTray != psw->getTray()) {
        gTray = psw->getTray();
        gSettings.setValue("/Tray", gTray);
        if(gTray)
          this->setWindowFlags(windowFlags() | Qt::Tool);
        else
          this->setWindowFlags(windowFlags() & ~Qt::Tool);
    }
    if(gTrayIconDate != psw->getTrayIconDate()) {
        gTrayIconDate = psw->getTray();
        gSettings.setValue("/TrayIconDate", gTrayIconDate);
        refreshTrayIcon();
    }
    delete psw;

    qlDates.clear();
    setLstDates();

    qlEvents.clear();
    setLstEvents();

    refreshWindows();
}

//пункт меню "О Qt"
void MainWindow::on_actionAbout_Qt_triggered()
{
    QMessageBox::aboutQt(this);
}
//Выводим нашу лицензию
void MainWindow::on_actionLicense_triggered()
{
    LicenseWindow *plw = new LicenseWindow(this);
    //if (gTray) plw->setWindowFlag(Qt::Tool, true);
    plw->exec();

    delete plw;
}
//Окно "О программе"
void MainWindow::on_actionAbout_triggered()
{
    AboutWindow * paw = new AboutWindow(this);
    //if (gTray) paw->setWindowFlag(Qt::Tool, true);
    paw->exec();

    delete paw;
}
//Выбираем цвет 3
void MainWindow::on_actionColor3_triggered()
{
    QColor tempColor = QColorDialog::getColor(gColorOtherText, this, tr("Color of Default dates"));
    if (tempColor.isValid())
    {
        gColorOtherText = tempColor;
        gSettings.setValue("/Red3", tempColor.red());
        gSettings.setValue("/Green3", tempColor.green());
        gSettings.setValue("/Blue3", tempColor.blue());

        setGColor();
        refreshWindows();
        refreshRuns(false);
    }
}
//Показываем кастомное контекстное меню для событий
void MainWindow::showContextMenuEvents(const QPoint& pos)
{
    QPoint globalPos = ui->plainTEditEvents->mapToGlobal(pos);
    QMenu myMenu;

    QAction* copyAction = new QAction(tr("Копировать"), this);
    QAction* selectAllAction = new QAction(tr("Выделить всё"), this);
    QAction* editAction = new QAction(tr("Редактировать..."), this);

    myMenu.addAction(copyAction);
    myMenu.addAction(selectAllAction);
    myMenu.addAction(editAction);

    connect(copyAction, &QAction::triggered, this, &MainWindow::showContextMenuEventsCopy);
    connect(selectAllAction, &QAction::triggered, this, &MainWindow::showContextMenuEventsSelectAll);
    connect(editAction, &QAction::triggered, this, &MainWindow::editContextMenuEvents);

    myMenu.exec(globalPos);
}
//Показываем кастомное контекстное меню для дат
void MainWindow::showContextMenuDates(const QPoint& pos)
{
    QPoint globalPos = ui->plainTEditDates->mapToGlobal(pos);
    QMenu myMenu;

    QAction* copyAction = new QAction(tr("Копировать"), this);
    QAction* selectAllAction = new QAction(tr("Выделить всё"), this);
    QAction* editAction  = new QAction(tr("Редактировать..."), this);

    myMenu.addAction(copyAction);
    myMenu.addAction(selectAllAction);
    myMenu.addAction(editAction);

    connect(copyAction, &QAction::triggered, this, &MainWindow::showContextMenuDatesCopy);
    connect(selectAllAction, &QAction::triggered, this, &MainWindow::showContextMenuDatesSelectAll);
    connect(editAction, &QAction::triggered, this, &MainWindow::editContextMenuDates);

    myMenu.exec(globalPos);
}
//Показываем кастомное контекстное меню для афоризмов
void MainWindow::showContextMenuRuns(const QPoint& pos)
{
    QPoint globalPos = ui->plainTEditRuns->mapToGlobal(pos);
    QMenu myMenu;

    QAction* copyAction = new QAction(tr("Копировать"), this);
    //QAction* selectAllAction = new QAction(tr("Выделить всё"), this);
    QAction* editAction  = new QAction(tr("Редактировать..."), this);

    myMenu.addAction(copyAction);
    //myMenu.addAction(selectAllAction);
    myMenu.addAction(editAction);

    connect(copyAction, &QAction::triggered, this, &MainWindow::showContextMenuRunsCopy);
    //connect(selectAllAction, &QAction::triggered, this, &MainWindow::showContextMenuRunsSelectAll);
    connect(editAction, &QAction::triggered, this, &MainWindow::editContextMenuRuns);

    myMenu.exec(globalPos);
}
//Обрабатываем редактирование событий
void MainWindow::editContextMenuEvents()
{
    on_actionEdit_triggered();
}
//Обрабатываем редактирование дат
void MainWindow::editContextMenuDates()
{
    callDatesEventsFile(qlDates, pathMan.datesFilePath());
    setLstDates();
    refreshWindows();
}
//Обрабатываем редактирование афоризмов
void MainWindow::editContextMenuRuns()
{
    callDatesEventsFile(qlRuns, pathMan.runsFilePath());
    setLstRuns();
    refreshWindows();
}
//пользовательские контекстные меню
void MainWindow::showContextMenuDatesCopy()
{
    ui->plainTEditDates->copy();
}

void MainWindow::showContextMenuDatesSelectAll()
{
    ui->plainTEditDates->selectAll();
}

void MainWindow::showContextMenuEventsCopy()
{
    ui->plainTEditEvents->copy();
}

void MainWindow::showContextMenuEventsSelectAll()
{
    ui->plainTEditEvents->selectAll();
}

void MainWindow::showContextMenuRunsCopy()
{
    ui->plainTEditRuns->selectAll();
    ui->plainTEditRuns->copy();
}
//обрабатываем нажатие на иконку в трее
void MainWindow::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
    //qDebug() << "iconActivated" << reason;
    switch (reason)
    {
    case QSystemTrayIcon::Trigger:
        //if(gTray)
        {
            if(!this->isVisible())
                this->show();
            else
                this->hide();
        }
        break;
    default:
        break;
    }
}
//снимаем комментарии с прошедших событий
void MainWindow::unCommentEvents()
{
    QRegExp regexpDtComm("^;([0-9]{2})/([0-9]{2})/?([0-9]{0,4})");

    QList<QString> qlEvents, qlStneve;

    QFile fl(pathMan.eventsFilePath());

    if (fl.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        //QTextCodec *codec = QTextCodec::codecForName("CP1251");
        while(!fl.atEnd())
        {
            //QByteArray sBStr = fl.readLine();
            QString sStr = fl.readLine();//codec->toUnicode(sBStr);

            qlEvents << sStr;
        }
        fl.close();
    }
    //определяем, есть ли вообще комментарий нужного нам характера
    bool commentExist = false;
    foreach(QString fs, qlEvents)
    {
        if (fs.contains(regexpDtComm))
        {
            commentExist = true;
            break;
        }
    }
    //если комментарий такой есть
    if (!commentExist) return;

    QDate currentDate = QDate::currentDate();
    foreach(QString fs, qlEvents)
    {
        auto matchDate = regexpDtComm.match(fs);
        if (!matchDate.hasMatch())
        {
            qlStneve << fs;
            continue;
        }
        bool isWritten = false;
        //смотрим в прошлое на 31 день
        for(int pdays = -31; pdays < 0; pdays++)
        {
            auto pDate = currentDate.addDays(pdays);
            //QString sDate = fs.left(11);
            //sDate = sDate.right(sDate.length()-1);//убираем ведущую ;
            //QDate dDate = QDate::fromString(sDate, "dd/MM/yyyy");
            if (matchDate.captured(1).toInt() == pDate.day() &&
                matchDate.captured(2).toInt() == pDate.month())
            {
                qlStneve << fs.right(fs.length()-1);//убираем ведущую ;
                isWritten = true;
                break;
            }
        }
        if (!isWritten)
            qlStneve << fs;
    }//foreach(QString fs,

    if(fl.open(QIODevice::WriteOnly))
    {
        foreach(QString sStneve, qlStneve)
        {
            QTextStream fileStream(&fl);
            fileStream.setEncoding(QStringConverter::Utf8);//setCodec("CP1251");
            fileStream << sStneve;
        }
        fl.close();
    }
}
//вычисляем, который это день в месяце - первая пятница, вторая, третья и т.п.
int MainWindow::getDayOfWeekOfMonth(int pCurDay)
{
    QDate testDate = QDate::currentDate().addDays(pCurDay);
    int dim = testDate.daysInMonth();
    int dow = testDate.dayOfWeek();
    int dowom = 0;
    for(int i = 1; i <= dim; i++)
    {
        QDate forDate = QDate(testDate.year(), testDate.month(), i);
        int forDow = forDate.dayOfWeek();
        if (forDow == dow)
        {
            dowom++;
            if (forDate == testDate)
                return  dowom;
        }
    }
}

void MainWindow::on_actionToolBar_triggered()
{
    if (!gToolBar) {//ui->mainToolBar->isHidden()) {
        gToolBar = true;
        ui->mainToolBar->show();
    } else {
        gToolBar = false;
        ui->mainToolBar->hide();
    }
    //qDebug() << "actionToolBar " << gToolBar;
    /*ui->actionToolBar->setChecked(gToolBar);
    gSettings.setValue("/ToolBar", gToolBar);*/
}

void MainWindow::handleToolBarVisibilityChange() {
    gToolBar = !ui->mainToolBar->isHidden();
    //qDebug() << "ToolBarVisibilityChange " << gToolBar;
    ui->actionToolBar->setChecked(gToolBar);
    gSettings.setValue("/ToolBar", gToolBar);
}

void MainWindow::on_actionMenuBar_triggered()
{
    if (!gMenuBar) {//ui->menuBar->isHidden()) {
        gMenuBar = true;
        ui->menuBar ->show();
    } else {
        gMenuBar = false;
        ui->menuBar->hide();
    }
    //qDebug() << "gMenuBar " << gMenuBar;
    ui->actionMenuBar->setChecked(gMenuBar);
    gSettings.setValue("/MenuBar", gMenuBar);
}
