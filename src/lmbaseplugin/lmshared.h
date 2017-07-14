/*
 * Copyright 2014 Canonical Ltd.
 * Copyright 2017 Link Motion Oy
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; version 2.1.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Juhapekka Piiroinen <juhapekka.piiroinen@canonical.com>
 * Author: Benjamin Zeller <benjamin.zeller@link-motion.com>
 */

#ifndef LMSHARED_INCLUDED_H
#define LMSHARED_INCLUDED_H

#include <QDateTime>
#include <QString>

bool readFile(const QString &fileName, QByteArray *data, QString *errorMessage);
void printToOutputPane(const QString &msg);

#endif // LMSHARED_INCLUDED_H
