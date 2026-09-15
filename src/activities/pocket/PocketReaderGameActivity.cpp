#include "PocketReaderGameActivity.h"

#include <algorithm>
#include <string>

#include <Bitmap.h>
#include <GfxRenderer.h>
#include <HalDisplay.h>
#include <HalStorage.h>

#include "activities/ActivityManager.h"
#include "fontIds.h"

#include "pocket_game/AdventureEngine.h"
#include "pocket_game/GameState.h"
#include "pocket_game/SaveSystem.h"

namespace {

constexpr int SCREEN_W = 480;
constexpr int SCREEN_H = 800;

// ==================================================
// EVENT VIEW
// ==================================================

constexpr int FOOTER_X = 12;
constexpr int FOOTER_Y = 756;
constexpr int FOOTER_W = 456;
constexpr int FOOTER_H = 36;

constexpr int CARD_X = 12;
constexpr int CARD_W = 456;
constexpr int CARD_SIDE_PAD = 22;
constexpr int CARD_MIN_H = 165;
constexpr int CARD_MAX_H = 260;
constexpr int CARD_GAP_TO_FOOTER = 16;

constexpr int AREA_BOX_W = 300;
constexpr int AREA_BOX_H = 40;

constexpr int STATS_BOX_TOP_PAD = 7;
constexpr int STATS_BOX_BOTTOM_PAD = 7;

// X4 physical side rocker labels.
// The separate upper side button remains the normal POWER button.
constexpr int SIDE_LABEL_X = 442;
constexpr int SIDE_LABEL_W = 30;
constexpr int SIDE_LABEL_H = 82;
constexpr int NORTH_LABEL_Y = 340;
constexpr int SOUTH_LABEL_Y = 440;

// ==================================================
// MONO UI
// ==================================================

constexpr int UI_MARGIN = 14;
constexpr int UI_CONTENT_W = SCREEN_W - (UI_MARGIN * 2);

bool overlayAtTopForLocation(const int location) {
  // Proven on hardware: Misty Marshes benefits from keeping its
  // lower foreground clear.
  return location == LOC_MISTY_MARSH;
}

const char* locationArtPath(const int location) {
  switch (location) {
    case LOC_MISTY_MARSH:
      return "/PocketReader/art/locations/misty_marshes.bmp";

    case LOC_WHISPERWOOD:
    default:
      // Phase 2 still only ships the two hardware-proven masters.
      // The remaining six location masters can drop into this mapping later.
      return "/PocketReader/art/locations/whispering_woods.bmp";
  }
}

std::string eventNarrative() {
  std::string text;

  if (currentEvent.line1.length() > 0) {
    text += currentEvent.line1.c_str();
  }

  if (currentEvent.line2.length() > 0) {
    if (!text.empty()) text += " ";
    text += currentEvent.line2.c_str();
  }

  // Contextual companion/result prose belongs with the event description.
  if (currentEvent.result.length() > 0) {
    if (!text.empty()) text += " ";
    text += currentEvent.result.c_str();
  }

  return text;
}

std::string oneLineStats() {
  // Canonical visual order: Strength -> Heart -> Luck.
  std::string stats =
      "S" + std::string(currentEvent.xpDelta[ROLE_STRENGTH] >= 0 ? "+" : "") +
      std::to_string(currentEvent.xpDelta[ROLE_STRENGTH]) +
      "  H" + std::string(currentEvent.xpDelta[ROLE_HEART] >= 0 ? "+" : "") +
      std::to_string(currentEvent.xpDelta[ROLE_HEART]) +
      "  L" + std::string(currentEvent.xpDelta[ROLE_LUCK] >= 0 ? "+" : "") +
      std::to_string(currentEvent.xpDelta[ROLE_LUCK]);

  stats += "  |  ";
  stats += std::to_string(locationProgress[currentLocation]);
  stats += "/100";

  stats += "  |  Steps ";
  stats += std::to_string(bankedSteps);

  return stats;
}

void drawAreaLabel(GfxRenderer& renderer, const bool cardAtTop) {
  const int x = (SCREEN_W - AREA_BOX_W) / 2;
  const int y = cardAtTop
      ? (FOOTER_Y - AREA_BOX_H - 12)
      : 18;

  renderer.fillRoundedRect(x, y, AREA_BOX_W, AREA_BOX_H, 9, Color::White);
  renderer.drawRoundedRect(x, y, AREA_BOX_W, AREA_BOX_H, 1, 9, true);

  renderer.drawCenteredText(
      NOTOSERIF_14_FONT_ID,
      y + 10,
      locationName(currentLocation).c_str(),
      true,
      EpdFontFamily::BOLD);
}

void drawBottomFrontButton(
    GfxRenderer& renderer,
    const int index,
    const char* label,
    const bool emphasized = false) {
  constexpr int SEGMENTS = 4;
  constexpr int SEG_W = FOOTER_W / SEGMENTS;

  const int x = FOOTER_X + (index * SEG_W);

  renderer.fillRoundedRect(
      x,
      FOOTER_Y,
      SEG_W,
      FOOTER_H,
      7,
      emphasized ? Color::LightGray : Color::White);

  renderer.drawRoundedRect(
      x,
      FOOTER_Y,
      SEG_W,
      FOOTER_H,
      1,
      7,
      true);

  if (label == nullptr || label[0] == '\0') {
    return;
  }

  const auto style =
      emphasized ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR;

  const int textWidth =
      renderer.getTextWidth(SMALL_FONT_ID, label, style);

  renderer.drawText(
      SMALL_FONT_ID,
      x + ((SEG_W - textWidth) / 2),
      FOOTER_Y + 10,
      label,
      true,
      style);
}

void drawVerticalReadUpText(
    GfxRenderer& renderer,
    const int pillX,
    const int pillY,
    const int pillW,
    const int pillH,
    const char* text,
    const bool bold) {
  const auto style =
      bold ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR;

  const int textLength =
      renderer.getTextWidth(SMALL_FONT_ID, text, style);

  const int textHeight =
      renderer.getLineHeight(SMALL_FONT_ID);

  // A normal left-to-right word drawn in LandscapeClockwise appears
  // bottom-to-top when viewed in the device's Portrait orientation.
  const int portraitBottom =
      pillY + ((pillH + textLength) / 2);

  const int landscapeX =
      (SCREEN_H - 1) - portraitBottom;

  const int landscapeY =
      pillX + ((pillW - textHeight) / 2);

  renderer.setOrientation(GfxRenderer::LandscapeClockwise);

  renderer.drawText(
      SMALL_FONT_ID,
      landscapeX,
      landscapeY,
      text,
      true,
      style);

  renderer.setOrientation(GfxRenderer::Portrait);
}

void drawSideControlPill(
    GfxRenderer& renderer,
    const int y,
    const char* label,
    const bool allowed) {
  renderer.fillRoundedRect(
      SIDE_LABEL_X,
      y,
      SIDE_LABEL_W,
      SIDE_LABEL_H,
      8,
      allowed ? Color::White : Color::LightGray);

  renderer.drawRoundedRect(
      SIDE_LABEL_X,
      y,
      SIDE_LABEL_W,
      SIDE_LABEL_H,
      1,
      8,
      true);

  std::string text = label;

  if (!allowed) {
    text += " X";
  }

  drawVerticalReadUpText(
      renderer,
      SIDE_LABEL_X,
      y,
      SIDE_LABEL_W,
      SIDE_LABEL_H,
      text.c_str(),
      !allowed);
}

void drawEventFooter(GfxRenderer& renderer) {
  const bool westAllowed = directionAllowed(DIR_WEST);
  const bool eastAllowed = directionAllowed(DIR_EAST);

  std::string westLabel = "WEST";
  std::string eastLabel = "EAST";

  if (!westAllowed) westLabel += " X";
  if (!eastAllowed) eastLabel += " X";

  drawBottomFrontButton(renderer, 0, "BACK");
  drawBottomFrontButton(renderer, 1, "+100P");
  drawBottomFrontButton(renderer, 2, westLabel.c_str(), !westAllowed);
  drawBottomFrontButton(renderer, 3, eastLabel.c_str(), !eastAllowed);
}

void drawAdventureSideControls(
    GfxRenderer& renderer,
    const bool cardAtTop,
    const int cardY,
    const int cardH) {
  constexpr int STACK_GAP = 18;

  int southY = SOUTH_LABEL_Y;
  int northY = NORTH_LABEL_Y;

  if (!cardAtTop) {
    // Keep the pair together in the lower-right artwork area while still
    // avoiding the parchment card.
    const int latestSouthY =
        cardY - SIDE_LABEL_H - 10;

    if (southY > latestSouthY) {
      southY = latestSouthY;
    }

    if (southY < 400) {
      southY = 400;
    }

    northY = southY - SIDE_LABEL_H - STACK_GAP;

    if (northY < 300) {
      northY = 300;
      southY = northY + SIDE_LABEL_H + STACK_GAP;
    }
  } else {
    // For top-card layouts, keep the pair together below the card.
    northY = cardY + cardH + 18;
    southY = northY + SIDE_LABEL_H + STACK_GAP;

    if (southY > SCREEN_H - SIDE_LABEL_H - 70) {
      southY = SCREEN_H - SIDE_LABEL_H - 70;
      northY = southY - SIDE_LABEL_H - STACK_GAP;
    }
  }

  drawSideControlPill(
      renderer,
      northY,
      "NORTH",
      directionAllowed(DIR_NORTH));

  drawSideControlPill(
      renderer,
      southY,
      "SOUTH",
      directionAllowed(DIR_SOUTH));
}

void drawEventOverlay(GfxRenderer& renderer) {
  const bool cardAtTop = overlayAtTopForLocation(currentLocation);

  const int textX = CARD_X + CARD_SIDE_PAD;
  const int textWidth = CARD_W - (CARD_SIDE_PAD * 2);

  const int titleFont = NOTOSERIF_14_FONT_ID;
  const int bodyFont = NOTOSERIF_12_FONT_ID;
  const int statsFont = UI_10_FONT_ID;

  const std::string narrative = eventNarrative();
  const auto narrativeLines =
      renderer.wrappedText(bodyFont, narrative.c_str(), textWidth, 7);

  const std::string stats = oneLineStats();

  const int titleH = renderer.getLineHeight(titleFont);
  const int bodyLineH = renderer.getLineHeight(bodyFont);
  const int statsLineH = renderer.getLineHeight(statsFont);

  const int statsBoxH =
      STATS_BOX_TOP_PAD +
      statsLineH +
      STATS_BOX_BOTTOM_PAD;

  int cardH = 0;
  cardH += 11;
  cardH += titleH;
  cardH += 4;
  cardH += 8;
  cardH += static_cast<int>(narrativeLines.size()) * bodyLineH;
  cardH += 8;
  cardH += statsBoxH;
  cardH += 13;

  cardH = std::max(CARD_MIN_H, std::min(CARD_MAX_H, cardH));

  const int cardY = cardAtTop
      ? 18
      : (FOOTER_Y - CARD_GAP_TO_FOOTER - cardH);

  renderer.fillRoundedRect(CARD_X, cardY, CARD_W, cardH, 12, Color::White);
  renderer.drawRoundedRect(CARD_X, cardY, CARD_W, cardH, 2, 12, true);

  renderer.fillRoundedRect(CARD_X + 8, cardY - 5, CARD_W - 16, 13, 7, Color::LightGray);
  renderer.drawRoundedRect(CARD_X + 8, cardY - 5, CARD_W - 16, 13, 1, 7, true);

  renderer.fillRoundedRect(CARD_X + 8, cardY + cardH - 8, CARD_W - 16, 13, 7, Color::LightGray);
  renderer.drawRoundedRect(CARD_X + 8, cardY + cardH - 8, CARD_W - 16, 13, 1, 7, true);

  int cursorY = cardY + 11;

  renderer.drawCenteredText(
      titleFont,
      cursorY,
      currentEvent.title.c_str(),
      true,
      EpdFontFamily::BOLD);

  cursorY += titleH + 3;

  renderer.drawLine(
      textX,
      cursorY,
      CARD_X + CARD_W - CARD_SIDE_PAD,
      cursorY,
      true);

  cursorY += 8;

  for (const auto& line : narrativeLines) {
    renderer.drawCenteredText(
        bodyFont,
        cursorY,
        line.c_str(),
        true);

    cursorY += bodyLineH;
  }

  cursorY += 7;

  const int statsBoxX = textX;
  const int statsBoxY = cursorY;
  const int statsBoxW = textWidth;

  renderer.fillRoundedRect(
      statsBoxX,
      statsBoxY,
      statsBoxW,
      statsBoxH,
      7,
      Color::White);

  renderer.drawRoundedRect(
      statsBoxX,
      statsBoxY,
      statsBoxW,
      statsBoxH,
      1,
      7,
      true);

  renderer.drawCenteredText(
      statsFont,
      statsBoxY + STATS_BOX_TOP_PAD,
      stats.c_str(),
      true,
      EpdFontFamily::BOLD);

  // Location title deliberately omitted from the adventure artwork screen.
  // The current location is already clear from the Locations screen and this
  // keeps the event artwork cleaner and avoids title-banner layout conflicts.
  drawAdventureSideControls(renderer, cardAtTop, cardY, cardH);
  drawEventFooter(renderer);
}

bool drawComposedEventFrame(GfxRenderer& renderer, Bitmap& bitmap) {
  const int pageWidth = renderer.getScreenWidth();
  const int pageHeight = renderer.getScreenHeight();

  renderer.clearScreen();

  if (!renderer.drawBitmap(bitmap, 0, 0, pageWidth, pageHeight, 0, 0)) {
    return false;
  }

  drawEventOverlay(renderer);
  return true;
}

void drawMonoHeader(
    GfxRenderer& renderer,
    const char* title,
    const char* subtitle = nullptr) {
  renderer.drawCenteredText(
      NOTOSERIF_18_FONT_ID,
      28,
      title,
      true,
      EpdFontFamily::BOLD);

  renderer.drawLine(UI_MARGIN, 62, SCREEN_W - UI_MARGIN, 62, true);

  if (subtitle != nullptr && subtitle[0] != '\0') {
    renderer.drawCenteredText(
        UI_10_FONT_ID,
        73,
        subtitle,
        true);
  }
}

void drawMonoFooter(
    GfxRenderer& renderer,
    const char* left,
    const char* center,
    const char* right) {
  const int y = 748;

  renderer.drawLine(UI_MARGIN, y - 8, SCREEN_W - UI_MARGIN, y - 8, true);

  renderer.drawText(
      SMALL_FONT_ID,
      UI_MARGIN,
      y,
      left,
      true);

  if (center != nullptr && center[0] != '\0') {
    renderer.drawCenteredText(
        SMALL_FONT_ID,
        y,
        center,
        true);
  }

  if (right != nullptr && right[0] != '\0') {
    renderer.drawText(
        SMALL_FONT_ID,
        SCREEN_W - UI_MARGIN - renderer.getTextWidth(SMALL_FONT_ID, right),
        y,
        right,
        true);
  }
}

std::string locationRowStatus(const int location) {
  if (!locationUnlocked[location]) {
    return "LOCKED";
  }

  if (locationCompleted[location]) {
    return "COMPLETE";
  }

  if (expeditionActive && location == currentLocation) {
    return "ACTIVE  " + std::to_string(locationProgress[location]) + "/100";
  }

  if (locationProgress[location] > 0) {
    return std::to_string(locationProgress[location]) + "/100";
  }

  return "READY";
}

void drawLocationRow(
    GfxRenderer& renderer,
    const int position,
    const int location,
    const int y,
    const bool selected) {
  constexpr int ROW_X = 20;
  constexpr int ROW_W = 440;
  constexpr int ROW_H = 65;

  if (selected) {
    renderer.fillRoundedRect(ROW_X, y, ROW_W, ROW_H, 9, Color::LightGray);
  } else {
    renderer.fillRoundedRect(ROW_X, y, ROW_W, ROW_H, 9, Color::White);
  }

  renderer.drawRoundedRect(ROW_X, y, ROW_W, ROW_H, 1, 9, true);

  std::string prefix = std::to_string(position + 1);
  prefix += ". ";

  const std::string name = prefix + locationName(location).c_str();

  renderer.drawText(
      NOTOSERIF_12_FONT_ID,
      ROW_X + 14,
      y + 10,
      name.c_str(),
      true,
      EpdFontFamily::BOLD);

  const std::string status = locationRowStatus(location);

  renderer.drawText(
      UI_10_FONT_ID,
      ROW_X + 14,
      y + 38,
      status.c_str(),
      true);

  if (selected) {
    const char* marker = ">";
    renderer.drawText(
        NOTOSERIF_14_FONT_ID,
        ROW_X + ROW_W - 28,
        y + 21,
        marker,
        true,
        EpdFontFamily::BOLD);
  }
}

void drawDirectionBox(
    GfxRenderer& renderer,
    const int x,
    const int y,
    const int w,
    const int h,
    const char* label,
    const int direction) {
  const bool allowed = directionAllowed(direction);

  renderer.fillRoundedRect(
      x,
      y,
      w,
      h,
      10,
      allowed ? Color::White : Color::LightGray);

  renderer.drawRoundedRect(
      x,
      y,
      w,
      h,
      1,
      10,
      true);

  renderer.drawCenteredText(
      NOTOSERIF_16_FONT_ID,
      y + 19,
      label,
      true,
      EpdFontFamily::BOLD);

  if (!allowed) {
    renderer.drawCenteredText(
        SMALL_FONT_ID,
        y + 49,
        "BLOCKED",
        true);
  }
}

}  // namespace

