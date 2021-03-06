####################################################################
#
# This file is part of the LinkMotion plugins.
#
# License: GNU Lesser General Public License v 2.1
# Author: Juhapekka Piiroinen <juhapekka.piiroinen@link-motion.com>
#
# All rights reserved.
# (C) 2016 Link Motion Oy
####################################################################

TEMPLATE = subdirs
SUBDIRS = src

####################################################################
# This target 'make local' will build and
# install the plugins to $HOME/.local/share/data/
#
# A useful target when you are developing the plugins on your local machine.
####################################################################
QMAKE_EXTRA_TARGETS += local
local.commands += $$[QT_INSTALL_BINS]/qmake -r CONFIG+=BUILDTOHOME;
local.commands += make

OTHER_FILES= $$system(find share)
