#include "PocketPrototypeActivity.h"

#include <GfxRenderer.h>
#include <HalDisplay.h>

#include "fontIds.h"

namespace {

constexpr const char* MENU_ITEMS[] = {
    "Library",
    "Stats",
    "Settings",
    "File Transfer",
    "Companion RPG",
};

}  // namespace

void PocketPrototypeActivity::onEnter() {
  Activity::onEnter();
  requestUpdate();
}

void PocketPrototypeActivity::moveSelection(const int delta) {
  selectedMenu = (selectedMenu + delta + MENU_COUNT) % MENU_COUNT;
  requestUpdate();
}

void PocketPrototypeActivity::activateSelection() {
  statusText = MENU_ITEMS[selectedMenu];
  requestUpdate();
}

void PocketPrototypeActivity::loop() {
  if (mappedInput.wasReleased(MappedInputManager::Button::NavNext)) {
    moveSelection(1);
    return;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::NavPrevious)) {
    moveSelection(-1);
    return;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    activateSelection();
    return;
  }
}

void PocketPrototypeActivity::drawCenteredLabel(const int centerX, const int y, const char* text,
                                                const bool black) const {
  const int width = renderer.getTextWidth(UI_10_FONT_ID, text);
  renderer.drawText(UI_10_FONT_ID, centerX - width / 2, y, text, black);
}

void PocketPrototypeActivity::render(RenderLock&&) {
  const int screenW = renderer.getScreenWidth();
  const int screenH = renderer.getScreenHeight();

  int insetTop = 0;
  int insetRight = 0;
  int insetBottom = 0;
  int insetLeft = 0;
  renderer.getOrientedViewableTRBL(&insetTop, &insetRight, &insetBottom, &insetLeft);

  const int left = insetLeft + 18;
  const int right = screenW - insetRight - 18;
  const int contentW = right - left;

  renderer.clearScreen();

  // Header
  renderer.drawText(UI_12_FONT_ID, left, insetTop + 12, "Currently Reading", true, EpdFontFamily::BOLD);

  const char* battery = "87%";
  const int batteryW = renderer.getTextWidth(UI_10_FONT_ID, battery);
  renderer.drawText(UI_10_FONT_ID, right - batteryW, insetTop + 14, battery);

  const int batteryIconW = 23;
  const int batteryIconH = 11;
  const int batteryX = right - batteryW - batteryIconW - 8;
  const int batteryY = insetTop + 17;
  renderer.drawRect(batteryX, batteryY, batteryIconW, batteryIconH);
  renderer.fillRect(batteryX + batteryIconW, batteryY + 3, 2, 5);
  renderer.fillRect(batteryX + 2, batteryY + 2, 15, batteryIconH - 4);

  renderer.drawLine(left, insetTop + 42, right, insetTop + 42);

  // Current book mock data
  const int coverX = left;
  const int coverY = insetTop + 58;
  const int coverW = 106;
  const int coverH = 150;

  renderer.drawRoundedRect(coverX, coverY, coverW, coverH, 2, 6, true);

  const int demoW = renderer.getTextWidth(UI_12_FONT_ID, "DEMO", EpdFontFamily::BOLD);
  renderer.drawText(UI_12_FONT_ID, coverX + (coverW - demoW) / 2, coverY + 48, "DEMO", true,
                    EpdFontFamily::BOLD);
  const int bookW = renderer.getTextWidth(UI_10_FONT_ID, "BOOK", EpdFontFamily::BOLD);
  renderer.drawText(UI_10_FONT_ID, coverX + (coverW - bookW) / 2, coverY + 76, "BOOK", true,
                    EpdFontFamily::BOLD);

  const int detailsX = coverX + coverW + 18;
  const int detailsW = right - detailsX;

  renderer.drawText(UI_12_FONT_ID, detailsX, coverY + 5, "Greater Than Zero", true, EpdFontFamily::BOLD);
  renderer.drawText(UI_10_FONT_ID, detailsX, coverY + 34, "Adam Gooch");
  renderer.drawText(UI_10_FONT_ID, detailsX, coverY + 71, "42% - 3h 18m remaining");

  const int progressY = coverY + 101;
  renderer.drawRect(detailsX, progressY, detailsW, 12);
  renderer.fillRect(detailsX + 2, progressY + 2, (detailsW - 4) * 42 / 100, 8);

  renderer.drawText(UI_10_FONT_ID, detailsX, coverY + 127, statusText);

  // Recent books
  const int recentTitleY = coverY + coverH + 18;
  renderer.drawText(UI_10_FONT_ID, left, recentTitleY, "Recently Read", true, EpdFontFamily::BOLD);

  const int recentY = recentTitleY + 27;
  constexpr int recentCount = 4;
  const int gap = 12;
  const int recentW = (contentW - gap * (recentCount - 1)) / recentCount;
  const int recentH = 92;

  for (int i = 0; i < recentCount; ++i) {
    const int x = left + i * (recentW + gap);
    renderer.drawRoundedRect(x, recentY, recentW, recentH, 1, 4, true);

    char number[2] = {static_cast<char>('1' + i), '\0'};
    const int numberW = renderer.getTextWidth(UI_12_FONT_ID, number, EpdFontFamily::BOLD);
    renderer.drawText(UI_12_FONT_ID, x + (recentW - numberW) / 2, recentY + 32, number, true,
                      EpdFontFamily::BOLD);
  }

  // Main menu
  const int menuTop = recentY + recentH + 18;
  const int bottomLegendTop = screenH - insetBottom - 47;
  const int availableMenuH = bottomLegendTop - menuTop - 8;
  const int rowH = availableMenuH / MENU_COUNT;

  for (int i = 0; i < MENU_COUNT; ++i) {
    const int y = menuTop + i * rowH;
    const bool selected = i == selectedMenu;

    if (selected) {
      renderer.fillRoundedRect(left, y, contentW, rowH - 5, 5, Color::Black);
      renderer.drawText(UI_12_FONT_ID, left + 14, y + 10, MENU_ITEMS[i], false, EpdFontFamily::BOLD);
    } else {
      renderer.drawRoundedRect(left, y, contentW, rowH - 5, 1, 5, true);
      renderer.drawText(UI_12_FONT_ID, left + 14, y + 10, MENU_ITEMS[i], true);
    }
  }

  // Contextual front-button legend
  renderer.drawLine(left, bottomLegendTop, right, bottomLegendTop);

  const auto labels = mappedInput.mapLabels("Back", "Select", "Up", "Down");
  const int quarter = contentW / 4;
  const int legendY = bottomLegendTop + 15;

  drawCenteredLabel(left + quarter / 2, legendY, labels.btn1, true);
  drawCenteredLabel(left + quarter + quarter / 2, legendY, labels.btn2, true);
  drawCenteredLabel(left + quarter * 2 + quarter / 2, legendY, labels.btn3, true);
  drawCenteredLabel(left + quarter * 3 + quarter / 2, legendY, labels.btn4, true);

  renderer.displayBuffer(firstPaint ? HalDisplay::HALF_REFRESH : HalDisplay::FAST_REFRESH);
  firstPaint = false;
}
