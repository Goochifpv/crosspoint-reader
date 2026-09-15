#pragma once

#include <string>

#include "MappedInputManager.h"
#include "activities/Activity.h"

class PocketReaderGameActivity final : public Activity {
 public:
  PocketReaderGameActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);

  void onEnter() override;
  void onExit() override;
  void loop() override;

 private:
  bool hasEvent = false;
  std::string statusMessage;

  void drawGame();
  void grantDevPages();
  void performNextStep();

  const char* backgroundForLocation(int location) const;
  bool overlayAtTopForLocation(int location) const;

  bool drawSceneBitmap(const char* path);
  void drawOverlay();
  void drawMissingAssetMessage(const char* path);

  std::string eventTitle() const;
  std::string eventBody() const;
  std::string eventResultBlock() const;
};
