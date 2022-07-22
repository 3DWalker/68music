#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QFileDialog>
#include <QTimer>
#include "databasemanager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

    void readLyricsFromFile(const QString &lyricsFilePath);

    void updateAllLyrics();

    void updateLyricsOnTime();

    bool initDatabase();

    void getPlaylistFromSql();

    void addSongToSqlPlaylist(const Song &song);

    void getSongInfoFromMpsFile(const QString &filePath);

    void clearSqlPlaylist();

    void destroyDatabase();

private slots:
    void on_add_clicked();

    void on_play_clicked();

    void handlePlayerStateChanged(QMediaPlayer::State state);

    void on_playbackMode_clicked();

    void handlePlaylistPlaybackModeChanged(QMediaPlaylist::PlaybackMode mode);

    void on_previous_clicked();

    void on_next_clicked();

    void handlePlaylistCurrentMediaChanged(const QMediaContent &content);

    void on_progress_sliderReleased();

    void handlePlayerPositionChanged(qint64 position);

    void handleTimeout();

    void on_clearPlaylist_clicked();

private:
    Ui::Widget *ui;
    QMediaPlayer *m_player;
    QMediaPlaylist *m_playlist;
    QMap<qint64, QString> m_lyrics;
    QTimer *m_timer;
    DatabaseManager *m_databse;
    QMap<QString, Song*> m_currentPlaylist;
};
#endif // WIDGET_H
