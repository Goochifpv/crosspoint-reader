#include "PocketReaderGameActivity.h"

#include <algorithm>
#include <string>

#include <Bitmap.h>
#include <GfxRenderer.h>
#include <HalDisplay.h>
#include <HalStorage.h>

#include "activities/ActivityManager.h"
#include "fontIds.h"

#include "pocket_game/v19/AdventureEngine.h"
#include "pocket_game/v19/GameState.h"
#include "pocket_game/v19/SaveSystem.h"

namespace {

using Button = MappedInputManager::Button;

constexpr int CARD_X = 12;
constexpr int CARD_W = 456;
constexpr int SIDE_PAD = 22;
constexpr int CORNER = 12;

constexpr int FOOTER_X = 12;
constexpr int FOOTER_Y = 756;
constexpr int FOOTER_W = 456;
constexpr int FOOTER_H = 36;

constexpr int CARD_MIN_H = 160;
constexpr int CARD_MAX_H = 270;
constexpr int CARD_GAP_TO_FOOTER = 16;

constexpr int RESULT_BOX_SIDE_PAD = 10;
constexpr int RESULT_BOX_TOP_PAD = 7;
constexpr int RESULT_BOX_BOTTOM_PAD = 7;

std::string toStdString(const String& value) {
  return std::string(value.c_str());
}

std::string signedNumber(const int value) {
  if (value > 0) return "+" + std::to_string(value);
  return std::to_string(value);
}

bool isStepInput(MappedInputManager& input) {
  return input.wasReleased(Button::Right) ||
         input.wasReleased(Button::Down) ||
         input.wasReleased(Button::PageForward) ||
         input.wasReleased(Button::NavNext);
}

}  // namespace

PocketReaderGameActivity::PocketReaderGameActivity(
    GfxRenderer& renderer,
    MappedInputManager& mappedInput)
    : Activity("PocketReaderGame", renderer, mappedInput) {}

void PocketReaderGameActivity::onEnter() {
  Activity::onEnter();

  renderer.setOrientation(GfxRenderer::Portrait);

  loadGame();

  hasEvent = false;
  statusMessage =
      "V19 gameplay engine loaded. +100P banks ten Adventure Steps; STEP runs a real event.";

  drawGame();
}

void PocketReaderGameActivity::onExit() {
  Activity::onExit();
}

void PocketReaderGameActivity::loop() {
  Activity::loop();

  if (mappedInput.wasReleased(Button::Back)) {
    activityManager.goHome();
    return;
  }

  // Development bridge until reading-session integration is connected.
  if (mappedInput.wasReleased(Button::Confirm)) {
    grantDevPages();
    return;
  }

  // First real-engine proof: STEP chooses a valid v19 direction automatically.
  if (isStepInput(mappedInput)) {
    performNextStep();
    return;
  }
}

void PocketReaderGameActivity::grantDevPages() {
  addPagesRead(100);

  statusMessage =
      "+100 pages read. Banked Adventure Steps: " + std::to_string(bankedSteps);

  drawGame();
}

void PocketReaderGameActivity::performNextStep() {
  if (bankedSteps <= 0) {
    statusMessage = "No Adventure Steps. Press +100P first.";
    drawGame();
    return;
  }

  if (locationProgress[currentLocation] >= 100) {
    statusMessage = "This location is complete.";
    drawGame();
    return;
  }

  if (!expeditionActive) {
    startExpedition();
  }

  const int direction = randomAllowedDirection();
  performAdventureStep(direction);

  hasEvent = true;
  statusMessage.clear();

  drawGame();
}

const char* PocketReaderGameActivity::backgroundForLocation(const int location) const {
  // Reuse the two already-proven X4 assets for this first backend proof.
  if (location == LOC_MISTY_MARSH) {
    return "/PocketReaderTest/misty_marshes.bmp";
  }

  if (location == LOC_WHISPERWOOD) {
    return "/PocketReaderTest/whispering_woods.bmp";
  }

  // Temporary safe fallback until the remaining six X4 masters are routed.
  return "/PocketReaderTest/whispering_woods.bmp";
}

bool PocketReaderGameActivity::overlayAtTopForLocation(const int location) const {
  return location == LOC_MISTY_MARSH;
}