PocketReaderGameActivity::PocketReaderGameActivity(
    GfxRenderer& renderer,
    MappedInputManager& mappedInput)
    : Activity("PocketReaderGame", renderer, mappedInput) {}

void PocketReaderGameActivity::onEnter() {
  Activity::onEnter();

  renderer.setOrientation(GfxRenderer::Portrait);

  initializeGameIfNeeded();
  ensureReadyEvent();

  selectedLocationPosition =
      std::max(0, progressionPositionForLocation(currentLocation));

  viewMode = ViewMode::Event;
  showCurrentEvent();
}

void PocketReaderGameActivity::onExit() {
  Activity::onExit();
}

void PocketReaderGameActivity::loop() {
  Activity::loop();

  using Button = MappedInputManager::Button;

  // CrossPoint's normal UI activities use wasPressed() for Back/menu
  // navigation. Phase 2a used wasReleased(), which proved unreliable on
  // the physical X4 for Back and Side-Up in this activity.

  // ==================================================
  // EVENT
  // ==================================================
  if (viewMode == ViewMode::Event) {
    if (mappedInput.wasPressed(Button::Back)) {
      showLocations();
      return;
    }

    // Temporary hidden development shortcut.
    if (mappedInput.wasPressed(Button::Confirm)) {
      addDevelopmentPages();
      return;
    }

    if (mappedInput.wasPressed(Button::Up)) {
      takeDirection(DIR_NORTH);
      return;
    }

    if (mappedInput.wasPressed(Button::Right)) {
      takeDirection(DIR_EAST);
      return;
    }

    if (mappedInput.wasPressed(Button::Down)) {
      takeDirection(DIR_SOUTH);
      return;
    }

    if (mappedInput.wasPressed(Button::Left)) {
      takeDirection(DIR_WEST);
      return;
    }

    return;
  }

  // ==================================================
  // LOCATIONS
  // ==================================================
  if (viewMode == ViewMode::Locations) {
    if (mappedInput.wasPressed(Button::Back)) {
      activityManager.goHome();
      return;
    }

    // The physical side rocker is the only vertical list control.
    if (mappedInput.wasPressed(Button::Up)) {
      moveLocationSelection(-1);
      return;
    }

    if (mappedInput.wasPressed(Button::Down)) {
      moveLocationSelection(+1);
      return;
    }

    if (mappedInput.wasPressed(Button::Confirm)) {
      chooseSelectedLocation();
      return;
    }

    return;
  }

  // ==================================================
  // LEAVE CONFIRM
  // ==================================================
  if (viewMode == ViewMode::LeaveConfirm) {
    if (mappedInput.wasPressed(Button::Back) ||
        mappedInput.wasPressed(Button::Left) ||
        mappedInput.wasPressed(Button::PageBack)) {
      pendingTargetLocation = -1;
      viewMode = ViewMode::Locations;
      drawLocationsScreen();
      return;
    }

    if (mappedInput.wasPressed(Button::Confirm) ||
        mappedInput.wasPressed(Button::Right) ||
        mappedInput.wasPressed(Button::PageForward)) {
      confirmLeaveAndSwitch();
      return;
    }

    return;
  }

  // ==================================================
  // REPLAY CONFIRM
  // ==================================================
  if (viewMode == ViewMode::ReplayConfirm) {
    if (mappedInput.wasPressed(Button::Back) ||
        mappedInput.wasPressed(Button::Left) ||
        mappedInput.wasPressed(Button::PageBack)) {
      pendingTargetLocation = -1;
      viewMode = ViewMode::Locations;
      drawLocationsScreen();
      return;
    }

    if (mappedInput.wasPressed(Button::Confirm) ||
        mappedInput.wasPressed(Button::Right) ||
        mappedInput.wasPressed(Button::PageForward)) {
      confirmReplay();
      return;
    }
  }
}

