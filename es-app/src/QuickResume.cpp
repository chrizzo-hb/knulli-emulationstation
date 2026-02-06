#include "QuickResume.h"
#include <SystemConf.h>
#include "utils/FileSystemUtil.h"
#include "Log.h"

namespace QuickResume
{
    const std::string shutdownFlag = "/var/run/shutdown.flag";

    bool setQuickResume(std::string quickResumeCommand, std::string quickResumePath)
    {
        bool configSaved = false;

        LOG(LogInfo) << "QuickResume::setQuickResume called - path: " << quickResumePath;
        LOG(LogDebug) << "QuickResume::setQuickResume - command: " << quickResumeCommand;

        if (!quickResumeEnabled())
        {
            LOG(LogWarning) << "QuickResume::setQuickResume - Quick Resume is not enabled";
            return false;
        }

        if (quickResumeCommand.empty() || quickResumePath.empty())
        {
            LOG(LogWarning) << "QuickResume::setQuickResume - command or path is empty";
            return false;
        }

        SystemConf::getInstance()->set("global.bootgame.path", quickResumePath);
        SystemConf::getInstance()->set("global.bootgame.cmd", quickResumeCommand);
        configSaved = SystemConf::getInstance()->saveSystemConf();

        LOG(LogInfo) << "QuickResume::setQuickResume - config saved: " << (configSaved ? "true" : "false");

        return configSaved;
    }

    bool clearQuickResume()
    {
        bool configSaved = false;

        if (quickResumeEnabled())
        {
            SystemConf::getInstance()->set("global.bootgame.path", "");
            SystemConf::getInstance()->set("global.bootgame.cmd", "");
            configSaved = SystemConf::getInstance()->saveSystemConf();
        }

        return configSaved;
    }

    bool postLaunchConditionalClean()
    {
        bool configSaved = false;

        if (quickResumeEnabled() && !shutDownInProgress())
        {
            configSaved = clearQuickResume();
        }

        return configSaved;
    }

    bool shutDownInProgress()
    {
        return Utils::FileSystem::exists(shutdownFlag);
    }

    bool quickResumeEnabled()
    {
        return SystemConf::getInstance()->getBool("global.quickresume") == true;
    }
}