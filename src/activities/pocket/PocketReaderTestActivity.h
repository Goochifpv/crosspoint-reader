#pragma once

#include <string>

#include "MappedInputManager.h"
#include "activities/Activity.h"

class PocketReaderTestActivity final : public Activity {
 public:
  PocketReaderTestActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);

  void onEnter() override;
  void onExit() override;
  void loop() override;

 private:
  int sceneIndex = 0;
  bool scrollVisible = false;
  unsigned long sceneShownAt = 0;

  void showScene();
  void revealScroll();
  bool drawSceneBitmap(const char* path, int artOffsetX, int artOffsetY, bool withScroll);
  void drawScrollOverlay();
  void drawMissingAssetMessage(const char* path);
};
