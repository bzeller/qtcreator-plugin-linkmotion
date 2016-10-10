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

#include "linkmotionbuildconfigurationfactory.h"
#include "linkmotionbuildplugin_constants.h"
#include "linkmotionbuildstep.h"

#include <qmlprojectmanager/qmlprojectconstants.h>
#include <projectexplorer/buildinfo.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/target.h>
#include <projectexplorer/project.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/buildsteplist.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/qtcassert.h>

using namespace LinkMotion;
using namespace LinkMotion::Internal;

LinkMotionBuildConfigurationFactory::LinkMotionBuildConfigurationFactory(QObject *parent)
    : IBuildConfigurationFactory(parent)
{
    qDebug() << Q_FUNC_INFO;
}

LinkMotionBuildConfigurationFactory::~LinkMotionBuildConfigurationFactory()
{
    qDebug() << Q_FUNC_INFO;
}

int LinkMotionBuildConfigurationFactory::priority(const ProjectExplorer::Target *parent) const
{
    qDebug() << Q_FUNC_INFO;
    if (canHandle(parent))
        return 100;
    return -1;
}

QList<ProjectExplorer::BuildInfo *> LinkMotionBuildConfigurationFactory::availableBuilds(const ProjectExplorer::Target *parent) const
{
    qDebug() << Q_FUNC_INFO;
    if(!canHandle(parent))
        return QList<ProjectExplorer::BuildInfo *>();
    return createBuildInfos(parent->kit(),parent->project()->projectDirectory().toString());
}

int LinkMotionBuildConfigurationFactory::priority(const ProjectExplorer::Kit *k, const QString &projectPath) const
{
    qDebug() << Q_FUNC_INFO;
    return (k && Utils::MimeDatabase().mimeTypeForFile(projectPath)
            .matchesName(QLatin1String(QmlProjectManager::Constants::QMLPROJECT_MIMETYPE))) ? 100 : 100;
}

QList<ProjectExplorer::BuildInfo *> LinkMotionBuildConfigurationFactory::availableSetups(const ProjectExplorer::Kit *k, const QString &projectPath) const
{
    qDebug() << Q_FUNC_INFO;
    return createBuildInfos(k,projectPath);
}

LinkMotionBuildConfiguration *LinkMotionBuildConfigurationFactory::create(ProjectExplorer::Target *parent, const ProjectExplorer::BuildInfo *info) const
{
    qDebug() << Q_FUNC_INFO;
    QTC_ASSERT(info->factory() == this, return 0);
    QTC_ASSERT(info->kitId == parent->kit()->id(), return 0);
    QTC_ASSERT(!info->displayName.isEmpty(), return 0);

    LinkMotionBuildConfiguration *conf = new LinkMotionBuildConfiguration(parent);
    conf->setBuildDirectory(info->buildDirectory);
    conf->setDefaultDisplayName(info->displayName);
    conf->setDisplayName(info->displayName);

    // TODO: Check that this is a linkmotion project
    // then only add the steps
    qDebug() << "INSERTING build step";
    ProjectExplorer::BuildStepList *bs = conf->stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    bs->insertStep(0, new LinkMotionBuildStep(bs));
    return conf;
}

bool LinkMotionBuildConfigurationFactory::canRestore(const ProjectExplorer::Target *parent, const QVariantMap &map) const
{
    qDebug() << Q_FUNC_INFO;
    if (!canHandle(parent))
        return false;
    qDebug() << Q_FUNC_INFO << ProjectExplorer::idFromMap(map);
    return ProjectExplorer::idFromMap(map) == Constants::LINKMOTION_BC_ID;
}

LinkMotionBuildConfiguration *LinkMotionBuildConfigurationFactory::restore(ProjectExplorer::Target *parent, const QVariantMap &map)
{
    qDebug() << Q_FUNC_INFO;
    if (!canRestore(parent,map) )
        return 0;

    LinkMotionBuildConfiguration *conf = new LinkMotionBuildConfiguration(parent);
    if (conf->fromMap(map)) {
        ProjectExplorer::BuildStepList *bs = conf->stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
        bs->insertStep(0, new LinkMotionBuildStep(bs));
        qDebug() << "2";

        return conf;
    }

    qDebug() << "1";

    delete conf;
    return 0;
}

bool LinkMotionBuildConfigurationFactory::canClone(const ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *product) const
{
    qDebug() << Q_FUNC_INFO;

    if (!canHandle(parent))
        return false;
    if (product->id() != Constants::LINKMOTION_BC_ID )
        return false;

    return true;
}

LinkMotionBuildConfiguration *LinkMotionBuildConfigurationFactory::clone(ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *product)
{
    qDebug() << Q_FUNC_INFO;
    if (!canClone(parent,product))
        return 0;
    return new LinkMotionBuildConfiguration(parent,static_cast<LinkMotionBuildConfiguration*>(product));
}

bool LinkMotionBuildConfigurationFactory::canHandle(const ProjectExplorer::Target *t) const
{
    qDebug() << Q_FUNC_INFO;

    return true;
}

QList<ProjectExplorer::BuildInfo *> LinkMotionBuildConfigurationFactory::createBuildInfos(const ProjectExplorer::Kit *k, const QString &projectDir) const
{
    qDebug() << Q_FUNC_INFO;

    QList<ProjectExplorer::BuildInfo *> builds;

    ProjectExplorer::BuildInfo *info = new ProjectExplorer::BuildInfo(this);
    info->buildDirectory = Utils::FileName::fromString(projectDir);

    info->typeName = tr("Qml");
    info->kitId    = k->id();
    info->displayName = tr("Default");

    builds << info;
    qDebug() << Q_FUNC_INFO << info->buildDirectory << info->kitId;
    return builds;
}
