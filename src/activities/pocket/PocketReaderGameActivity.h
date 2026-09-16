#pragma once

#include "MappedInputManager.h"
#include "activities/Activity.h"

class PocketReaderGameActivity final : public Activity {
 public:
  PocketReaderGameActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);

  void onEnter() override;
  void onExit() override;
  void loop() override;

 private:
  enum class ViewMode {
    Event,
    Locations,
    TeamRoster,
    CompanionDetail,
    LeaveConfirm,
    ReplayConfirm
  };

  bool gameLoaded = false;
  ViewMode viewMode = ViewMode::Event;

  int selectedLocationPosition = 0;
  int pendingTargetLocation = -1;

  ViewMode rosterReturnMode = ViewMode::Event;
  int rosterSelection = 0;
  int reservePage = 0;
  int swapSourceSelection = -1;
  int detailRosterIndex = -1;

  void initializeGameIfNeeded();
  void ensureReadyEvent();

  void addDevelopmentPages();
  void addDevelopmentPagesFromLocations();

  void showCurrentEvent();
  bool drawEventBitmap(const char* path);
  void drawMissingAssetMessage(const char* path);

  void takeDirection(int direction);

  void showLocations();
  void moveLocationSelection(int delta);
  void chooseSelectedLocation();

  void showTeamRoster(ViewMode returnMode);
  void moveRosterSelection(int delta);
  void changeReservePage(int delta);
  void beginOrCompleteSwap();
  void cancelSwap();
  void openSelectedCompanionDetail();
  int selectedRosterIndex() const;
  void returnFromRoster();

  void showLeaveConfirmation(int targetLocation);
  void confirmLeaveAndSwitch();

  void showReplayConfirmation(int targetLocation);
  void confirmReplay();

  void activateLocation(int location, bool deliberateReplay);
  void setReadyEventForCurrentLocation();
  void setNewAdventureEventForCurrentLocation();
  void setNoStepsEvent();

  void drawLocationsScreen();
  void drawTeamRosterScreen();
  void drawCompanionDetailScreen();
  void drawLeaveConfirmScreen();
  void drawReplayConfirmScreen();
};