void PocketReaderGameActivity::initializeGameIfNeeded() {
  if (gameLoaded) return;

  loadGame();

  // Phase 1 could leave expeditionActive true after the 100% boss because
  // the old TFT UI used to close the run after rendering that final event.
  // Repair that state on load without disturbing permanent completion data.
  if (expeditionActive &&
      validLocation(currentLocation) &&
      locationProgress[currentLocation] >= 100) {
    finishExpedition();
  }

  gameLoaded = true;
}

void PocketReaderGameActivity::ensureReadyEvent() {
  if (currentEvent.title.length() > 0) return;

  setReadyEventForCurrentLocation();
}

void PocketReaderGameActivity::setReadyEventForCurrentLocation() {
  clearEventXP();

  currentEvent.type = TRAVEL_EVENT;
  currentEvent.title = expeditionActive ? "ADVENTURE RESUMED" : "ADVENTURE READY";
  currentEvent.line1 = "Choose a direction when you are ready to travel.";
  currentEvent.line2 = "";
  currentEvent.result = "Reading earns the Adventure Steps that carry the party onward.";
  currentEvent.pauseAuto = false;
}

void PocketReaderGameActivity::setNewAdventureEventForCurrentLocation() {
  clearEventXP();

  currentEvent.type = TRAVEL_EVENT;
  currentEvent.title = "A NEW ADVENTURE";
  currentEvent.line1 = "The team sets out into ";
  currentEvent.line1 += locationName(currentLocation);
  currentEvent.line1 += ".";
  currentEvent.line2 = "";
  currentEvent.result = "Choose a direction when you are ready to travel.";
  currentEvent.pauseAuto = false;
}

