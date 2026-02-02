/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version. See COPYING file for more details.
 *
 */

#ifndef PATHMANAGER_H
#define PATHMANAGER_H

#include <QString>

class PathManager
{
public:
    PathManager();

    enum class FileType {
        Dates,
        Events,
        Runs
    };
    QString datesFilePath(); // путь к файлу государственных и других общих праздников
    QString eventsFilePath(); // путь к файлу дней рождения и других личных праздников
    QString runsFilePath(); // путь к файлу афоризмов
    bool ok(); // обработка ошибок
    QString errString();
private:
    const QString datesFileName = "dates.txt";
    const QString eventsFileName = "events.txt";
    const QString runsFileName = "runs.txt";
    bool _ok;
    QString _errString;
    QString configPath, _datesFilePath, _eventsFilePath, _runsFilePath;
};

#endif // PATHMANAGER_H
