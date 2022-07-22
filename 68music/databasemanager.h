#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <song.h>

class DatabaseManager
{
private:
    DatabaseManager();
    QSqlDatabase m_sqlDatabase;
    QString m_databaseName = "68music.db";

public:
    static DatabaseManager *getInstance();

    bool open();

    void close();

    bool initSongs();

    bool addSongs(const Song &song);

    bool querySongs(QList<Song*>& songsResult);

    bool clearSongs();
};

#endif // DATABASEMANAGER_H