void PocketReaderGameActivity::setNoStepsEvent() {
  clearEventXP();

  currentEvent.type = REST_EVENT;
  currentEvent.title = "NO ADVENTURE STEPS";
  currentEvent.line1 = "There are no banked Adventure Steps.";
  currentEvent.line2 = "Read more pages before setting out again.";
  currentEvent.result = "";
  currentEvent.pauseAuto = false;
}

void PocketReaderGameActivity::addDevelopmentPages() {
  addPagesRead(100);

  clearEventXP();
  currentEvent.type = TRAVEL_EVENT;
  currentEvent.title = "PAGES BANKED";
  currentEvent.line1 = "One hundred test pages have been added.";
  currentEvent.line2 = "That earned ten Adventure Steps.";
  currentEvent.result = "Reading progress creates adventure.";
  currentEvent.pauseAuto = false;

  viewMode = ViewMode::Event;
  showCurrentEvent();
}


void PocketReaderGameActivity::takeDirection(const int direction) {
  if (!directionAllowed(direction)) {
    return;
  }

  if (bankedSteps <= 0) {
    setNoStepsEvent();
    viewMode = ViewMode::Event;
    showCurrentEvent();
    return;
  }

  if (locationCompleted[currentLocation] &&
      locationProgress[currentLocation] >= 100 &&
      !expeditionActive) {
    showReplayConfirmation(currentLocation);
    return;
  }

  if (!expeditionActive) {
    startExpedition();
  }

  if (!expeditionActive) {
    setNoStepsEvent();
    viewMode = ViewMode::Event;
    showCurrentEvent();
    return;
  }

  performAdventureStep(direction);

  if (locationProgress[currentLocation] >= 100) {
    finishExpedition();
  }

  viewMode = ViewMode::Event;
  showCurrentEvent();
}
void PocketReaderGameActivity::showLocations() {
  selectedLocationPosition =
      std::max(0, progressionPositionForLocation(currentLocation));

  viewMode = ViewMode::Locations;
  drawLocationsScreen();
}

