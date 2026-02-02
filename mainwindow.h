/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version. See COPYING file for more details.
 *
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPlainTextEdit>
#include <QSettings>
#include <QDate>
#include <QCloseEvent>
#include <QSystemTrayIcon>
#if QT_VERSION >= 0x050000
#include <QRegularExpression>
#define QRegExp QRegularExpression
#endif
#include <QRandomGenerator>
#include "pathmanager.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void scrollToStartEdit();

protected:
    void showEvent(QShowEvent* event) override;
    void timerEvent(QTimerEvent *);
    void resizeEvent(QResizeEvent *);
    void closeEvent(QCloseEvent * event);


private slots:
    void on_actionColor_triggered();

    void on_actionFont_triggered();

    void on_actionExit_triggered();

    void on_actionEdit_triggered();

    void on_actionSettings_triggered();

    void on_actionAbout_Qt_triggered();

    void on_actionLicense_triggered();

    void on_actionAbout_triggered();

    void on_actionColor3_triggered();

    void on_actionToolBar_triggered();

    void handleToolBarVisibilityChange();

    void showContextMenuEvents(const QPoint& pos);
    void showContextMenuDates(const QPoint& pos);
    void showContextMenuRuns(const QPoint& pos);
    void editContextMenuEvents();
    void editContextMenuDates();
    void editContextMenuRuns();
    void showContextMenuDatesCopy();
    void showContextMenuDatesSelectAll();
    void showContextMenuEventsCopy();
    void showContextMenuEventsSelectAll();
    void showContextMenuRunsCopy();
    void iconActivated(QSystemTrayIcon::ActivationReason reason);

private:
    Ui::MainWindow *ui;
    QRandomGenerator rndRuns;
    void refreshWindows();
    QDateTime refreshTitle();
    void refreshRuns(bool changeText = true);
    void setLst(const QString& path, const PathManager::FileType &type);
    void setLstEvents();
    void setLstDates();
    void setLstRuns();
    QString getDaysStr(int pDays);
    void getCurrentLStrPrefix(QStringList &slb, const QList<QString> &pql, const QDate &currentDate, QString prefix);
    QStringList getResultLStr(QList<QString>, int pdays);
    QStringList getResultTodayLStr(QList<QString>);
    QStringList getResultYesterdayLStr(QList<QString>);
    QStringList getResultTomorrowLStr(QList<QString>);
    //QString getResult3Str(QList<QString>);
    int getDayOfWeekOfMonth(int pCurDay = 0);
    void findTodayStrs(QPlainTextEdit *);
    void setWindowFont();
    void setWindowSize();
    void setGColor();
    void callDatesEventsFile(QList<QString>&, QString);
    void unCommentEvents();
    QList<QString> qlEvents;
    QList<QString> qlDates;
    QList<QString> qlRuns;
    //QList<QString> qlToday;
    //QList<QString> ql3;
    QColor gColorTodayText;
    QString gsColorTodayText;
    QColor gColorOtherText;
    QString gsColorOtherText;
    int gDays;
    int currentDatesCount;
    int currentEventsCount;
    QString gDelimiter;
    bool gTray;
    bool gToolBar;
    QSettings gSettings;
    PathManager pathMan;
    QDate lastDate;
    const QRegExp regexpDt;
    QSystemTrayIcon* trayIcon;
};

#endif // MAINWINDOW_H
