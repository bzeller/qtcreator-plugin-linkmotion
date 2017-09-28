/*
 * Copyright 2016 Canonical Ltd.
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
 * Author: Benjamin Zeller <benjamin.zeller@link-motion.com>
 */
#include "containerdevice.h"
#include "containerdevice_p.h"
#include "containerdeviceprocess.h"

#include <lmbaseplugin/lmbaseplugin_constants.h>
#include <lmbaseplugin/lmbaseplugin.h>
#include <lmbaseplugin/settings.h>
#include <lmbaseplugin/lmtargettool.h>

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/taskhub.h>
#include <ssh/sshconnection.h>
#include <utils/portlist.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QTimer>

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

namespace LmBase {
namespace Internal {

ContainerDevicePrivate::ContainerDevicePrivate(ContainerDevice *q)
    : QObject(nullptr)
    , q_ptr(q)
    , m_detectionProcess(nullptr)
{

}

void ContainerDevicePrivate::resetProcess()
{
    if (m_detectionProcess) {
        m_detectionProcess->disconnect(this);
        if (m_detectionProcess->state() != QProcess::NotRunning)
            m_detectionProcess->kill();
        delete m_detectionProcess;
        m_detectionProcess = nullptr;
    }
    m_detectionProcess = new QProcess(this);
    connect(m_detectionProcess, SIGNAL(finished(int)), this, SLOT(handleDetectionStepFinished()));
}

QString ContainerDevicePrivate::userName() const
{
    return LinkMotionTargetTool::targetDefaultUser(q_ptr->containerName());
}

void ContainerDevicePrivate::reset()
{
    Q_Q(ContainerDevice);

    m_deviceState = Initial;
    ProjectExplorer::DeviceManager::instance()->setDeviceState(q->id(), ProjectExplorer::IDevice::DeviceDisconnected);

    handleDetectionStepFinished();
}

void ContainerDevicePrivate::showWarningMessage(const QString &msg)
{
    ProjectExplorer::TaskHub::addTask(ProjectExplorer::Task::Error, msg, Constants::LM_TASK_CATEGORY_DEVICE);
}

void ContainerDevicePrivate::handleDetectionStepFinished()
{
    Q_Q(ContainerDevice);

    switch(m_deviceState) {
        case Initial: {
            m_deviceState = GetStatus;

            resetProcess();

            QString tool = LinkMotionBasePlugin::lmTargetTool();
            if (tool.isEmpty()) {
                showWarningMessage(tr("Could not find lmsdk-target. Container backend will not work."));
                triggerDeviceRedetection();
                return;
            }

            m_detectionProcess->setProgram(tool);
            m_detectionProcess->setArguments(QStringList{
                QStringLiteral("status"),
                q->containerName()
            });
            break;
        }
        case GetStatus: {
            if ((m_detectionProcess->exitStatus() != QProcess::NormalExit || m_detectionProcess->exitStatus() != 0)) {
                printProcessError();
                resetProcess();
                triggerDeviceRedetection();
                return;
            }

            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson( m_detectionProcess->readAllStandardOutput(), &err);
            if (err.error != QJsonParseError::NoError) {
                showWarningMessage(tr("There was a error in the device detection of %1, it was not possible to parse the status:\n%2")
                                   .arg(q->containerName())
                                   .arg(err.errorString()));
                triggerDeviceRedetection();
                return;
            }

            if (!doc.isObject()) {
                showWarningMessage(tr("There was a error in the device detection of %1, the returned format was not a JSON object.")
                                   .arg(q->containerName()));
                triggerDeviceRedetection();
                return;
            }

            QVariantMap obj = doc.object().toVariantMap();
            if (!obj.contains(QStringLiteral("ipv4"))) {
                showWarningMessage(tr("There was a error in the device detection of %1, no IP address was returned.")
                                   .arg(q->containerName()));
                triggerDeviceRedetection();
                return;
            }

            m_deviceIP = obj[QStringLiteral("ipv4")].toString();
            m_deviceState = DeployKey;

            resetProcess();
            m_detectionProcess->setProgram(QString::fromLatin1(Constants::LM_CONTAINER_DEPLOY_PUBKEY_SCRIPT)
                                           .arg(Constants::LM_SCRIPTPATH));
            m_detectionProcess->setArguments(QStringList{q->containerName()});
            break;
        }
        case DeployKey: {
            if ((m_detectionProcess->exitStatus() != QProcess::NormalExit || m_detectionProcess->exitStatus() != 0)) {
                printProcessError();
                resetProcess();

                triggerDeviceRedetection();
                return;
            }

            m_deviceState = Finished;

            QSsh::SshConnectionParameters params = q->sshParameters();
            params.userName = userName();
            params.host = m_deviceIP;
            params.authenticationType = QSsh::SshConnectionParameters::AuthenticationTypePublicKey;
            params.timeout = 20;
            params.port = 22;
            params.privateKeyFile = Settings::settingsPath()
                    .appendPath(QLatin1String(Constants::LM_DEVICE_SSHIDENTITY))
                    .toString();

            q->setSshParameters(params);
            ProjectExplorer::DeviceManager::instance()->setDeviceState(q->id(), ProjectExplorer::IDevice::DeviceReadyToUse);
            return;
        }
        default: {
            break;
        }
    }

    m_detectionProcess->start();
    if (!m_detectionProcess->waitForStarted(3000)) {
        showWarningMessage(tr("Error while detecting the device state of %1.\n%2 %3")
                           .arg(q->containerName())
                           .arg(m_detectionProcess->program())
                           .arg(m_detectionProcess->arguments().join(QStringLiteral(" "))));
        resetProcess();
        triggerDeviceRedetection();
    }
}

void ContainerDevicePrivate::triggerDeviceRedetection()
{
    //trigger device redetection
    QTimer::singleShot(1000, this, &ContainerDevicePrivate::reset);
}

void ContainerDevicePrivate::printProcessError()
{
    QString message = tr("There was a error in the device detection, it will not be possible to run apps on it:\n%1\n%2")
            .arg(QString::fromLocal8Bit(m_detectionProcess->readAllStandardOutput()))
            .arg(QString::fromLocal8Bit(m_detectionProcess->readAllStandardError()));
    showWarningMessage(message);
}

ContainerDevice::ContainerDevice(Core::Id type, Core::Id id) :
    LinuxDevice(type.suffixAfter(Constants::LM_CONTAINER_DEVICE_TYPE_ID),
                type,
                ProjectExplorer::IDevice::Hardware,
                ProjectExplorer::IDevice::AutoDetected,
                id),
    d_ptr(new ContainerDevicePrivate(this))
{
    setDisplayName(QCoreApplication::translate("LmBase::Internal::ContainerDevice"
                                               , "Link Motion Desktop Device (%1)").arg(containerName()));
    setDeviceState(IDevice::DeviceDisconnected);

    const QString portRange =
            QString::fromLatin1("%1-%2")
            .arg(Constants::LM_DESKTOP_PORT_START)
            .arg(Constants::LM_DESKTOP_PORT_END);
    setFreePorts(Utils::PortList::fromString(portRange));

    d_ptr->reset();
}

ContainerDevice::ContainerDevice(const ContainerDevice &other)
    : LinuxDevice(other)
    , d_ptr(new ContainerDevicePrivate(this))
{
    //no need to copy over the private, just redetect the device status
    d_ptr->reset();
}

ContainerDevice::Ptr ContainerDevice::create(Core::Id type, Core::Id id)
{
    return ContainerDevice::Ptr(new ContainerDevice(type, id));
}

ContainerDevice::~ContainerDevice()
{
    delete d_ptr;
}

Core::Id ContainerDevice::createIdForContainer(const QString &name)
{
    return Core::Id(Constants::LM_CONTAINER_DEVICE_TYPE_ID).withSuffix(name);
}

QString ContainerDevice::containerName() const
{
    return id().suffixAfter(Constants::LM_CONTAINER_DEVICE_TYPE_ID);
}

Utils::FileName ContainerDevice::westonConfig() const
{
    Utils::FileName rootfsDir = Utils::FileName::fromString(
                LinkMotionTargetTool::targetBasePath(QDir::cleanPath(containerName())));
    if (!rootfsDir.exists()) {
        return Utils::FileName();
    }

    return rootfsDir.appendPath("weston.ini");
}

ProjectExplorer::IDeviceWidget *ContainerDevice::createWidget()
{
    if (!qgetenv("LMSDK_SHOW_DEVICE_WIDGET").isEmpty())
        return LinuxDevice::createWidget();
    return 0;
}

QList<Core::Id> ContainerDevice::actionIds() const
{
    return QList<Core::Id>();
}

void ContainerDevice::executeAction(Core::Id actionId, QWidget *parent)
{
    Q_UNUSED(actionId);
    Q_UNUSED(parent);
}

ProjectExplorer::IDevice::Ptr ContainerDevice::clone() const
{
    return IDevice::Ptr(new ContainerDevice(*this));
}

QString ContainerDevice::displayNameForActionId(Core::Id actionId) const
{
    Q_UNUSED(actionId);
    return QString();
}

QString ContainerDevice::displayType() const
{
    return tr("Link Motion Desktop Device");
}

ProjectExplorer::DeviceProcess *ContainerDevice::createProcess(QObject *parent) const
{
    return new ContainerDeviceProcess(sharedFromThis(), parent);
}

} // namespace Internal
} // namespace LmBase