void PocketReaderGameActivity::moveLocationSelection(const int delta) {
  selectedLocationPosition += delta;

  if (selectedLocationPosition < 0) {
    selectedLocationPosition = LOCATION_COUNT - 1;
  }

  if (selectedLocationPosition >= LOCATION_COUNT) {
    selectedLocationPosition = 0;
  }

  drawLocationsScreen();
}

void PocketReaderGameActivity::chooseSelectedLocation() {
  const int target =
      locationForProgressionPosition(selectedLocationPosition);

  if (!validLocation(target)) return;

  if (!locationUnlocked[target]) {
    // Locked rows are visible but not selectable.
    return;
  }

  // Same active run: RESUME. No reset, no confirmation.
  if (expeditionActive &&
      target == currentLocation &&
      locationProgress[currentLocation] < 100) {
    viewMode = ViewMode::Event;
    showCurrentEvent();
    return;
  }

  // Switching away from an unfinished active run is the one destructive
  // navigation action and therefore requires explicit confirmation.
  if (expeditionActive &&
      target != currentLocation &&
      locationProgress[currentLocation] < 100) {
    showLeaveConfirmation(target);
    return;
  }

  // Deliberately selecting a completed location starts a replay, but only
  // after a dedicated confirmation.
  if (locationCompleted[target] &&
      locationProgress[target] >= 100) {
    showReplayConfirmation(target);
    return;
  }

  activateLocation(target, false);
}

