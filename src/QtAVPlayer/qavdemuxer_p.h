/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVDEMUXER_H
#define QAVDEMUXER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qavpacket_p.h"
#include "qavstream.h"
#include <QObject>
#include <QMap>
#include <QScopedPointer>

QT_BEGIN_NAMESPACE

class QAVDemuxerPrivate;
class QAVVideoCodec;
class QAVAudioCodec;
class QAVIODevice;
struct AVStream;
struct AVCodecContext;
struct AVFormatContext;
class Q_AVPLAYER_EXPORT QAVDemuxer : public QObject, public std::enable_shared_from_this<QAVDemuxer>
{
public:
    QAVDemuxer(QObject *parent = nullptr);
    ~QAVDemuxer();

    void abort(bool stop = true);
    int load(const QString &url, QAVIODevice *dev = nullptr);
    void unload();

    QList<QAVStream> videoStreams() const;
    QAVStream videoStream() const;
    bool setVideoStream(const QAVStream &stream);
    QList<QAVStream> audioStreams() const;
    QAVStream audioStream() const;
    bool setAudioStream(const QAVStream &stream);

    QList<QAVStream> subtitleStreams() const;

    AVStream *stream(int index) const;
    QAVCodec *codec(int index) const;

    QAVPacket read();

    double duration() const;
    bool seekable() const;
    int seek(double sec);
    bool eof() const;
    double videoFrameRate() const;

    AVRational frameRate() const;

    QMap<QString, QString> metadata() const;

    static QStringList supportedFormats();
    static QStringList supportedProtocols();

protected:
    QScopedPointer<QAVDemuxerPrivate> d_ptr;

private:
    Q_DISABLE_COPY(QAVDemuxer)
    Q_DECLARE_PRIVATE(QAVDemuxer)
};

QT_END_NAMESPACE

#endif
