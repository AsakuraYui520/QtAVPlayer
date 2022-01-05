/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "private/qavdemuxer_p.h"
#include "qavaudioframe.h"
#include "qavvideoframe.h"
#include "private/qaviodevice_p.h"
#include "private/qavvideocodec_p.h"
#include "private/qavaudiocodec_p.h"

#include <QDebug>
#include <QtTest/QtTest>

QT_USE_NAMESPACE

class tst_QAVDemuxer : public QObject
{
    Q_OBJECT
private slots:
    void construction();
    void loadIncorrect();
    void loadAudio();
    void loadVideo();
    void fileIO();
    void qrcIO();
    void supportedFormats();
    void metadata();
};

void tst_QAVDemuxer::construction()
{
    auto d = std::make_shared<QAVDemuxer>();
    QVERIFY(!d->videoStream());
    QVERIFY(!d->audioStream());
    QCOMPARE(d->duration(), 0);
    QCOMPARE(d->seekable(), false);
    QCOMPARE(d->eof(), false);
    QVERIFY(!d->read());
    QVERIFY(d->seek(0) < 0);

    QAVPacket p;
    QVERIFY(!p);
    QCOMPARE(p.duration(), 0);
    QCOMPARE(p.bytes(), 0);
    QVERIFY(p.streamIndex() < 0);
    QVERIFY(!p.decode());

    QAVFrame f;
    QVERIFY(!f);
    QVERIFY(isnan(f.pts()));

    QAVAudioFrame af;
    QVERIFY(!af);
    QVERIFY(isnan(af.pts()));

    QAVVideoFrame vf;
    QVERIFY(!vf);
}

void tst_QAVDemuxer::loadIncorrect()
{
    auto d = std::make_shared<QAVDemuxer>();
    QVERIFY(d->load(QLatin1String("unknown.mp4")) < 0);
    QVERIFY(!d->videoStream());
    QVERIFY(!d->audioStream());
    QVERIFY(!d->read());
    QVERIFY(d->seek(0) < 0);
}

void tst_QAVDemuxer::loadAudio()
{
    auto d = std::make_shared<QAVDemuxer>();

    QFileInfo file(QLatin1String("../testdata/test.wav"));

    QVERIFY(d->load(file.absoluteFilePath()) >= 0);
    QVERIFY(!d->videoStream());
    QVERIFY(d->audioStream());
    QVERIFY(d->duration() > 0);
    QCOMPARE(d->seekable(), true);
    QCOMPARE(d->eof(), false);

    QAVPacket p;
    QAVPacket p2;
    QAVFrame f2;
    while ((p = d->read())) {
        QVERIFY(p);
        QVERIFY(p.packet());
        QVERIFY(p.duration() > 0);
        QVERIFY(p.pts() >= 0);
        QVERIFY(p.bytes() > 0);
        QCOMPARE(p.streamIndex(), d->audioStream().index());

        p2 = p;
        QVERIFY(p2);
        QCOMPARE(p2.duration(), p.duration());
        QCOMPARE(p2.bytes(), p.bytes());
        QCOMPARE(p2.streamIndex(), p.streamIndex());

        QAVPacket p3 = p2;
        QVERIFY(p2);
        QCOMPARE(p2.duration(), p.duration());
        QCOMPARE(p2.bytes(), p.bytes());
        QCOMPARE(p2.streamIndex(), p.streamIndex());

        auto f = p.decode();
        QVERIFY(f);
        QVERIFY(f.frame());
        QVERIFY(f.pts() >= 0);

        f2 = f;
        QVERIFY(f2);
        QVERIFY(f2.frame());
        QCOMPARE(f2.pts(), f.pts());

        QAVFrame f3 = f2;
        QVERIFY(f3);
        QVERIFY(f3.frame());
        QCOMPARE(f3.pts(), f.pts());

        QAVAudioFrame af = f;
        QVERIFY(af);
        QVERIFY(af.frame());
        QCOMPARE(af.pts(), f.pts());
        QVERIFY(af.stream().codec()->codec() != nullptr);

        auto format = af.format();
        QCOMPARE(format.sampleFormat(), QAVAudioFormat::Int32);
        auto data = af.data();
        QVERIFY(!data.isEmpty());

        QCOMPARE(d->eof(), false);
    }

    QVERIFY(p2);
    QVERIFY(p2.packet());
    QVERIFY(p2.duration() > 0);
    QVERIFY(p2.bytes() > 0);
    QCOMPARE(p2.streamIndex(), d->audioStream().index());

    QVERIFY(f2);
    QVERIFY(f2.frame());
    QVERIFY(f2.pts() > 0);

    QCOMPARE(d->eof(), true);
    QVERIFY(d->seek(0) >= 0);
}

