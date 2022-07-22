#include "song.h"

const QUrl &Song::url() const
{
    return m_url;
}

const QString &Song::name() const
{
    return m_name;
}

const QString &Song::artist() const
{
    return m_artist;
}

const QString &Song::album() const
{
    return m_album;
}
