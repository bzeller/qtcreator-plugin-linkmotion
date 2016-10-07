#ifndef LINKMOTIONBUILDSTEPCONFIGWIDGET_H
#define LINKMOTIONBUILDSTEPCONFIGWIDGET_H

#include <projectexplorer/buildstep.h>

#include <QObject>

namespace LinkMotion {
namespace Internal {


class LinkMotionBuildStepConfigWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT

public:
    LinkMotionBuildStepConfigWidget(ProjectExplorer::BuildStep* step);
    virtual QString summaryText() const { return QStringLiteral("summaryTextHere"); }
    virtual QString displayName() const { return QStringLiteral("displayNamehere"); }

protected:
    ProjectExplorer::BuildStep* m_step;

};

}
}

#endif // LINKMOTIONBUILDSTEPCONFIGWIDGET_H