void tst_QAVDemuxer::loadVideo()
{
    auto d = std::make_shared<QAVDemuxer>();

    QFileInfo file(QLatin1String("../testdata/colors.mp4"));

    QVERIFY(d->load(file.absoluteFilePath()) >= 0);
    QVERIFY(d->videoStream());
    QVERIFY(d->audioStream());
    QVERIFY(d->duration() > 0);
    QCOMPARE(d->seekable(), true);
    QCOMPARE(d->eof(), false);

    auto p = d->read();
    QVERIFY(p);
    QVERIFY(p.packet());
    QVERIFY(p.duration() > 0);
    QVERIFY(p.bytes() > 0);
    QVERIFY(p.streamIndex() >= 0);

    auto f = p.decode();
    QVERIFY(f);
    QVERIFY(f.frame());
    QVERIFY(f.pts() >= 0);
    QCOMPARE(d->eof(), false);

    QVERIFY(d->seek(0) >= 0);
    while ((p = d->read())) {
        QCOMPARE(d->eof(), false);
        if (p.streamIndex() == d->videoStream().index()) {
            auto f = p.decode();
            QVERIFY(f);
            QVERIFY(f.frame());
            QVERIFY(f.pts() >= 0);

            QAVVideoFrame vf = f;
            QVERIFY(vf);
            QVERIFY(vf.frame());
            QCOMPARE(vf.pts(), f.pts());
            QVERIFY(vf.size().isValid());
        }
    }

    QCOMPARE(d->eof(), true);
}

void tst_QAVDemuxer::fileIO()
{
    auto d = std::make_shared<QAVDemuxer>();

    QFile file(QLatin1String("../testdata/colors.mp4"));
    if (!file.open(QIODevice::ReadOnly)) {
        QFAIL("Could not open");
        return;
    }

    QAVIODevice dev(file);
    QVERIFY(d->load(QLatin1String("colors.mp4"), &dev) >= 0);

    QVERIFY(d->videoStream());
    QVERIFY(d->audioStream());
    QVERIFY(d->duration() > 0);
    QCOMPARE(d->seekable(), true);
    QCOMPARE(d->eof(), false);

    auto p = d->read();
    QVERIFY(p);
    QVERIFY(p.packet());
    QVERIFY(p.duration() > 0);
    QVERIFY(p.bytes() > 0);
    QVERIFY(p.streamIndex() >= 0);

    auto f = p.decode();
    QVERIFY(f);
    QVERIFY(f.frame());
    QVERIFY(f.pts() >= 0);
    QCOMPARE(d->eof(), false);

    QVERIFY(d->seek(0) >= 0);
    while ((p = d->read())) {
        QCOMPARE(d->eof(), false);
        if (p.streamIndex() == d->videoStream().index()) {
            auto f = p.decode();
            QVERIFY(f);
            QVERIFY(f.frame());
            QVERIFY(f.pts() >= 0);

            QAVVideoFrame vf = f;
            QVERIFY(vf);
            QVERIFY(vf.frame());
            QCOMPARE(vf.pts(), f.pts());
            QVERIFY(vf.size().isValid());
        }
    }

    QCOMPARE(d->eof(), true);
}

