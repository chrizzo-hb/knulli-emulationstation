#include "SyncthingWatcher.h"
#include "LocaleES.h"
#include "Log.h"
#include <algorithm>

#define GUIICON _U("\uF07C ")

SyncthingWatcher::SyncthingWatcher(Window* window) : mWindow(window), mSyncthingUtil(SyncthingUtil::getInstance()) {
}

bool SyncthingWatcher::enabled() {
	return mSyncthingUtil.isEnabled();
}

bool SyncthingWatcher::check() {
	if (!SyncthingUtil::isEnabled() || !mSyncthingUtil.isConnected()) {
		if (wndNotification != nullptr) {
			wndNotification->close();
			wndNotification = nullptr;
		}
		return false;
	}

	SyncthingState state = mSyncthingUtil.getState();
	std::vector<std::string> syncedDevices;

	// Check if any devices have become dirty or are no longer dirty
	if (state.dirtyDevices.size() > 0) {
		for (const auto& dev : mDirtyDevices) {
			if (std::find(state.dirtyDevices.begin(), state.dirtyDevices.end(), dev) == state.dirtyDevices.end()) {
				LOG(LogError) << "Syncthing: Device " << dev << " is no longer dirty.";
				mDirtyDevices.erase(std::remove(mDirtyDevices.begin(), mDirtyDevices.end(), dev), mDirtyDevices.end());
				syncedDevices.push_back(dev);
			}
		}
		for (const auto& dev : state.dirtyDevices) {
			if (std::find(mDirtyDevices.begin(), mDirtyDevices.end(), dev) == mDirtyDevices.end()) {
				LOG(LogError) << "Syncthing: Device " << dev << " is now dirty.";
				mDirtyDevices.push_back(dev);
			}
		}
	}

	if (state.isSyncing()) {
		mStateUpdateCounter++;

		if (wndNotification == nullptr && mWindow != nullptr) {
			LOG(LogError) << "Syncthing: opened notification window at state itemsSynced=" << state.itemsSynced << " itemsTotal=" << state.itemsTotal << " transferSpeed=" << state.transferSpeed;
			mStateUpdateCounter = 1;
			mCurrentTransferNeededFiles = state.itemsTotal - state.itemsSynced;
			wndNotification = mWindow->createAsyncNotificationComponent();
			wndNotification->updateTitle(GUIICON + _("SYNCTHING"));
		}
		
		int currentTransferTransferredFiles = mCurrentTransferNeededFiles - (state.itemsTotal - state.itemsSynced);
		
		if (mCurrentTransferNeededFiles > 0) {
			std::string idx = std::to_string(currentTransferTransferredFiles) + "/" + std::to_string(mCurrentTransferNeededFiles);
			int percentDone = (currentTransferTransferredFiles * 100) / mCurrentTransferNeededFiles;
			wndNotification->updateText(_("Transferring file") + " " + idx);
			wndNotification->updatePercent(percentDone);
		}
		return true;
	} else {
		if (wndNotification != nullptr) {
			// If we were just syncing, show the finished message
			if (mCurrentTransferNeededFiles > 0) {
				if (syncedDevices.size() == 0) {
					wndNotification->updateText(_("Finished synchronization."));
				} else {
					wndNotification->updateText(_("Finished synchronization with") + " " + toSyncedDevicesNameString(syncedDevices) + ".");
				}
				wndNotification->updatePercent(100);
				mCurrentTransferNeededFiles = 0; 
			} else {
				// Otherwise close the window after some time
				LOG(LogError) << "Syncthing: closed notification window after " << mStateUpdateCounter << " iterations.";
				wndNotification->close();
				wndNotification = nullptr;
			}
		} else if (syncedDevices.size() > 0 && mWindow != nullptr) {
			// If we weren't syncing but a device just finished, show finished message
			wndNotification = mWindow->createAsyncNotificationComponent();
			wndNotification->updateTitle(GUIICON + _("SYNCTHING"));
			wndNotification->updateText(_("Finished synchronization with") + " " + toSyncedDevicesNameString(syncedDevices) + ".");
			wndNotification->updatePercent(100);
		}
	}

	return false;
}

std::string SyncthingWatcher::toSyncedDevicesNameString(const std::vector<std::string>& deviceNames) {
	std::string result;
	for (size_t i = 0; i < deviceNames.size(); i++) {
		result += deviceNames[i];
		if (i < deviceNames.size() - 1) {
			result += ", ";
		}
	}
	return result;
}