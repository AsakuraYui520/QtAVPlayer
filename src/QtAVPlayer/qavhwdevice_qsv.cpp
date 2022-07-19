/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavhwdevice_qsv_p.h"
#include "qavvideobuffer_gpu_p.h"

QT_BEGIN_NAMESPACE

QAVHWDevice_QSV::QAVHWDevice_QSV(QObject *parent)
    : QObject(parent)
{
}

AVPixelFormat QAVHWDevice_QSV::format() const
{
    return AV_PIX_FMT_QSV;
}

AVHWDeviceType QAVHWDevice_QSV::type() const
{
    return AV_HWDEVICE_TYPE_QSV;
}

QAVVideoBuffer *QAVHWDevice_QSV::videoBuffer(const QAVVideoFrame &frame) const
{
    return new QAVVideoBuffer_GPU(frame);
}

QT_END_NAMESPACE
