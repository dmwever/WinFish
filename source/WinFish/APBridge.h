#ifndef __APBRIDGE_H__
#define __APBRIDGE_H__

#include <cstdint>
#include <set>
#include <string>
#include <vector>

// Thin wrapper around apclientpp. Only APBridge.cpp includes apclient.hpp, so asio/websocketpp
// never reach the SexyAppFramework headers (Common.h pins _WIN32_WINNT to 0x0500, asio needs 0x0600+).

class APClient;

namespace Sexy
{
	// One entry of a ReceivedItems packet. mIndex is the item's position in the slot's full received list.
	struct APReceivedItem
	{
		int64_t		mItem;
		int64_t		mLocation;
		int			mPlayer;
		int			mIndex;
	};

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
			AP_WRONG_SEED,		// stopped: the server runs a different multiworld than the profile
		};

		// theDataFolder holds ap_uuid.txt; theCertFile is the CA bundle used for wss://.
		APBridge(const std::string& theDataFolder, const std::string& theCertFile);
		~APBridge();

		// theServer is "host:port" or a full ws:// / wss:// uri; no scheme tries wss first, then ws.
		void Connect(const std::string& theServer, const std::string& theSlot, const std::string& thePassword);
		void Disconnect();
		// Disconnects and stays in AP_WRONG_SEED until the next Connect().
		void StopForWrongSeed();

		// Call once per frame. All apclientpp callbacks fire from inside this call.
		void Update();

		State GetState() const { return mState; }
		const std::string& GetLastError() const { return mLastError; }

		// One-line description for the UI; empty while disconnected.
		std::string GetStatusText() const;

		// The bridge only queues what the server sends; the game drains the queues each frame.
		bool ConsumeJustConnected();									// true once per successful slot login
		void PopReceivedItems(std::vector<APReceivedItem>& theItems);
		void PopConfirmedChecks(std::vector<int>& theLocations);	// locations the server reports as checked
		bool IsLocationChecked(int theLocationId) const;
		std::string GetSeed() const;									// empty until room info arrives

		// Sent now if logged in; otherwise apclientpp queues checks until the next login.
		void SendChecks(const std::set<int>& theLocations);
		void SendGoal();
		// Asks the server to resend all received items (used when an item index is skipped).
		void RequestSync();

	private:
		APClient* mClient;
		State mState;
		int mSocketErrors;			// consecutive failed connection attempts
		int mSocketErrorsToReport;	// failures before the status line says "can't reach"
		std::string mDataFolder;
		std::string mCertFile;
		std::string mServer;
		std::string mSlot;
		std::string mPassword;
		std::string mLastError;
		bool mJustConnected;
		std::vector<APReceivedItem> mReceivedItems;
		std::vector<int> mConfirmedChecks;
	};
};

#endif