void PocketReaderGameActivity::showLeaveConfirmation(const int targetLocation) {
  pendingTargetLocation = targetLocation;
  viewMode = ViewMode::LeaveConfirm;
  drawLeaveConfirmScreen();
}

void PocketReaderGameActivity::confirmLeaveAndSwitch() {
  if (!validLocation(pendingTargetLocation)) {
    pendingTargetLocation = -1;
    showLocations();
    return;
  }

  const int target = pendingTargetLocation;

  // IMPORTANT: currentLocation still points at the active run here.
  // abandonCurrentAdventure() therefore resets exactly the run being left.
  abandonCurrentAdventure();

  pendingTargetLocation = -1;

  // The leave confirmation itself counts as deliberate intent. If the target
  // is completed, going there means replaying it; don't ask for a second popup.
  activateLocation(target, locationCompleted[target]);
}

void PocketReaderGameActivity::showReplayConfirmation(const int targetLocation) {
  pendingTargetLocation = targetLocation;
  viewMode = ViewMode::ReplayConfirm;
  drawReplayConfirmScreen();
}

void PocketReaderGameActivity::confirmReplay() {
  if (!validLocation(pendingTargetLocation)) {
    pendingTargetLocation = -1;
    showLocations();
    return;
  }

  const int target = pendingTargetLocation;
  pendingTargetLocation = -1;

  activateLocation(target, true);
}

void PocketReaderGameActivity::activateLocation(
    const int location,
    const bool deliberateReplay) {
  if (!validLocation(location) || !locationUnlocked[location]) {
    showLocations();
    return;
  }

  currentLocation = location;
  saveGame();

  if (bankedSteps <= 0) {
    setNoStepsEvent();
    viewMode = ViewMode::Event;
    showCurrentEvent();
    return;
  }

  // For a completed location this is intentionally called only after the
  // player confirmed REPLAY (or confirmed leaving another run for it).
  if (locationCompleted[location] &&
      locationProgress[location] >= 100 &&
      !deliberateReplay) {
    showReplayConfirmation(location);
    return;
  }

  startExpedition();

  setNewAdventureEventForCurrentLocation();

  viewMode = ViewMode::Event;
  showCurrentEvent();
}

void PocketReaderGameActivity::showCurrentEvent() {
  RenderLock lock(*this);

  renderer.setRenderMode(GfxRenderer::BW);
  renderer.clearScreen();

  const char* artPath = locationArtPath(currentLocation);

  if (!drawEventBitmap(artPath)) {
    renderer.setRenderMode(GfxRenderer::BW);
    renderer.clearScreen();
    drawMissingAssetMessage(artPath);
    renderer.displayBuffer(HalDisplay::HALF_REFRESH);
  }
}

bool PocketReaderGameActivity::drawEventBitmap(const char* path) {
  HalFile file;

  if (!Storage.openFileForRead("PRG", path, file)) {
    return false;
  }

  const bool absoluteGray =
      renderer.grayscaleCapabilities(HalDisplay::GrayscaleMode::Absolute).supported() &&
      display.getController() == HalDisplay::Controller::SSD1677;

  Bitmap bitmap(file, true, absoluteGray);

  if (bitmap.parseHeaders() != BmpReaderError::Ok) {
    file.close();
    return false;
  }

  if (!bitmap.hasGreyscale()) {
    renderer.setRenderMode(GfxRenderer::BW);

    const bool ok = drawComposedEventFrame(renderer, bitmap);

    if (ok) {
      renderer.displayBuffer(HalDisplay::HALF_REFRESH);
    }

    file.close();
    return ok;
  }

  const bool absolute =
      renderer.grayscaleCapabilities(HalDisplay::GrayscaleMode::Absolute).supported();

  if (absolute) {
    if (!renderer.displayGrayscaleBase(HalDisplay::GrayscaleMode::Absolute)) {
      file.close();
      return false;
    }
  } else {
    renderer.displayGrayscaleBase(HalDisplay::HALF_REFRESH);
  }

  bool planesReady = true;

  for (const auto mode : {GfxRenderer::GRAYSCALE_LSB, GfxRenderer::GRAYSCALE_MSB}) {
    if (bitmap.rewindToData() != BmpReaderError::Ok) {
      planesReady = false;
      break;
    }

    renderer.clearScreen(absolute ? 0xFF : 0x00);
    renderer.setRenderMode(mode);

    if (!drawComposedEventFrame(renderer, bitmap)) {
      planesReady = false;
      break;
    }

    if (mode == GfxRenderer::GRAYSCALE_LSB) {
      renderer.copyGrayscaleLsbBuffers();
    } else {
      renderer.copyGrayscaleMsbBuffers();
    }
  }

  if (planesReady) {
    renderer.displayGrayBuffer();
  }

  renderer.setRenderMode(GfxRenderer::BW);
  file.close();

  return planesReady;
}


