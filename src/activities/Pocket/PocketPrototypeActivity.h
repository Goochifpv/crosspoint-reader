#pragma once

#include "activities/Activity.h"

class PocketPrototypeActivity final : public Activity {
 public:
  explicit PocketPrototypeActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("PocketPrototype", renderer, mappedInput) {}

  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;
  bool isHomeActivity() const override { return true; }

 private:
  static constexpr int MENU_COUNT = 5;

  int selectedMenu = 0;
  bool firstPaint = true;
  const char* statusText = "G00-CH1 prototype";

  void moveSelection(int delta);
  void activateSelection();
  void drawCenteredLabel(int centerX, int y, const char* text, bool black) const;
};
