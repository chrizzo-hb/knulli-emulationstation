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
		return true;
	}

	int lastTotalBytesTransferred = mTotalBytesTransferred;

	SyncthingState state = mSyncthingUtil.getState();
	std::vector<std::string> syncedDevices;
	// Check if any devices have become dirty or are no longer dirty
	for (const auto& dev : mDirtyDevices) {
		if (std::find(state.dirtyDevices.begin(), state.dirtyDevices.end(), dev) == state.dirtyDevices.end()) {
			LOG(LogError) << "Syncthing: Device " << dev << " is no longer dirty.";
			syncedDevices.push_back(dev);
		}
	}
	mDirtyDevices = state.dirtyDevices;
	std::vector<std::string> cleanDevices;
	for (const auto& dev : state.connectedDevices) {
		if (std::find(state.dirtyDevices.begin(), state.dirtyDevices.end(), dev) == state.dirtyDevices.end()) {
			cleanDevices.push_back(dev);
		}
	}
	if (state.isSyncing()) {
		if (wndNotification == nullptr && mWindow != nullptr) {
			LOG(LogError) << "Syncthing: opened notification window at state itemsSynced=" << state.itemsSynced << " itemsTotal=" << state.itemsTotal << " transferSpeed=" << state.transferSpeed;
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
		} else {
			wndNotification->updateText(_("Preparing synchronization..."));
			wndNotification->updatePercent(0);
		}
	} else {
		if (wndNotification != nullptr) {
			// If we were just syncing, show the finished message
			if (mkillNotificationInNextCycle) {
				wndNotification->close();
				wndNotification = nullptr;
				mkillNotificationInNextCycle = false;
			} else if (mCurrentTransferNeededFiles == 0) {
				if (syncedDevices.size() == 0) {
					wndNotification->updateText(_("Synchronization finished."));
				} else {
					wndNotification->updateText(_("Synced with") + " " + toSyncedDevicesNameString(syncedDevices) + ".");
				}
				wndNotification->updatePercent(100);
				mCurrentTransferNeededFiles = 0;
				mkillNotificationInNextCycle = true;
			}
		} else if (mWindow != nullptr) {
			int bytesTransferred = state.totalBytesTransferred - lastTotalBytesTransferred;
			// If we didn't catch the syncing but at least one device has finished, show finished message
			if (syncedDevices.size() > 0) {
				createSyncedNotification(syncedDevices);
			// If we didn't catch the syncing but more than 1KB has been transferred, show finished message
			} else if (bytesTransferred >= 1024) {
				createSyncedNotification(cleanDevices);
			}
		}
	}
	
	mTotalBytesTransferred = state.totalBytesTransferred;
	if (lastTotalBytesTransferred != mTotalBytesTransferred) {
		LOG(LogError) << "Syncthing: Total bytes transferred updated to " << mTotalBytesTransferred;
	}
	return true;
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

void SyncthingWatcher::createSyncedNotification(const std::vector<std::string>& deviceNames) {
	if (mWindow == nullptr || wndNotification != nullptr) {
		return;
	}
	if (deviceNames.size() > 0) {
		createNotification(_("Synced with") + " " + toSyncedDevicesNameString(deviceNames) + ".", 100);
	} else {
		createNotification(_("Finished synchronization."), 100);
	}
	mkillNotificationInNextCycle = true;
}

void SyncthingWatcher::createNotification(const std::string& message, int percent) {
	if (mWindow == nullptr || wndNotification != nullptr) {
		return;
	}
	wndNotification = mWindow->createAsyncNotificationComponent();
	wndNotification->updateTitle(GUIICON + _("SYNCTHING"));
	wndNotification->updateText(message);
	wndNotification->updatePercent(percent);
}