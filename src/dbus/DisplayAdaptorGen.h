/*
 * This file was generated by qdbusxml2cpp version 0.8
 * Source file was DisplaySchema.xml
 *
 * qdbusxml2cpp is Copyright (C) The Qt Company Ltd. and other contributors.
 *
 * This is an auto-generated file.
 * This file may have been hand-edited. Look for HAND-EDIT comments
 * before re-generating it.
 */

#ifndef DISPLAYADAPTORGEN_H
#define DISPLAYADAPTORGEN_H

#include <QtCore/QObject>
#include <QtDBus/QtDBus>
#include "DisplaySchemaTypes.hpp"
#include <QtCore/qcontainerfwd.h>

/*
 * Adaptor class for interface org.buddiesofbudgie.BudgieDaemonX.Displays
 */
class DisplaysAdaptor: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.buddiesofbudgie.BudgieDaemonX.Displays")
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"org.buddiesofbudgie.BudgieDaemonX.Displays\">\n"
"    <method name=\"GetAvailableOutputs\">\n"
"      <annotation value=\"QStringList\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
"      <arg direction=\"out\" type=\"as\" name=\"outputs\"/>\n"
"    </method>\n"
"    <method name=\"GetOutputDetails\">\n"
"      <annotation value=\"OutputDetailsList\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
"      <arg direction=\"out\" type=\"a(ssiiiiddbb)\" name=\"details\"/>\n"
"      <arg direction=\"in\" type=\"s\" name=\"identifier\"/>\n"
"    </method>\n"
"    <method name=\"GetAvailableModes\">\n"
"      <annotation value=\"QString\" name=\"org.qtproject.QtDBus.QtTypeName.In0\"/>\n"
"      <annotation value=\"OutputModesList\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
"      <arg direction=\"in\" type=\"s\" name=\"identifier\"/>\n"
"      <arg direction=\"out\" type=\"a(iid)\" name=\"modes\"/>\n"
"    </method>\n"
"    <method name=\"SetCurrentMode\">\n"
"      <arg direction=\"in\" type=\"s\" name=\"identifier\"/>\n"
"      <arg direction=\"in\" type=\"i\" name=\"width\"/>\n"
"      <arg direction=\"in\" type=\"i\" name=\"height\"/>\n"
"      <arg direction=\"in\" type=\"i\" name=\"refresh\"/>\n"
"      <arg direction=\"in\" type=\"b\" name=\"preferred\"/>\n"
"    </method>\n"
"    <method name=\"SetOutputPosition\">\n"
"      <arg direction=\"in\" type=\"s\" name=\"identifier\"/>\n"
"      <arg direction=\"in\" type=\"i\" name=\"x\"/>\n"
"      <arg direction=\"in\" type=\"i\" name=\"y\"/>\n"
"    </method>\n"
"    <method name=\"SetOutputEnabled\">\n"
"      <arg direction=\"in\" type=\"s\" name=\"identifier\"/>\n"
"      <arg direction=\"in\" type=\"b\" name=\"enabled\"/>\n"
"    </method>\n"
"  </interface>\n"
        "")
public:
    DisplaysAdaptor(QObject *parent);
    ~DisplaysAdaptor() override;

public: // PROPERTIES
public Q_SLOTS: // METHODS
    OutputModesList GetAvailableModes(const QString &identifier);
    QStringList GetAvailableOutputs();
    OutputDetailsList GetOutputDetails(const QString &identifier);
    void SetCurrentMode(const QString &identifier, int width, int height, int refresh, bool preferred);
    void SetOutputEnabled(const QString &identifier, bool enabled);
    void SetOutputPosition(const QString &identifier, int x, int y);
Q_SIGNALS: // SIGNALS
};

#endif
