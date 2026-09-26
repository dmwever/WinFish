#ifndef __ARCHIPELAGODIALOG_H__
#define __ARCHIPELAGODIALOG_H__

#include "MoneyDialog.h"
#include "SexyAppFramework/EditListener.h"

namespace Sexy
{
	class WinFishApp;
	class EditWidget;
	class UserProfile;

	// Creates a profile from a new Archipelago connection, or edits an existing profile's connection.
	class ArchipelagoDialog : public MoneyDialog, public ButtonListener, public EditListener
	{
	public:
		enum
		{
			EDIT_SERVER,
			EDIT_SLOT,
			EDIT_PASSWORD,
			NUM_EDITS
		};

		SexyString				mUserName;		// profile being edited; empty for a new connection
		EditWidget*				mEditWidgets[NUM_EDITS];

	public:
		// theProfile == NULL opens a new connection with theDefaultServer prefilled.
		ArchipelagoDialog(WinFishApp* theApp, UserProfile* theProfile, const std::string& theDefaultServer);
		virtual ~ArchipelagoDialog();

		virtual void			AddedToManager(WidgetManager* theWidgetManager);
		virtual void			RemovedFromManager(WidgetManager* theWidgetManager);
		virtual void			Draw(Graphics* g);
		virtual void			Resize(int theX, int theY, int theWidth, int theHeight);
		virtual int				GetPreferredHeight(int theWidth);

		virtual void			ButtonPress(int theId);

		virtual void			EditWidgetText(int theId, const SexyString& theString);

		bool					IsNewConnection() const { return mUserName.empty(); }
		std::string				GetServer() const;
		std::string				GetSlot() const;

		// Copies the (trimmed) fields into theProfile.
		void					ApplyTo(UserProfile* theProfile);
	};
}

#endif
