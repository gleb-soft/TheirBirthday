/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version. See COPYING file for more details.
 *
 */

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QObject>
#if QT_VERSION >= 0x050000
#include <QStandardPaths>
#else
#include <QDesktopServices>
#endif
#include "pathmanager.h"

PathManager::PathManager()
    :_ok(false), _errString(""), configPath("")
{
    // Путь к шаблонам рабочих файлов
    QString templPath =
#ifdef Q_OS_WIN32
            qApp->applicationDirPath() + QDir::separator() + "templates";
#else
            "/etc/skel/.local/share/TheirBirthdaySoft/TheirBirthday";
#endif
    // Путь к рабочим файлам (for Linux, in /home...)
    QString configPath =
#if QT_VERSION >= 0x050000
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
#else
        QDesktopServices::storageLocation( QDesktopServices::DataLocation);
#endif
    //_errString = configPath; //==>
    //return; //===>
    _datesFilePath = configPath+QDir::separator()+datesFileName;
    _eventsFilePath = configPath+QDir::separator()+eventsFileName;
    _runsFilePath = configPath+QDir::separator()+runsFileName;
    // Файл дат копируем, если первый запуск
    QDir d;
    if (!d.exists(configPath))
       d.mkpath(configPath);
    if (!d.exists(configPath)) {
        _errString = QObject::tr("Не могу создать каталог %1").arg(configPath);
        return;
    }
    if (!QFile::exists(_datesFilePath)) {
        QString datesTemplatePath = templPath + QDir::separator()+datesFileName;
        if (!QFile::copy(datesTemplatePath, _datesFilePath)) {
            _errString = QObject::tr("Не могу скопировать файл %1").arg(datesTemplatePath);
            return;
        }
    }
    // Файл событий - аналогично
    if (!QFile::exists(_eventsFilePath)) {
        QString eventsTemplatePath = templPath + QDir::separator()+eventsFileName;
        if (!QFile::copy(eventsTemplatePath, _eventsFilePath)) {
            _errString = QObject::tr("Не могу скопировать файл %1").arg(eventsTemplatePath);
            return;
        }
    }
    // Файл афоризмов - аналогично
    if (!QFile::exists(_runsFilePath)) {
        QString runsTemplatePath = templPath + QDir::separator()+runsFileName;
        if (!QFile::copy(runsTemplatePath, _runsFilePath)) {
            _errString = QObject::tr("Не могу скопировать файл %1").arg(runsTemplatePath);
            return;
        }
    }
    _ok = true;
}

QString PathManager::datesFilePath()
{
    return _datesFilePath;
}

QString PathManager::eventsFilePath()
{
    return _eventsFilePath;
}

QString PathManager::runsFilePath()
{
    return _runsFilePath;
}

bool PathManager::ok()
{
    return _ok;
}

QString PathManager::errString()
{
    return _errString;
}
