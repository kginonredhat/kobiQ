#pragma once

#include <QString>

#ifndef KOBIQ_VERSION
#define KOBIQ_VERSION "unknown"
#endif

inline QString kobiQVersionString()
{
  return QString::fromLatin1(KOBIQ_VERSION);
}

inline QString kobiQVersionedTitle()
{
  return QStringLiteral("kobiQ-%1").arg(kobiQVersionString());
}
