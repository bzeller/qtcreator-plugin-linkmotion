/*
 * Copyright 2013 Canonical Ltd.
 * Copyright 2017 Link Motion Oy.
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
 * Authors:
 * Juhapekka Piiroinen <juhapekka.piiroinen@canonical.com>
 * Benjamin Zeller <benjamin.zeller@link-motion.com>
 */

#include "lmbaseplugin.h"
#include "lmbaseplugin_constants.h"
#include "lmtargettool.h"
#include "lmkitmanager.h"
#include "lmtoolchain.h"
#include "lmqtversion.h"
#include "lmwelcomepage.h"
#include "processoutputdialog.h"

#include <lmbaseplugin/lmsettingstargetpage.h>

#include <lmbaseplugin/device/container/containerdevicefactory.h>
#include <lmbaseplugin/device/container/lmlocalrunconfigurationfactory.h>
#include <lmbaseplugin/device/container/lmlocaldeployconfiguration.h>
#if 0
#include "ubuntudevicesmodel.h"
#include "localportsmanager.h"
#include "ubuntuqmlbuildconfiguration.h"
#include "ubuntueditorfactory.h"
#include "ubuntutestcontrol.h"
#include "ubuntupackageoutputparser.h"
#include "ubuntuprojecthelper.h"
#include "ubuntuscopefinalizer.h"
#include "targetupgrademanager.h"
#include "ubuntusettingsdeviceconnectivitypage.h"
#include "ubuntusettingsclickpage.h"
#include "ubuntusettingsprojectdefaultspage.h"
#include "processoutputdialog.h"


#include <ubuntu/device/container/ubuntulocalruncontrolfactory.h>

#include <ubuntu/device/remote/ubunturemoteruncontrolfactory.h>
#include <ubuntu/device/remote/ubuntudevicefactory.h>
#include <ubuntu/device/remote/ubunturemotedeployconfiguration.h>
#include <ubuntu/device/remote/ubuntudeploystepfactory.h>

#include <ubuntu/wizards/ubuntuprojectapplicationwizard.h>
#include <ubuntu/wizards/ubuntufirstrunwizard.h>
#include <ubuntu/wizards/ubuntuprojectmigrationwizard.h>
#include <ubuntu/ubuntuversion.h>

#include "ubuntujsextension.h"

#endif

#include <coreplugin/modemanager.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/processparameters.h>
#include <coreplugin/featureprovider.h>
#include <coreplugin/coreplugin.h>
#include <coreplugin/jsexpander.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/mimetypes/mimeglobpattern_p.h>
#include <cmakeprojectmanager/cmaketoolmanager.h>

#include <qmakeprojectmanager/qmakenodes.h>
#include <qmakeprojectmanager/qmakeproject.h>
#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h>

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <QFileInfo>
#include <QGuiApplication>
#include <QtQml>
#include <QFile>
#include <QAction>
#include <QMessageBox>
#include <QCheckBox>

#include <coreplugin/icore.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>


using namespace LmBase;
using namespace LmBase::Internal;

static void criticalError (const QString &err)
{
    QMessageBox::critical(Core::ICore::mainWindow(), qApp->applicationName(), err);
    qCritical("%s", qPrintable(err));

    //the Qt event loop does not stop so we force it
    exit(1);
}

LinkMotionBasePlugin::LinkMotionBasePlugin()
{
}

LinkMotionBasePlugin::~LinkMotionBasePlugin()
{
}