std::string PocketReaderGameActivity::eventTitle() const {
  if (!hasEvent || currentEvent.title.length() == 0) {
    return "V19 ENGINE READY";
  }

  return toStdString(currentEvent.title);
}

std::string PocketReaderGameActivity::eventBody() const {
  if (!hasEvent) {
    return statusMessage;
  }

  std::string body = toStdString(currentEvent.line1);

  if (currentEvent.line2.length() > 0) {
    if (!body.empty()) body += " ";
    body += toStdString(currentEvent.line2);
  }

  if (!statusMessage.empty()) {
    if (!body.empty()) body += " ";
    body += statusMessage;
  }

  return body;
}

std::string PocketReaderGameActivity::eventResultBlock() const {
  std::string result;

  if (hasEvent && currentEvent.result.length() > 0) {
    result += toStdString(currentEvent.result);
  } else if (!statusMessage.empty()) {
    result += statusMessage;
  }

  if (hasEvent) {
    if (!result.empty()) result += " | ";

    // Canonical display order: Strength -> Heart -> Luck.
    result += "S" + signedNumber(currentEvent.xpDelta[ROLE_STRENGTH]);
    result += " H" + signedNumber(currentEvent.xpDelta[ROLE_HEART]);
    result += " L" + signedNumber(currentEvent.xpDelta[ROLE_LUCK]);
  }

  if (!result.empty()) result += " | ";

  result += toStdString(locationName(currentLocation));
  result += " ";
  result += std::to_string(locationProgress[currentLocation]);
  result += "/100";
  result += " | Steps ";
  result += std::to_string(bankedSteps);

  if (pendingCompanionReveal >= 0 &&
      pendingCompanionReveal < ROSTER_SIZE) {
    result += " | NEW FRIEND: ";
    result += toStdString(roster[pendingCompanionReveal].name);
  }

  return result;
}

void PocketReaderGameActivity::drawGame() {
  RenderLock lock(*this);

  renderer.setRenderMode(GfxRenderer::BW);
  renderer.clearScreen();

  const char* path = backgroundForLocation(currentLocation);

  if (!drawSceneBitmap(path)) {
    drawMissingAssetMessage(path);
    renderer.displayBuffer(HalDisplay::HALF_REFRESH);
  }
}

