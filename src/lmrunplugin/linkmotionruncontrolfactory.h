/*####################################################################
#
# This file is part of the LinkMotion Run plugin.
#
# License: Proprietary
# Author: Juhapekka Piiroinen <juhapekka.piiroinen@link-motion.com>
#
# All rights reserved.
# (C) 2016 Link Motion Oy
####################################################################*/
#ifndef LINKMOTIONRUNCONTROLFACTORY_H
#define LINKMOTIONRUNCONTROLFACTORY_H

#include <QObject>
#include <projectexplorer/runconfiguration.h>

namespace LinkMotion {
namespace Internal {

class LinkMotionRunControlFactory : public ProjectExplorer::IRunControlFactory
{
    Q_OBJECT

public:
    LinkMotionRunControlFactory(QObject *parent = 0);

    bool canRun(ProjectExplorer::RunConfiguration *runConfiguration,
                Core::Id mode) const;
    ProjectExplorer::RunControl *create(ProjectExplorer::RunConfiguration *runConfiguration,
                       Core::Id mode,
                       QString *errorMessage);
};

}
}

#endif // LINKMOTIONRUNCONTROLFACTORY_H