void tst_QAVDemuxer::qrcIO()
{
    auto d = std::make_shared<QAVDemuxer>();

    QFile file(QLatin1String(":/test.wav"));
    if (!file.open(QIODevice::ReadOnly)) {
        QFAIL("Could not open");
        return;
    }

    QAVIODevice dev(file);
    QVERIFY(d->load(QLatin1String("test.wav"), &dev) >= 0);
    QVERIFY(!d->videoStream());
    QVERIFY(d->audioStream());
    QVERIFY(d->duration() > 0);
    QCOMPARE(d->seekable(), true);
    QCOMPARE(d->eof(), false);

    QAVPacket p;
    QAVPacket p2;
    QAVFrame f2;
    while ((p = d->read())) {
        QVERIFY(p);
        QVERIFY(p.packet());
        QVERIFY(p.duration() > 0);
        QVERIFY(p.pts() >= 0);
        QVERIFY(p.bytes() > 0);
        QCOMPARE(p.streamIndex(), d->audioStream().index());

        p2 = p;
        QVERIFY(p2);
        QCOMPARE(p2.duration(), p.duration());
        QCOMPARE(p2.bytes(), p.bytes());
        QCOMPARE(p2.streamIndex(), p.streamIndex());

        QAVPacket p3 = p2;
        QVERIFY(p2);
        QCOMPARE(p2.duration(), p.duration());
        QCOMPARE(p2.bytes(), p.bytes());
        QCOMPARE(p2.streamIndex(), p.streamIndex());

        auto f = p.decode();
        QVERIFY(f);
        QVERIFY(f.frame());
        QVERIFY(f.pts() >= 0);

        f2 = f;
        QVERIFY(f2);
        QVERIFY(f2.frame());
        QCOMPARE(f2.pts(), f.pts());

        QAVFrame f3 = f2;
        QVERIFY(f3);
        QVERIFY(f3.frame());
        QCOMPARE(f3.pts(), f.pts());

        QAVAudioFrame af = f;
        QVERIFY(af);
        QVERIFY(af.frame());
        QCOMPARE(af.pts(), f.pts());
        QVERIFY(af.stream().codec()->codec() != nullptr);

        auto format = af.format();
        QCOMPARE(format.sampleFormat(), QAVAudioFormat::Int32);
        auto data = af.data();
        QVERIFY(!data.isEmpty());

        QCOMPARE(d->eof(), false);
    }

    QVERIFY(p2);
    QVERIFY(p2.packet());
    QVERIFY(p2.duration() > 0);
    QVERIFY(p2.bytes() > 0);
    QCOMPARE(p2.streamIndex(), d->audioStream().index());

    QVERIFY(f2);
    QVERIFY(f2.frame());
    QVERIFY(f2.pts() > 0);

    QCOMPARE(d->eof(), true);
    QVERIFY(d->seek(0) >= 0);
}

void tst_QAVDemuxer::supportedFormats()
{
    auto d = std::make_shared<QAVDemuxer>();
    auto fmts = QAVDemuxer::supportedFormats();
    QVERIFY(!fmts.isEmpty());
    QVERIFY(!QAVDemuxer::supportedProtocols().isEmpty());
    if (fmts.contains(QLatin1String("v4l2"))) {
        QFileInfo file(QLatin1String("/dev/video0"));
        if (file.exists()) {
            QVERIFY(d->load(QLatin1String(" -f   v4l2   -i /dev/video0")) >= 0);
            d->unload();
            QVERIFY(d->load(QLatin1String("-f v4l2 -i /dev/video0")) >= 0);
            d->unload();
            QVERIFY(d->load(QLatin1String("-i /dev/video0 -f v4l2")) >= 0);
            d->unload();
            QVERIFY(d->load(QLatin1String("/dev/video0")) >= 0);
            d->unload();
            QVERIFY(d->load(QLatin1String("-i /dev/video0")) >= 0);
            d->unload();
            QVERIFY(d->load(QLatin1String("-f v4l2")) < 0);
            d->unload();
        }
    }

    QVERIFY(d->load(QLatin1String("-f v4l2 -i /dev/dummy")) < 0);
    d->unload();
    QVERIFY(d->load(QLatin1String("/dev/dummy")) < 0);
    d->unload();
    QVERIFY(d->load(QLatin1String("-i /dev/dummy")) < 0);
    d->unload();

    QFileInfo file(QLatin1String("../testdata/colors.mp4"));
    QVERIFY(d->load("-i " + file.absoluteFilePath()) >= 0);
    d->unload();
    QVERIFY(d->load(" -i " + file.absoluteFilePath()) >= 0);
}

void tst_QAVDemuxer::metadata()
{
    auto d = std::make_shared<QAVDemuxer>();

    QFileInfo file(QLatin1String("../testdata/colors.mp4"));

    QVERIFY(d->load(file.absoluteFilePath()) >= 0);
    QVERIFY(!d->metadata().isEmpty());
    QVERIFY(!d->videoStream().metadata().isEmpty());
    QVERIFY(!d->audioStream().metadata().isEmpty());
    for (auto &stream : d->audioStreams())
        QVERIFY(!stream.metadata().isEmpty());
    for (auto &stream : d->videoStreams())
        QVERIFY(!stream.metadata().isEmpty());
}

QTEST_MAIN(tst_QAVDemuxer)
#include "tst_qavdemuxer.moc"