bool PocketReaderGameActivity::drawSceneBitmap(const char* path) {
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

  const int pageWidth = renderer.getScreenWidth();
  const int pageHeight = renderer.getScreenHeight();

  renderer.clearScreen();

  if (!bitmap.hasGreyscale()) {
    renderer.setRenderMode(GfxRenderer::BW);

    if (!renderer.drawBitmap(bitmap, 0, 0, pageWidth, pageHeight, 0, 0)) {
      file.close();
      return false;
    }

    drawOverlay();

    renderer.displayBuffer(HalDisplay::HALF_REFRESH);
    file.close();
    return true;
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

    if (!renderer.drawBitmap(bitmap, 0, 0, pageWidth, pageHeight, 0, 0)) {
      planesReady = false;
      break;
    }

    drawOverlay();

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

void PocketReaderGameActivity::drawOverlay() {
  const int screenWidth = renderer.getScreenWidth();

  const std::string title = eventTitle();
  const std::string body = eventBody();
  const std::string resultText = eventResultBlock();

  const int textX = CARD_X + SIDE_PAD;
  const int textWidth = CARD_W - (SIDE_PAD * 2);

  const int titleFont = NOTOSERIF_14_FONT_ID;
  const int bodyFont = NOTOSERIF_12_FONT_ID;
  const int resultFont = UI_10_FONT_ID;

  const auto bodyLines =
      renderer.wrappedText(bodyFont, body.c_str(), textWidth, 6);

  const int resultTextWidth = textWidth - (RESULT_BOX_SIDE_PAD * 2);

  const auto resultLines =
      renderer.wrappedText(
          resultFont,
          resultText.c_str(),
          resultTextWidth,
          4,
          EpdFontFamily::BOLD);

  const int titleH = renderer.getLineHeight(titleFont);
  const int bodyLineH = renderer.getLineHeight(bodyFont);
  const int resultLineH = renderer.getLineHeight(resultFont);

  const int resultBoxH =
      RESULT_BOX_TOP_PAD +
      static_cast<int>(resultLines.size()) * resultLineH +
      RESULT_BOX_BOTTOM_PAD;

  int cardH = 0;
  cardH += 11;
  cardH += titleH;
  cardH += 4;
  cardH += 8;
  cardH += static_cast<int>(bodyLines.size()) * bodyLineH;
  cardH += 8;
  cardH += resultBoxH;
  cardH += 13;

  cardH = std::max(CARD_MIN_H, std::min(CARD_MAX_H, cardH));

  const int cardY =
      overlayAtTopForLocation(currentLocation)
          ? 18
          : (FOOTER_Y - CARD_GAP_TO_FOOTER - cardH);

  renderer.fillRoundedRect(CARD_X, cardY, CARD_W, cardH, CORNER, Color::White);
  renderer.drawRoundedRect(CARD_X, cardY, CARD_W, cardH, 2, CORNER, true);

  renderer.fillRoundedRect(CARD_X + 8, cardY - 5, CARD_W - 16, 13, 7, Color::LightGray);
  renderer.drawRoundedRect(CARD_X + 8, cardY - 5, CARD_W - 16, 13, 1, 7, true);
  renderer.fillRoundedRect(CARD_X + 8, cardY + cardH - 8, CARD_W - 16, 13, 7, Color::LightGray);
  renderer.drawRoundedRect(CARD_X + 8, cardY + cardH - 8, CARD_W - 16, 13, 1, 7, true);

  int cursorY = cardY + 11;

  renderer.drawCenteredText(
      titleFont,
      cursorY,
      title.c_str(),
      true,
      EpdFontFamily::BOLD);

  cursorY += titleH + 3;

  renderer.drawLine(
      textX,
      cursorY,
      CARD_X + CARD_W - SIDE_PAD,
      cursorY,
      true);

  cursorY += 8;

  for (const auto& line : bodyLines) {
    renderer.drawText(bodyFont, textX, cursorY, line.c_str(), true);
    cursorY += bodyLineH;
  }

  cursorY += 7;

  const int resultBoxX = textX;
  const int resultBoxY = cursorY;
  const int resultBoxW = textWidth;

  renderer.fillRoundedRect(
      resultBoxX,
      resultBoxY,
      resultBoxW,
      resultBoxH,
      7,
      Color::White);

  renderer.drawRoundedRect(
      resultBoxX,
      resultBoxY,
      resultBoxW,
      resultBoxH,
      1,
      7,
      true);

  int resultY = resultBoxY + RESULT_BOX_TOP_PAD;

  for (const auto& line : resultLines) {
    renderer.drawText(
        resultFont,
        resultBoxX + RESULT_BOX_SIDE_PAD,
        resultY,
        line.c_str(),
        true,
        EpdFontFamily::BOLD);

    resultY += resultLineH;
  }

  renderer.fillRoundedRect(FOOTER_X, FOOTER_Y, FOOTER_W, FOOTER_H, 8, Color::White);
  renderer.drawRoundedRect(FOOTER_X, FOOTER_Y, FOOTER_W, FOOTER_H, 1, 8, true);

  const int footerTextY = FOOTER_Y + 10;
  constexpr int FOOTER_PAD = 18;

  renderer.drawText(
      SMALL_FONT_ID,
      FOOTER_X + FOOTER_PAD,
      footerTextY,
      "BACK",
      true);

  const char* addPages = "+100P";
  renderer.drawText(
      SMALL_FONT_ID,
      (screenWidth - renderer.getTextWidth(SMALL_FONT_ID, addPages)) / 2,
      footerTextY,
      addPages,
      true);

  const char* step = "STEP";
  renderer.drawText(
      SMALL_FONT_ID,
      FOOTER_X + FOOTER_W - FOOTER_PAD -
          renderer.getTextWidth(SMALL_FONT_ID, step),
      footerTextY,
      step,
      true);
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
      renderer.wrappedText(UI_10_FONT_ID, path, renderer.getScreenWidth() - 50, 3);

  int y = 295;
  for (const auto& line : pathLines) {
    renderer.drawCenteredText(UI_10_FONT_ID, y, line.c_str(), true);
    y += renderer.getLineHeight(UI_10_FONT_ID) + 4;
  }

  renderer.drawCenteredText(
      UI_10_FONT_ID,
      430,
      "Keep /PocketReaderTest on the SD card.",
      true);

  renderer.drawCenteredText(
      UI_10_FONT_ID,
      500,
      "BACK returns to normal CrossPoint.",
      true);
}
