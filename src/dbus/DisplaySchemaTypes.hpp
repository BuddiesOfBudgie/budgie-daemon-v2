#pragma once
#include <qmetatype.h>

#include <QList>

typedef QList<QVector<QVariant>> OutputModesList;
typedef QVariantList             OutputDetailsList;

Q_DECLARE_METATYPE(OutputModesList);
Q_DECLARE_METATYPE(OutputDetailsList);