bool LinkMotionBasePlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    if (::getuid() == 0) {
        criticalError(tr("\nThe Link Motion SDK can not be used as superuser."));
        return false;
    }
    if (QStandardPaths::findExecutable(QStringLiteral("lxc-start")).isEmpty()) {
        criticalError(tr("\nLxc is not installed properly.\nIt is required for the Link Motion SDK IDE to work."));
        return false;
    }

    if (LinkMotionBasePlugin::lmTargetWrapper().isEmpty()) {
        criticalError(tr("\nlmsdk-wrapper was not found in PATH."));
        return false;
    }

    if (LinkMotionBasePlugin::lmTargetTool().isEmpty()) {
        criticalError(tr("\nlmsdk-target was not found in PATH."));
        return false;
    }

    Utils::FileName toolName = Utils::FileName::fromString(LinkMotionBasePlugin::lmTargetWrapper());
    QString toolsDir = toolName.parentDir().toString();

    Utils::Environment env = Utils::Environment::systemEnvironment();
    if (!env.path().contains(toolsDir))
        qputenv("PATH",
                QString::fromLatin1("%1:%2")
                .arg(toolsDir)
                .arg(env.value(QStringLiteral("PATH"))).toLocal8Bit());

    m_settings.restoreSettings();

    if(!checkContainerSetup()) {
        if(errorString)
            *errorString = tr("Initializing the container backend failed");
        return false;
    }

    // welcome page plugin
    addAutoReleasedObject(new LinkMotionWelcomePage);

    //register Qt version
    addAutoReleasedObject(new LinkMotionQtVersionFactory);

    // Build support
    addAutoReleasedObject(new LinkMotionToolChainFactory);

    CMakeProjectManager::CMakeToolManager::registerAutodetectionHelper([](){
        QList<CMakeProjectManager::CMakeTool *> found;

        QList<LinkMotionTargetTool::Target> targets = LinkMotionTargetTool::listAvailableTargets();
        foreach (const LinkMotionTargetTool::Target &t, targets) {
            CMakeProjectManager::CMakeTool *tool = LinkMotionKitManager::createCMakeTool(t);
            if (tool)
                found.append(tool);
        }

        return found;
    });

    //trigger kit autodetection and update after projectexplorer loaded the kits
    connect(ProjectExplorer::KitManager::instance(),SIGNAL(kitsLoaded())
            ,this,SLOT(onKitsLoaded()));

    //settings
    addAutoReleasedObject(new LinkMotionSettingsTargetPage);

    //device support
    addAutoReleasedObject(new ContainerDeviceFactory);
    addAutoReleasedObject(new LinkMotionLocalRunConfigurationFactory);
    addAutoReleasedObject(new LinkMotionLocalDeployConfigurationFactory);
    //addAutoReleasedObject(new UbuntuLocalPortsManager);

#if 0
    qmlRegisterUncreatableType<UbuntuQmlDeviceConnectionState>("Ubuntu.DevicesModel",0,1,"DeviceConnectionState",QStringLiteral("Not instantiable"));
    qmlRegisterUncreatableType<UbuntuQmlDeviceDetectionState>("Ubuntu.DevicesModel",0,1,"DeviceDetectionState",QStringLiteral("Not instantiable"));
    qmlRegisterUncreatableType<UbuntuQmlFeatureState>("Ubuntu.DevicesModel",0,1,"FeatureState",QStringLiteral("Not instantiable"));
    qmlRegisterUncreatableType<UbuntuQmlDeviceMachineType>("Ubuntu.DevicesModel",0,1,"DeviceMachineType",QStringLiteral("Not instantiable"));

    Utils::MimeDatabase::addMimeTypes(QLatin1String(Constants::LM_MIMETYPE_XML));

    m_ubuntuDeviceMode = new UbuntuDeviceMode();
    addAutoReleasedObject(m_ubuntuDeviceMode);

    m_ubuntuPackagingMode = new UbuntuPackagingMode();
    addAutoReleasedObject(m_ubuntuPackagingMode);

    addAutoReleasedObject(new UbuntuSettingsProjectDefaultsPage);
    addAutoReleasedObject(new UbuntuSettingsDeviceConnectivityPage);

    // Handle new project type files
    addAutoReleasedObject(new UbuntuProjectManager);
    addAutoReleasedObject(new UbuntuRemoteRunControlFactory);
    addAutoReleasedObject(new UbuntuLocalRunControlFactory);

    //ubuntu device support
    addAutoReleasedObject(new UbuntuDeviceFactory);

    //deploy support
    addAutoReleasedObject(new UbuntuRemoteDeployConfigurationFactory);
    addAutoReleasedObject(new UbuntuClickReviewTaskHandler);

    //register wizards
    addAutoReleasedObject(
                new UbuntuWizardFactory<UbuntuProjectApplicationWizard,UbuntuProjectApplicationWizard::CMakeProject>(
                    QStringLiteral("ubuntu-project-cmake"),
                    Core::IWizardFactory::ProjectWizard));
    addAutoReleasedObject(
                new UbuntuWizardFactory<UbuntuProjectApplicationWizard,UbuntuProjectApplicationWizard::QMakeProject>(
                    QStringLiteral("ubuntu-project-qmake"),
                    Core::IWizardFactory::ProjectWizard));
    addAutoReleasedObject(
                new UbuntuWizardFactory<UbuntuProjectApplicationWizard,UbuntuProjectApplicationWizard::UbuntuHTMLProject>(
                    QStringLiteral("ubuntu-project-plain-html"),
                    Core::IWizardFactory::ProjectWizard));
    addAutoReleasedObject(
                new UbuntuWizardFactory<UbuntuProjectApplicationWizard,UbuntuProjectApplicationWizard::UbuntuQMLProject>(
                    QStringLiteral("ubuntu-project-plain-qml"),
                    Core::IWizardFactory::ProjectWizard));
    addAutoReleasedObject(
                new UbuntuWizardFactory<UbuntuProjectApplicationWizard,UbuntuProjectApplicationWizard::GoProject>(
                    QStringLiteral("ubuntu-project-go"),
                    Core::IWizardFactory::ProjectWizard));


    addAutoReleasedObject(new UbuntuDeployStepFactory);

    addAutoReleasedObject(new Internal::UbuntuManifestEditorFactory);
    addAutoReleasedObject(new Internal::UbuntuApparmorEditorFactory);

    //Ubuntu expander
    Core::JsExpander::registerQObjectForJs(QStringLiteral("Ubuntu"), new UbuntuJsExtension);


    const Core::Context qmakeProjectContext(QmakeProjectManager::Constants::PROJECT_ID);

    Core::ActionContainer *mproject =
            Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_PROJECTCONTEXT);
    Core::ActionContainer *msubproject =
            Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_SUBPROJECTCONTEXT);

    #endif

    return true;
}

