/*####################################################################
#
# This file is part of the LinkMotion Build plugin.
#
# License: Proprietary
# Author: Juhapekka Piiroinen <juhapekka.piiroinen@link-motion.com>
#
# All rights reserved.
# (C) 2016 Link Motion Oy
####################################################################*/
#ifndef LINKMOTIONBUILDSETTINGSWIDGET_H
#define LINKMOTIONBUILDSETTINGSWIDGET_H

#include <QObject>
#include <QDebug>

#include <utils/pathchooser.h>
#include <projectexplorer/namedwidget.h>
#include <projectexplorer/buildconfiguration.h>

#include "linkmotionbuildconfiguration.h"

namespace LinkMotion {
namespace Internal {

class LinkMotionBuildSettingsWidget : public ProjectExplorer::NamedWidget
{
    Q_OBJECT

public:
    explicit LinkMotionBuildSettingsWidget(LinkMotionBuildConfiguration *conf, QWidget *parent = 0);

    QString username() { return m_buildConfiguration->m_username; }
    QString password() { return m_buildConfiguration->m_password; }
    QString device() { return m_buildConfiguration->m_device; }

public slots:
    void onUsernameChanged(QString username);
    void onPasswordChanged(QString password);
    void onDeviceChanged(QString device);

private slots:
    void updateBuildDirectory() const;

private:
    LinkMotionBuildConfiguration *m_buildConfiguration;

    QLineEdit* m_username;
    QLineEdit* m_password;
    QLineEdit* m_device;
};
}
}

#endif // LINKMOTIONBUILDSETTINGSWIDGET_H