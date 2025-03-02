#pragma once
#include <QList>
#include <qmetatype.h>

typedef QList<QVector<QVariant>> OutputModesList;
typedef QVariantList        OutputDetailsList;

Q_DECLARE_METATYPE(OutputModesList);
Q_DECLARE_METATYPE(OutputDetailsList);