void LinkMotionBasePlugin::extensionsInitialized()
{
    ProjectExplorer::TaskHub::addCategory(Constants::LM_TASK_CATEGORY_DEVICE,
                         tr("LinkMotion", "Category for ubuntu device issues listed under 'Issues'"));
#if 0
    if (m_ubuntuMenu) m_ubuntuMenu->initialize();
    m_ubuntuDeviceMode->initialize();
    m_ubuntuPackagingMode->initialize();

    //add the create click package menu item to the project context menu
    Core::ActionContainer *mproject =
            Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_PROJECTCONTEXT);
    if(mproject) {
        Core::Command *comm = Core::ActionManager::command("Ubuntu.Build.CreateClickPackage");
        if(comm)
            mproject->addAction(comm, ProjectExplorer::Constants::G_PROJECT_BUILD);

        comm = Core::ActionManager::command("Ubuntu.Build.CreateManifest");
        if(comm)
            mproject->addAction(comm, ProjectExplorer::Constants::G_PROJECT_BUILD);
    }

    //add ubuntu testcontrol to the object tree
    new UbuntuTestControl(Core::ICore::mainWindow());
#endif
}

static QString findToolInPathOrAppDir (const QString &tool)
{
    QString lmsdkTool = QStandardPaths::findExecutable(tool);
    if (!lmsdkTool.isEmpty()){return lmsdkTool;}

    Utils::FileName fName = Utils::FileName::fromString(QCoreApplication::applicationDirPath());
    fName.appendPath(tool);
    if (fName.exists()) {
        lmsdkTool = fName.toString();
    }

    return lmsdkTool;
}

QString LinkMotionBasePlugin::lmTargetTool()
{
    static QString lmsdkTarget;
    if (lmsdkTarget.isEmpty())
        lmsdkTarget = findToolInPathOrAppDir(QStringLiteral("lmsdk-target"));
    return lmsdkTarget;
}

QString LinkMotionBasePlugin::lmTargetWrapper()
{
    static QString lmsdkTarget;
    if (lmsdkTarget.isEmpty())
        lmsdkTarget = findToolInPathOrAppDir(QStringLiteral("lmsdk-wrapper"));
    return lmsdkTarget;
}

void LinkMotionBasePlugin::onKitsLoaded()
{
    LinkMotionKitManager::autoDetectKits();
    disconnect(ProjectExplorer::KitManager::instance(),SIGNAL(kitsLoaded())
               ,this,SLOT(onKitsLoaded()));

    showFirstStartWizard();

}

void LinkMotionBasePlugin::showFirstStartWizard()
{
#if 0
    QString file = m_settings.settingsPath().appendPath(QStringLiteral("firstrun")).toString();

    if(!QFile::exists(file)) {
        UbuntuFirstRunWizard wiz(Core::ICore::mainWindow());
        if( wiz.exec() == QDialog::Accepted ) {
            if (wiz.field(QStringLiteral("createEmulator")).toBool()) {
                Core::ModeManager::activateMode(LmBase::Constants::LM_MODE_DEVICES);

                //invoke the method the next time the event loop starts
                QMetaObject::invokeMethod(m_ubuntuDeviceMode,"showAddEmulatorDialog",Qt::QueuedConnection);
            }
        }

        if(wiz.field(QStringLiteral("disableWizard")).toBool()) {
            QFile f(file);
            if(f.open(QIODevice::WriteOnly)) {
                f.write("1");
                f.close();
            }
        }
    }
#endif
}

void LinkMotionBasePlugin::updateContextMenu(ProjectExplorer::Project *project, ProjectExplorer::Node *node)
{
}

