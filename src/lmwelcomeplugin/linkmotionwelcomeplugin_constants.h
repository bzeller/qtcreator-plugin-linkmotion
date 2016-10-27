/*####################################################################
#
# This file is part of the LinkMotion Welcome plugin.
#
# License: Proprietary
# Author: Juhapekka Piiroinen <juhapekka.piiroinen@link-motion.com>
#
# All rights reserved.
# (C) 2016 Link Motion Oy
####################################################################*/

#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QString>

namespace LinkMotion {
namespace Welcome {
namespace Constants {

const QString LINKMOTION_WELCOMESCREEN_QML = QLatin1String("qrc:/linkmotion/qml/main.qml");
const int  P_MODE_LINKMOTION          = 100;
const char C_LINKMOTION_MODE[]        = "LinkMotion.WelcomeMode";
const char MODE_LINKMOTION[]          = "LinkMotion";

}
}
}
#endif // CONSTANTS_H
