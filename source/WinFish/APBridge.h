#ifndef __APBRIDGE_H__
#define __APBRIDGE_H__

#include <string>

// Thin wrapper around apclientpp. Only APBridge.cpp includes apclient.hpp, so asio/websocketpp
// never reach the SexyAppFramework headers (Common.h pins _WIN32_WINNT to 0x0500, asio needs 0x0600+).

class APClient;

namespace Sexy
{
	class APBridge
	{
	public:
		enum State
		{
			AP_DISCONNECTED,
			AP_SOCKET_CONNECTING,
			AP_SLOT_CONNECTING,
			AP_SLOT_CONNECTED,
			AP_SLOT_REFUSED,
		};

		// theDataFolder holds ap_uuid.txt; theCertFile is the CA bundle used for wss://.
		APBridge(const std::string& theDataFolder, const std::string& theCertFile);
		~APBridge();

		// theServer is "host:port" or a full ws:// / wss:// uri; no scheme tries wss first, then ws.
		void Connect(const std::string& theServer, const std::string& theSlot, const std::string& thePassword);
		void Disconnect();

		// Call once per frame. All apclientpp callbacks fire from inside this call.
		void Update();

		State GetState() const { return mState; }
		const std::string& GetLastError() const { return mLastError; }

		// One-line description for the UI; empty while disconnected.
		std::string GetStatusText() const;

	private:
		APClient* mClient;
		State mState;
		int mSocketErrors;			// consecutive failed connection attempts
		int mSocketErrorsToReport;	// failures before the status line says "can't reach"
		bool mHasMockItem;
		bool mGoalSent;
		std::string mDataFolder;
		std::string mCertFile;
		std::string mServer;
		std::string mSlot;
		std::string mPassword;
		std::string mLastError;
	};
};

#endif
