#include "linkmotionwizardplugin.h"

#include "linkmotionprojectwizardfactory.h"


using namespace LinkMotion;
using namespace LinkMotion::Internal;

LinkMotionWizardPlugin::LinkMotionWizardPlugin()
{
    qDebug() << Q_FUNC_INFO;

}

LinkMotionWizardPlugin::~LinkMotionWizardPlugin()
{
    qDebug() << Q_FUNC_INFO;

}

bool LinkMotionWizardPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    qDebug() << Q_FUNC_INFO;
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    addAutoReleasedObject(new LinkMotionProjectWizardFactory(QStringLiteral("linkmotion-project"), Core::IWizardFactory::ProjectWizard));
    /*
    addAutoReleasedObject(new LinkMotionDeployStepFactory);
*/
    return true;
}
