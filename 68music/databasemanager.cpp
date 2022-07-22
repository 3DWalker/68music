#include "databasemanager.h"

DatabaseManager::DatabaseManager()
{
    m_sqlDatabase = QSqlDatabase::addDatabase("QSQLITE");
}

DatabaseManager *DatabaseManager::getInstance()
{
    static DatabaseManager manager;
    return &manager;
}

//打开数据库
bool DatabaseManager::open()
{
    m_sqlDatabase.setDatabaseName(m_databaseName);
    bool ok = m_sqlDatabase.open();
    if(ok)
    {
        ok = initSongs();
        if (!ok)
        {
            qDebug() << "Init songs table failed";
            close();
        }
    }
    else
    {
        qDebug() << "Open database failed";
    }

    return ok;
}

//关闭数据库
void DatabaseManager::close()
{
    if (m_sqlDatabase.isOpen())
    {
        m_sqlDatabase.close();
    }
    QSqlDatabase::removeDatabase(m_databaseName);
}

//初始化歌曲表
bool DatabaseManager::initSongs()
{
    bool ok = m_sqlDatabase.isOpen();
    if (ok)
    {
        QSqlQuery query;
        query.prepare("create table if not exists songs("
                      "id integer primary key,"
                      "url text not null,"
                      "name text not null,"
                      "artist text not null,"
                      "album text not null);");
        ok = query.exec();
        if (!ok)
        {
            qDebug() << "Init songs table failed, error:" << query.lastError().text();
        }
    }
    else
    {
        qDebug() << "Init songs table failed, sql is not open";
    }

    return ok;
}

//添加音乐
bool DatabaseManager::addSongs(const Song &song)
{
    bool ok = m_sqlDatabase.isOpen();
    if (ok)
    {
        QSqlQuery query;
        query.prepare("insert into songs(url, name, artist, album)"
                      "values(:url, :name, :artist, :album);");
        query.bindValue(":url", song.url());
        query.bindValue(":name", song.name());
        query.bindValue(":artist", song.artist());
        query.bindValue(":album", song.album());
        ok = query.exec();
        if (!ok)
        {
            qDebug() << "Add song failed, error:" << query.lastError().text();
        }
    }
    else
    {
        qDebug() << "Add song failed, sql is not open";
    }

    return ok;
}

//查询歌曲
bool DatabaseManager::querySongs(QList<Song *> &songsResult)
{
    bool ok = m_sqlDatabase.isOpen();
    if (ok)
    {
        QSqlQuery query;
        query.prepare("select url, name, artist, album from songs;");
        ok = query.exec();
        if (ok)
        {
            QString url, name, artist, album;
            Song* song = nullptr;
            while (query.next())
            {
                url = query.value(0).toString();
                name = query.value(1).toString();
                artist = query.value(2).toString();
                album = query.value(4).toString();
                song = new Song(url, name, artist, album);
                songsResult.push_back(song);
                qDebug() << "query succeed, song name:" << name;
            }
        }
        else
        {
            qDebug() << "query songs failed, error:" << query.lastError().text();
        }
    }
    else
    {
        qDebug() << "query songs failed, sql is not open";
    }

    return ok;
}

//清空/删除歌曲表
bool DatabaseManager::clearSongs()
{
    bool ok = m_sqlDatabase.isOpen();
    if (ok)
    {
        QSqlQuery query;
        query.prepare("delete from songs;");
        ok = query.exec();
        if (!ok)
        {
            qDebug() << "Clear songs failed, error:" << query.lastError().text();
        }
    }
    else
    {
        qDebug() << "Clear songs failed, sql is not open";
    }

    return ok;
}