bool LinkMotionBasePlugin::checkContainerSetup()
{
    enum {
        ERR_NONE         = 0,
        ERR_NO_ACCESS    = 255,
        ERR_NEEDS_FIXING = 254,
        ERR_NO_BRIDGE    = 253,
        ERR_NO_LXC       = 252,
        ERR_NO_SETUP     = 251
        //ERR_UNKNOWN      = 200
    };

    bool ok = false;

    while (!ok) {
        QProcess proc;
        qDebug()<<"Running "<<LinkMotionBasePlugin::lmTargetTool();
        proc.setProgram(LinkMotionBasePlugin::lmTargetTool());

        QStringList args{QStringLiteral("initialized")};
        if(!Settings::askForContainerSetup())
            args.append(QStringLiteral("-b"));

        proc.setArguments(args);
        proc.start();
        if (!proc.waitForFinished()) {
            criticalError(tr("The container backend setup detection failed.\nThe detection tool did not return in time.\nPlease try again."));
        }
        if (proc.exitStatus() != QProcess::NormalExit) {
            criticalError(tr("The container backend setup detection failed.\nPlease try again."));
        }

        switch(proc.exitCode()) {
            case ERR_NONE:
                ok = true;
                break;
            case ERR_NO_ACCESS:
                    //the tool tells us that we have no access to the LXD server
                    criticalError(tr("The current user can not access the LXD server which is required for the Link Motion SDK.\n"
                                     "Make sure the user is part of the lxd group, relogin and restart the IDE."));
                break;
            case ERR_NO_LXC:
                //the tool tells us that we have no LXC installed
                criticalError(tr("LXC is not installed properly.\nMake sure to install LXC and restart the Link Motion SDK."));
                break;
            //@FIXME those two states should be handled seperately
            case ERR_NO_BRIDGE:
            case ERR_NO_SETUP:
                if (Settings::askForContainerSetup()) {
                    QString text = tr("The container backend is not initialized.\n\n"
                                      "Create default configuration?\n"
                                      "Not setting up the container configuration will\nmake it impossible to run applications locally."
                                      );

                    QMessageBox box(QMessageBox::Question, qApp->applicationName(),text, QMessageBox::Yes | QMessageBox::No | QMessageBox::Abort, Core::ICore::mainWindow());
                    QCheckBox *check = new QCheckBox(&box);
                    check->setText(tr("Do not show again"));
                    check->setChecked(false);
                    box.setCheckBox(check);

                    int choice = box.exec();
                    Settings::setAskForContainerSetup(check->checkState() != Qt::Checked);
                    Settings::flushSettings();

                    if (choice == QMessageBox::Yes) {
                        QString arguments = Utils::QtcProcess::joinArgs(QStringList{
                            LinkMotionBasePlugin::lmTargetTool(),
                            QStringLiteral("autosetup"),
                            QStringLiteral("-y")
                        });

                        ProjectExplorer::ProcessParameters params;
                        params.setCommand(QLatin1String(Constants::LM_SUDO_BINARY));
                        params.setEnvironment(Utils::Environment::systemEnvironment());
                        params.setArguments(arguments);

                        int res = ProcessOutputDialog::runProcessModal(params, Core::ICore::mainWindow());
                        if (res != 0) {
                            criticalError(tr("Setting up the container backend failed."));
                        }
                    } else if (choice == QMessageBox::Abort) {
                        criticalError(tr("Container backend initialization was cancelled."));
                    }
                }
                break;
            case ERR_NEEDS_FIXING: {
                QString text = tr("The container backend detection indicated problems.\n\n"
                                  "Fix them automatically?\n"
                                  );

                QMessageBox box(QMessageBox::Question, qApp->applicationName(),text, QMessageBox::Yes | QMessageBox::Abort, Core::ICore::mainWindow());
                int choice = box.exec();
                if (choice == QMessageBox::Yes) {
                    QString arguments = Utils::QtcProcess::joinArgs(QStringList{
                        LinkMotionBasePlugin::lmTargetTool(),
                        QStringLiteral("autofix")
                    });

                    ProjectExplorer::ProcessParameters params;
                    params.setCommand(QLatin1String(Constants::LM_SUDO_BINARY));
                    params.setEnvironment(Utils::Environment::systemEnvironment());
                    params.setArguments(arguments);

                    int res = ProcessOutputDialog::runProcessModal(params, Core::ICore::mainWindow());
                    if (res != 0) {
                        criticalError(tr("Fixing the container backend failed."));
                    }
                } else if (choice == QMessageBox::Abort) {
                    criticalError(tr("Automatic container backend fixing was cancelled."));
                }
                break;
            }
            default:
                criticalError(tr("The container backend returned an unknown error status."));
                break;
        }
    }

    QProcess proc;
    proc.setProgram(QString::fromLatin1("%0/qtc_initialize_sound").arg(Constants::LM_SCRIPTPATH));
    proc.start();
    if (!proc.waitForFinished()) {
        QMessageBox::warning(Core::ICore::mainWindow(),
                              qApp->applicationName(),
                              tr("Initializing the sound backend did time out.\nPlaying sound from containers may not work."));
    } else if (proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0) {
        QMessageBox::warning(Core::ICore::mainWindow(),
                              qApp->applicationName(),
                              tr("Initializing the sound backend failed.\nPlaying sound from containers may not work."));
    }

    return true;
}
