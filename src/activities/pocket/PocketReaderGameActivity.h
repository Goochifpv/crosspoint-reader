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
    LeaveConfirm,
    ReplayConfirm
  };

  bool gameLoaded = false;
  ViewMode viewMode = ViewMode::Event;

  int selectedLocationPosition = 0;
  int pendingTargetLocation = -1;

  void initializeGameIfNeeded();
  void ensureReadyEvent();

  void addDevelopmentPages();

  void showCurrentEvent();
  bool drawEventBitmap(const char* path);
  void drawMissingAssetMessage(const char* path);

  void takeDirection(int direction);

  void showLocations();
  void moveLocationSelection(int delta);
  void chooseSelectedLocation();

  void showLeaveConfirmation(int targetLocation);
  void confirmLeaveAndSwitch();

  void showReplayConfirmation(int targetLocation);
  void confirmReplay();

  void activateLocation(int location, bool deliberateReplay);
  void setReadyEventForCurrentLocation();
  void setNewAdventureEventForCurrentLocation();
  void setNoStepsEvent();

  void drawLocationsScreen();
  void drawLeaveConfirmScreen();
  void drawReplayConfirmScreen();
};