void PocketReaderGameActivity::drawLocationsScreen() {
  RenderLock lock(*this);

  renderer.setRenderMode(GfxRenderer::BW);
  renderer.clearScreen();

  std::string subtitle = "Steps ";
  subtitle += std::to_string(bankedSteps);

  if (expeditionActive) {
    subtitle += "  |  Active: ";
    subtitle += locationName(currentLocation).c_str();
  }

  drawMonoHeader(renderer, "LOCATIONS", subtitle.c_str());

  constexpr int FIRST_ROW_Y = 105;
  constexpr int ROW_STEP = 72;

  for (int position = 0; position < LOCATION_COUNT; position++) {
    const int location =
        locationForProgressionPosition(position);

    drawLocationRow(
        renderer,
        position,
        location,
        FIRST_ROW_Y + (position * ROW_STEP),
        position == selectedLocationPosition);
  }

  // Physical X4 side rocker. Draw these after all location rows so the
  // labels cannot be covered by the list boxes.
  drawSideControlPill(renderer, 340, "UP", true);
  drawSideControlPill(renderer, 440, "DOWN", true);

  constexpr int LOC_FOOTER_Y = 748;
  constexpr int LOC_FOOTER_X = 14;
  constexpr int LOC_FOOTER_W = 452;
  constexpr int LOC_SEG_W = LOC_FOOTER_W / 4;

  renderer.drawLine(
      LOC_FOOTER_X,
      LOC_FOOTER_Y - 8,
      SCREEN_W - 14,
      LOC_FOOTER_Y - 8,
      true);

  const char* locLabels[4] = {"BACK", "SELECT", "", ""};

  for (int i = 0; i < 4; i++) {
    const int x = LOC_FOOTER_X + (i * LOC_SEG_W);

    if (locLabels[i][0] == '\0') {
      continue;
    }

    const int tw =
        renderer.getTextWidth(SMALL_FONT_ID, locLabels[i]);

    renderer.drawText(
        SMALL_FONT_ID,
        x + ((LOC_SEG_W - tw) / 2),
        LOC_FOOTER_Y,
        locLabels[i],
        true);
  }

  renderer.displayBuffer(HalDisplay::HALF_REFRESH);
}

