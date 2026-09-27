// apclient.hpp must come before anything that includes windows.h (see APBridge.h).
#include <apclient.hpp>
#include <apuuid.hpp>

#include "APBridge.h"

using namespace Sexy;

// Must match `game` in Archipelago/worlds/insaniquarium.
static const char* const AP_GAME_NAME = "Insaniquarium Deluxe";

// Remote items from other worlds, our own world, and starting inventory.
static const int AP_ITEMS_HANDLING = 0b111;

static std::string MakeServerUri(const std::string& theServer)
{
	if (theServer.find("://") != std::string::npos)
		return theServer;

	std::string aHost = theServer.substr(0, theServer.find(':'));
	for (char& c : aHost)
		c = (char)tolower((unsigned char)c);
	if (aHost == "localhost" || aHost == "127.0.0.1")
		return "ws://" + theServer;
	return theServer;
}

APBridge::APBridge(const std::string& theDataFolder, const std::string& theCertFile)
{
	mClient = NULL;
	mState = AP_DISCONNECTED;
	mSocketErrors = 0;
	mSocketErrorsToReport = 1;
	mJustConnected = false;
	mDataFolder = theDataFolder;
	mCertFile = theCertFile;
}

APBridge::~APBridge()
{
	Disconnect();
}

void APBridge::Connect(const std::string& theServer, const std::string& theSlot, const std::string& thePassword)
{
	Disconnect();

	mServer = theServer;
	mSlot = theSlot;
	mPassword = thePassword;
	mLastError.clear();
	mSocketErrors = 0;
	mJustConnected = false;
	mReceivedItems.clear();
	mConfirmedChecks.clear();

	std::string aUuid = ap_get_uuid(mDataFolder + "ap_uuid.txt", theServer);
	std::string aUri = MakeServerUri(theServer);
	mSocketErrorsToReport = aUri.find("://") != std::string::npos ? 1 : 2;
	mClient = new APClient(aUuid, AP_GAME_NAME, aUri, mCertFile);
	mClient->set_receive_own_locations(true);
	mState = AP_SOCKET_CONNECTING;

	mClient->set_socket_error_handler([this](const std::string& theError)
	{
		mLastError = theError;
		mSocketErrors++;
	});

	mClient->set_socket_disconnected_handler([this]()
	{
		// apclientpp reconnects on its own; we just have to log in again after the next RoomInfo.
		mState = AP_SOCKET_CONNECTING;
	});

	mClient->set_room_info_handler([this]()
	{
		mState = AP_SLOT_CONNECTING;
		mSocketErrors = 0;
		mClient->ConnectSlot(mSlot, mPassword, AP_ITEMS_HANDLING);
	});

	mClient->set_slot_connected_handler([this](const nlohmann::json&)
	{
		mState = AP_SLOT_CONNECTED;
		mLastError.clear();
		mJustConnected = true;
	});

	mClient->set_items_received_handler([this](const std::list<APClient::NetworkItem>& theItems)
	{
		for (const APClient::NetworkItem& anItem : theItems)
			mReceivedItems.push_back({ anItem.item, anItem.location, anItem.player, anItem.index });
	});

	// Fires with the server's checked locations right after login, and again whenever it confirms new checks.
	mClient->set_location_checked_handler([this](const std::list<int64_t>& theLocations)
	{
		for (int64_t aLocation : theLocations)
			mConfirmedChecks.push_back((int)aLocation);
	});

	mClient->set_slot_refused_handler([this](const std::list<std::string>& theErrors)
	{
		mState = AP_SLOT_REFUSED;
		mLastError.clear();
		for (const std::string& anError : theErrors)
		{
			if (!mLastError.empty())
				mLastError += ", ";
			mLastError += anError;
		}
	});
}

void APBridge::Disconnect()
{
	delete mClient;
	mClient = NULL;
	mState = AP_DISCONNECTED;
}

void APBridge::StopForWrongSeed()
{
	Disconnect();
	mState = AP_WRONG_SEED;
}

void APBridge::Update()
{
	if (mClient != NULL)
		mClient->poll();
}

bool APBridge::ConsumeJustConnected()
{
	bool aJustConnected = mJustConnected;
	mJustConnected = false;
	return aJustConnected;
}

void APBridge::PopReceivedItems(std::vector<APReceivedItem>& theItems)
{
	theItems.swap(mReceivedItems);
	mReceivedItems.clear();
}

void APBridge::PopConfirmedChecks(std::vector<int>& theLocations)
{
	theLocations.swap(mConfirmedChecks);
	mConfirmedChecks.clear();
}

bool APBridge::IsLocationChecked(int theLocationId) const
{
	if (mClient == NULL)
		return false;
	std::set<int64_t> aChecked = mClient->get_checked_locations();
	return aChecked.find(theLocationId) != aChecked.end();
}

std::string APBridge::GetSeed() const
{
	return mClient != NULL ? mClient->get_seed() : "";
}

void APBridge::SendChecks(const std::set<int>& theLocations)
{
	if (mClient == NULL || theLocations.empty())
		return;
	std::list<int64_t> aLocations(theLocations.begin(), theLocations.end());
	mClient->LocationChecks(aLocations);
}

void APBridge::SendGoal()
{
	if (mClient != NULL)
		mClient->StatusUpdate(APClient::ClientStatus::GOAL);
}

void APBridge::RequestSync()
{
	if (mClient != NULL)
		mClient->Sync();
}

std::string APBridge::GetStatusText() const
{
	switch (mState)
	{
	case AP_SOCKET_CONNECTING:
		if (mSocketErrors >= mSocketErrorsToReport)
			return "Archipelago: can't reach " + mServer + ", retrying...";
		return "Archipelago: connecting to " + mServer + "...";
	case AP_SLOT_CONNECTING:
		return "Archipelago: logging in as " + mSlot + "...";
	case AP_SLOT_CONNECTED:
		return "Archipelago: connected as " + mSlot;
	case AP_SLOT_REFUSED:
		return "Archipelago: refused (" + mLastError + ")";
	case AP_WRONG_SEED:
		return "Archipelago: this server is running a different multiworld";
	default:
		return "";
	}
}
