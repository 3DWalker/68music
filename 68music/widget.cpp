#include "widget.h"
#include "ui_widget.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    //分配一个播放器
    m_player = new QMediaPlayer();

    //连接--接收播放状态变化的信号
    connect(m_player, &QMediaPlayer::stateChanged, this, &Widget::handlePlayerStateChanged);
    //连接--接收播放位置变化的信号
    connect(m_player, &QMediaPlayer::positionChanged, this, &Widget::handlePlayerPositionChanged);

    //新建一个播放列表
    m_playlist = new QMediaPlaylist();
    //设置播放列表的默认播放模式为顺序播放
    m_playlist->setPlaybackMode(QMediaPlaylist::PlaybackMode::Sequential);

    //连接--接收播放模式变化的信号
    connect(m_playlist, &QMediaPlaylist::playbackModeChanged, this, &Widget::handlePlaylistPlaybackModeChanged);
    //连接--接收当前播放内容变化的信号
    connect(m_playlist, &QMediaPlaylist::currentMediaChanged, this, &Widget::handlePlaylistCurrentMediaChanged);

    //把播放列表分配给播放器
    m_player->setPlaylist(m_playlist);

    //分配一个计时器
    m_timer = new QTimer(this);
    //连接--接收计时器超时的信号
    connect(m_timer, &QTimer::timeout, this, &Widget::handleTimeout);
    m_timer->start(100);

    //去掉歌词列表的滑块
    ui->lyrics->setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
    ui->lyrics->setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);

    if (initDatabase())
    {
        getPlaylistFromSql();
    }
}

Widget::~Widget()
{
    for (auto item : m_currentPlaylist)
    {
        delete item;
    }
    delete m_timer;
    delete m_player;
    delete m_playlist;
    destroyDatabase();
    delete ui;
}

//添加音乐到播放列表
void Widget::on_add_clicked()
{
    QStringList files = QFileDialog::getOpenFileNames(this
                                                      , tr("添加音乐")
                                                      , QDir::currentPath() + "/../68music/songs"
                                                      , tr("音频文件(*.mp3)"));
    for (auto file :files)
    {
        m_playlist->addMedia(QMediaContent(file));
        QListWidgetItem *item = new QListWidgetItem();
        item->setText(QFileInfo(QDir(file).dirName()).baseName());
        ui->playlist->addItem(item);
        getSongInfoFromMpsFile(file);
    }
}

//播放和暂停音乐
void Widget::on_play_clicked()
{
    if (m_player->state() != m_player->PlayingState)
    {
        m_player->play();
    }
    else
    {
        m_player->pause();
    }
}

//处理播放状态变化的槽函数
void Widget::handlePlayerStateChanged(QMediaPlayer::State state)
{
    if (state == QMediaPlayer::PlayingState)
    {
        ui->play->setIcon(QPixmap(":images/play.png"));
    }
    else
    {
        ui->play->setIcon(QPixmap(":images/pause.png"));
    }
}

//处理单击播放模式按钮的槽函数
void Widget::on_playbackMode_clicked()
{
    QMediaPlaylist::PlaybackMode mode = m_playlist->playbackMode();
    int nextMode = (mode +1) % 5;
    m_playlist->setPlaybackMode(QMediaPlaylist::PlaybackMode(nextMode));
}

//处理播放模式变化的槽函数
void Widget::handlePlaylistPlaybackModeChanged(QMediaPlaylist::PlaybackMode mode)
{
    QStringList modeIconList = { ":/images/currentItemOnce.png"
                               , ":/images/currentItemLoop.png"
                               , ":/images/sequential.png"
                               , ":/images/loop.png"
                               , ":/images/random.png" };
    ui->playbackMode->setIcon(QPixmap(modeIconList[mode]));
}

//切换到上一首歌曲
void Widget::on_previous_clicked()
{
    m_playlist->previous();
}

//切换到下一首歌曲
void Widget::on_next_clicked()
{
    m_playlist->next();
}

//处理播放列表当前媒体内容变化的槽函数
void Widget::handlePlaylistCurrentMediaChanged(const QMediaContent &content)
{
    QString songNames = QFileInfo(content.request().url().fileName()).baseName();
    //ui->songName->setText(songNames);
    auto item = m_currentPlaylist.begin();
    while (item != m_currentPlaylist.end())
    {
        if (QUrl(item.key()).path() == QUrl(content.request().url().toString()).path())
        {
            ui->songName->setText(item.value()->name() + " - " + item.value()->artist());
            break;
        }
        else
        {
            qDebug() << "It's not the file: " << item.key() << ", " << content.request().url().toString();
        }
    }

    //通过音乐路径获取歌词路径
    QString mediaPath = content.request().url().path();
    QString lyricsPath = mediaPath.replace(".mp3", ".lrc");
    //判断得到的歌词文件是不是一个文件
    if (QFileInfo(lyricsPath).isFile())
    {
        readLyricsFromFile(lyricsPath);
    }
    else
    {
        m_lyrics.clear();
    }
    updateAllLyrics();
}

//处理播放进度滑块拖动释放的槽函数
void Widget::on_progress_sliderReleased()
{
    m_player->setPosition((double)ui->progress->value() / ui->progress->maximum() * m_player->duration());
}

//处理音乐播放位置变化的槽函数
void Widget::handlePlayerPositionChanged(qint64 position)
{
    //根据播放位置设置进度条的值
    double progressValue = (double)position / m_player->duration() * 99;
    ui->progress->setValue(progressValue);

    //设置时间标签
    char time[12] = {0};
    snprintf(time, sizeof(time)
             , "%02lld:%02lld/%02lld:%02lld"
             , position / (60 * 1000)
             , position % (60 * 1000) / 1000
             , m_player->duration() / (60 * 1000)
             , m_player->duration() % (60 * 1000) / 1000);
    ui->progressLabel->setText(time);
}