void PocketReaderGameActivity::drawLeaveConfirmScreen() {
  RenderLock lock(*this);

  renderer.setRenderMode(GfxRenderer::BW);
  renderer.clearScreen();

  // Custom header for this confirmation screen. The standard mono header rule
  // sat too close to the large title and visually cut through it.
  renderer.drawCenteredText(
      NOTOSERIF_18_FONT_ID,
      28,
      "LEAVE ADVENTURE?",
      true,
      EpdFontFamily::BOLD);

  renderer.drawLine(UI_MARGIN, 82, SCREEN_W - UI_MARGIN, 82, true);

  const int oldProgress =
      validLocation(currentLocation)
          ? locationProgress[currentLocation]
          : 0;

  std::string warning =
      "Leaving ";
  warning += locationName(currentLocation).c_str();
  warning += " will reset this run.";

  std::string progress =
      "Current progress: ";
  progress += std::to_string(oldProgress);
  progress += "/100";

  const std::string kept =
      "XP, companions, boss history and unlocks are kept.";

  std::string target =
      "Continue to ";
  target += validLocation(pendingTargetLocation)
      ? locationName(pendingTargetLocation).c_str()
      : "the selected location";
  target += "?";

  constexpr int BODY_W = 390;

  const auto warningLines =
      renderer.wrappedText(
          NOTOSERIF_12_FONT_ID,
          warning.c_str(),
          BODY_W,
          4);

  int y = 145;

  for (const auto& line : warningLines) {
    renderer.drawCenteredText(
        NOTOSERIF_12_FONT_ID,
        y,
        line.c_str(),
        true,
        EpdFontFamily::BOLD);

    y += renderer.getLineHeight(NOTOSERIF_12_FONT_ID);
  }

  y += 24;

  renderer.drawCenteredText(
      NOTOSERIF_14_FONT_ID,
      y,
      progress.c_str(),
      true);

  y += renderer.getLineHeight(NOTOSERIF_14_FONT_ID) + 55;

  const auto keptLines =
      renderer.wrappedText(
          NOTOSERIF_12_FONT_ID,
          kept.c_str(),
          BODY_W,
          4);

  for (const auto& line : keptLines) {
    renderer.drawCenteredText(
        NOTOSERIF_12_FONT_ID,
        y,
        line.c_str(),
        true);

    y += renderer.getLineHeight(NOTOSERIF_12_FONT_ID);
  }

  y += 58;

  const auto targetLines =
      renderer.wrappedText(
          NOTOSERIF_12_FONT_ID,
          target.c_str(),
          BODY_W,
          4);

  for (const auto& line : targetLines) {
    renderer.drawCenteredText(
        NOTOSERIF_12_FONT_ID,
        y,
        line.c_str(),
        true,
        EpdFontFamily::BOLD);

    y += renderer.getLineHeight(NOTOSERIF_12_FONT_ID);
  }

  // Two large choice boxes. drawCenteredText() centres on the whole screen,
  // so explicitly centre each label inside its own button instead.
  constexpr int BUTTON_Y = 560;
  constexpr int BUTTON_H = 74;
  constexpr int BUTTON_W = 160;
  constexpr int STAY_X = 55;
  constexpr int LEAVE_X = 265;

  renderer.fillRoundedRect(
      STAY_X, BUTTON_Y, BUTTON_W, BUTTON_H, 10, Color::White);
  renderer.drawRoundedRect(
      STAY_X, BUTTON_Y, BUTTON_W, BUTTON_H, 1, 10, true);

  renderer.fillRoundedRect(
      LEAVE_X, BUTTON_Y, BUTTON_W, BUTTON_H, 10, Color::LightGray);
  renderer.drawRoundedRect(
      LEAVE_X, BUTTON_Y, BUTTON_W, BUTTON_H, 1, 10, true);

  const char* stayText = "STAY";
  const char* leaveText = "LEAVE";

  const int stayW =
      renderer.getTextWidth(
          NOTOSERIF_14_FONT_ID,
          stayText,
          EpdFontFamily::BOLD);

  const int leaveW =
      renderer.getTextWidth(
          NOTOSERIF_14_FONT_ID,
          leaveText,
          EpdFontFamily::BOLD);

  const int buttonTextY =
      BUTTON_Y + ((BUTTON_H - renderer.getLineHeight(NOTOSERIF_14_FONT_ID)) / 2);

  renderer.drawText(
      NOTOSERIF_14_FONT_ID,
      STAY_X + ((BUTTON_W - stayW) / 2),
      buttonTextY,
      stayText,
      true,
      EpdFontFamily::BOLD);

  renderer.drawText(
      NOTOSERIF_14_FONT_ID,
      LEAVE_X + ((BUTTON_W - leaveW) / 2),
      buttonTextY,
      leaveText,
      true,
      EpdFontFamily::BOLD);

  // Four physical front controls, in the same left-to-right order as the X4.
  constexpr int LEGEND_Y = 748;
  constexpr int LEGEND_X = 14;
  constexpr int LEGEND_W = 452;
  constexpr int LEGEND_SEG_W = LEGEND_W / 4;

  renderer.drawLine(
      LEGEND_X,
      LEGEND_Y - 8,
      SCREEN_W - 14,
      LEGEND_Y - 8,
      true);

  const char* legend[4] = {
      "BACK",
      "CONFIRM",
      "STAY",
      "LEAVE",
  };

  for (int i = 0; i < 4; i++) {
    const int x = LEGEND_X + (i * LEGEND_SEG_W);
    const int tw =
        renderer.getTextWidth(SMALL_FONT_ID, legend[i]);

    renderer.drawText(
        SMALL_FONT_ID,
        x + ((LEGEND_SEG_W - tw) / 2),
        LEGEND_Y,
        legend[i],
        true);
  }

  renderer.displayBuffer(HalDisplay::HALF_REFRESH);
}
void PocketReaderGameActivity::drawReplayConfirmScreen() {
  RenderLock lock(*this);

  renderer.setRenderMode(GfxRenderer::BW);
  renderer.clearScreen();

  drawMonoHeader(renderer, "REPLAY LOCATION?");

  std::string name =
      validLocation(pendingTargetLocation)
          ? locationName(pendingTargetLocation).c_str()
          : "Completed location";

  renderer.drawCenteredText(
      NOTOSERIF_18_FONT_ID,
      180,
      name.c_str(),
      true,
      EpdFontFamily::BOLD);

  renderer.drawCenteredText(
      NOTOSERIF_12_FONT_ID,
      270,
      "Replay starts a new run at 0/100.",
      true);

  renderer.drawCenteredText(
      NOTOSERIF_12_FONT_ID,
      310,
      "Levels, XP, unlocks and boss history stay permanently.",
      true);

  if (bankedSteps <= 0) {
    renderer.drawCenteredText(
        UI_10_FONT_ID,
        380,
        "You need at least one Adventure Step to begin.",
        true);
  }

  renderer.fillRoundedRect(54, 520, 155, 70, 10, Color::White);
  renderer.drawRoundedRect(54, 520, 155, 70, 1, 10, true);
  renderer.drawCenteredText(
      NOTOSERIF_14_FONT_ID,
      542,
      "CANCEL",
      true,
      EpdFontFamily::BOLD);

  renderer.fillRoundedRect(271, 520, 155, 70, 10, Color::LightGray);
  renderer.drawRoundedRect(271, 520, 155, 70, 1, 10, true);
  renderer.drawCenteredText(
      NOTOSERIF_14_FONT_ID,
      542,
      "REPLAY",
      true,
      EpdFontFamily::BOLD);

  drawMonoFooter(renderer, "BACK / LEFT", "", "CONFIRM / RIGHT");

  renderer.displayBuffer(HalDisplay::HALF_REFRESH);
}

void PocketReaderGameActivity::drawMissingAssetMessage(const char* path) {
  renderer.clearScreen();

  renderer.drawCenteredText(
      NOTOSERIF_18_FONT_ID,
      150,
      "Pocket Reader v19",
      true,
      EpdFontFamily::BOLD);

  renderer.drawCenteredText(
      NOTOSERIF_14_FONT_ID,
      235,
      "Artwork file not found.",
      true);

  const auto pathLines =
      renderer.wrappedText(UI_10_FONT_ID, path, renderer.getScreenWidth() - 50, 4);

  int y = 300;

  for (const auto& line : pathLines) {
    renderer.drawCenteredText(UI_10_FONT_ID, y, line.c_str(), true);
    y += renderer.getLineHeight(UI_10_FONT_ID) + 4;
  }

  renderer.drawCenteredText(
      UI_10_FONT_ID,
      430,
      "Copy /PocketReader from the",
      true);

  renderer.drawCenteredText(
      UI_10_FONT_ID,
      458,
      "Phase 1 package to the SD root.",
      true);
}