//从文件中读取歌词
void Widget::readLyricsFromFile(const QString &lyricsFilePath)
{
    //清除所保存的上一首歌词
    m_lyrics.clear();
    //声明一个字符串保存每行内容，[时间标签]文本
    QString line;
    //声明一个字符串列表保存第一次分割的结果，[时间标签，文本
    QStringList lineContents;
    //声明一个字符串列表保存时间标签分割的结果，[分钟数，秒数.毫秒数
    QStringList timeContents;

    QFile file(lyricsFilePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream textStream(&file);
        while (!textStream.atEnd())
        {
            line = textStream.readLine();
            if (!line.isEmpty())
            {
                lineContents = line.split("]");
                timeContents = lineContents[0].split(":");
                //计算播放时间
                int minutes = timeContents[0].mid(1).toInt();
                double seconds = timeContents[1].toDouble();
                qint64 time = (minutes * 60 +seconds) * 1000;
                //保存歌词信息到m_lyrics中
                m_lyrics.insert(time, lineContents[1]);
                //变量初始化
                lineContents.clear();
                timeContents.clear();
            }
        }
    }
}

//在歌词列表中显示歌词
void Widget::updateAllLyrics()
{
    //切歌时清空上一首音乐的歌词
    ui->lyrics->clear();
    //如果有歌词则显示歌词
    if (!m_lyrics.empty())
    {
        for (auto text : m_lyrics.values())
        {
            QListWidgetItem *item = new QListWidgetItem();
            item->setText(text);
            ui->lyrics->addItem(item);
        }
    }
    else
    {
        ui->lyrics->clear();
        QListWidgetItem *item = new QListWidgetItem();
        item->setText("当前音乐没有歌词");
        ui->lyrics->addItem(item);
    }
}

//处理定时器超时信号的槽函数
void Widget::handleTimeout()
{
    updateLyricsOnTime();
}

//同步音乐与歌词
void Widget::updateLyricsOnTime()
{
    if (!m_lyrics.empty())
    {
        //如果界面还未开始，就将第一行设置为当前行
        if (-1 == ui->lyrics->currentRow())
        {
            ui->lyrics->setCurrentRow(0);
            return;
        }
        else
        {
            //获取当前行的行数
            int currentRow = ui->lyrics->currentRow();
            //获取播放器当前播放的位置
            qint64 playPosition = m_player->position();
            QList<qint64> lyricsPosition = m_lyrics.keys();
            if (playPosition < lyricsPosition[currentRow])
            {
                while (currentRow > 0)
                {
                    --currentRow;
                    if (playPosition >= lyricsPosition[currentRow])
                    {
                        break;
                    }
                }
            }
            else if (playPosition > lyricsPosition[currentRow])
            {
                while (currentRow < lyricsPosition.size() - 1)
                {
                    ++currentRow;
                    if (playPosition < lyricsPosition[currentRow])
                    {
                        --currentRow;
                        break;
                    }
                }
            }

            QListWidgetItem *item = ui->lyrics->item(currentRow);
            ui->lyrics->setCurrentItem(item);
            ui->lyrics->scrollToItem(item, QAbstractItemView::PositionAtCenter);
        }
    }
}

//打开数据库
bool Widget::initDatabase()
{
    return m_databse->getInstance()->open();
}

//调用数据库管理类的查询歌曲方法，查询所有歌曲
void Widget::getPlaylistFromSql()
{
    QList<Song *> songsResult;
    bool ok = m_databse->getInstance()->querySongs(songsResult);
    if (ok)
    {
        for (int i = 0; i < songsResult.size(); i++)
        {
            m_currentPlaylist.insert(songsResult[i]->url().toString(), songsResult[i]);

            QListWidgetItem *item = new QListWidgetItem();
            item->setText(songsResult[i]->name() + " - " + songsResult[i]->artist());
            ui->playlist->addItem(item);

            m_playlist->addMedia(songsResult[i]->url());
        }
    }
}

//添加音乐到数据库
void Widget::addSongToSqlPlaylist(const Song &song)
{
     m_databse->getInstance()->addSongs(song);
}

void Widget::getSongInfoFromMpsFile(const QString &filePath)
{
    QString name, artist, album;
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        int length = file.size();
        file.seek(length - 128);
        char infoArray[128];
        if ( file.read(infoArray, sizeof(infoArray)) > 0)
        {
            QString tag = QString::fromLocal8Bit(infoArray, 3);
            name = QString::fromLocal8Bit(infoArray + 3, 30);
            artist = QString::fromLocal8Bit(infoArray + 33, 30);
            album = QString::fromLocal8Bit(infoArray + 63, 30);

            name.truncate(name.indexOf(QChar::Null));
            artist.truncate(artist.indexOf(QChar::Null));
            album.truncate(album.indexOf(QChar::Null));
        }
        else
        {
            qDebug() << "get song info failed";
        }
        file.close();
    }
    else
    {
        qDebug() << "open file failed";
    }

    Song *song = new Song(filePath, name, artist, album);
    addSongToSqlPlaylist(*song);
    m_currentPlaylist.insert(filePath, song);
}

//单击清除按钮的槽函数
void Widget::on_clearPlaylist_clicked()
{
    m_playlist->clear();
    ui->playlist->clear();
    clearSqlPlaylist();
}

void Widget::clearSqlPlaylist()
{
    m_databse->getInstance()->clearSongs();
}

void Widget::destroyDatabase()
{
    m_databse->getInstance()->close();
}
