#include "nrf_gpio.h"
#include "nrf_delay.h"

#include "messages.h"
#include "GameBoard.h"
#include "player2.h"

#define TX_PIN_NRF NRF_GPIO_PIN_MAP(1,3)
#define RX_PIN_NRF NRF_GPIO_PIN_MAP(0,28)

#define BIT_PERIOD_US   50
#define HALF_BIT_US     (BIT_PERIOD_US / 2)
#define PREAMBLE_BITS   23
#define RX_TIMEOUT_US   30000

#define RX_INVERTED 1

static const uint16_t BOT_ID = 0x02C4;
static const TokenColor MY_COLOR  = TokenColor::RED;
static const TokenColor OPP_COLOR = TokenColor::YELLOW;

inline void sendLineLevel(uint8_t level) {
  nrf_gpio_pin_write(TX_PIN_NRF, level);
  nrf_delay_us(BIT_PERIOD_US);
}

void sendPDU(uint16_t pdu) {
  __disable_irq();

  nrf_gpio_pin_write(TX_PIN_NRF, 1);

  for (int i = 0; i < PREAMBLE_BITS; i++) {
    sendLineLevel(0);
  }

  sendLineLevel(1);

  for (int i = 0; i < 16; i++) {
    uint8_t bitVal = (pdu >> i) & 0x01;
    sendLineLevel(!bitVal);
  }

  nrf_gpio_pin_write(TX_PIN_NRF, 1);

  __enable_irq();
}

inline uint8_t readProtocolLevel() {
  uint8_t raw = nrf_gpio_pin_read(RX_PIN_NRF);
  return RX_INVERTED ? !raw : raw;
}

void printBinary16(uint16_t value) {
  for (int i = 15; i >= 0; i--) {
    Serial.print((value >> i) & 0x01);
  }
  Serial.println();
}

bool syncToFrameStart() {
  uint32_t t0 = micros();

  while (readProtocolLevel() != 0) {
    if ((uint32_t)(micros() - t0) > RX_TIMEOUT_US) {
      return false;
    }
  }

  while (readProtocolLevel() == 0) {
    if ((uint32_t)(micros() - t0) > RX_TIMEOUT_US) {
      return false;
    }
  }

  __disable_irq();

  nrf_delay_us((PREAMBLE_BITS - 1) * BIT_PERIOD_US + HALF_BIT_US);

  if (readProtocolLevel() != 1) {
    __enable_irq();
    return false;
  }

  nrf_delay_us(BIT_PERIOD_US);

  if (readProtocolLevel() != 0) {
    __enable_irq();
    return false;
  }

  nrf_delay_us(BIT_PERIOD_US);

  return true;
}

bool readPDU(uint16_t &pdu) {
  pdu = 0;

  if (!syncToFrameStart()) {
    return false;
  }

  for (int i = 0; i < 16; i++) {
    uint8_t bitVal = readProtocolLevel();
    if (bitVal) {
      pdu |= (1u << i);
    }

    if (i < 15) {
      nrf_delay_us(BIT_PERIOD_US);
    }
  }

  __enable_irq();
  return true;
}

static const uint8_t MSG_START_GAME     = 0b001;
static const uint8_t MSG_START_GAME_ACK = 0b010;
static const uint8_t MSG_MOVE           = 0b100;
static const uint8_t INVALID_MOVE_COL   = 0b11111;

GameBoard gameBoard(6, 6, 4);
SolverPlayer myPlayer(MY_COLOR, gameBoard);

struct AcceptedMove {
  uint8_t col;
  bool mine;
};

AcceptedMove acceptedMoves[225];
uint16_t acceptedMoveCount = 0;

bool gameConfigured = false;
uint8_t lastRxSeq = 0;

bool myMovePending = false;
uint8_t pendingBaseRxSeq = 0;
uint8_t pendingMoveCol = 255;

uint16_t moveTimeoutMs = 200;

uint8_t getPduType(uint16_t pdu) {
  return (pdu >> 13) & 0x07;
}

uint16_t makeStartGameAckPdu(uint16_t botId) {
  return (uint16_t(MSG_START_GAME_ACK) << 13) | (botId & 0x1FFF);
}

uint16_t makeMovePdu(uint8_t seq, uint8_t moveCol) {
  return (uint16_t(MSG_MOVE) << 13) |
         ((uint16_t(seq) & 0xFF) << 5) |
         (uint16_t(moveCol) & 0x1F);
}

bool parseStartGamePdu(uint16_t pdu,
                       uint8_t &mtoBits,
                       uint8_t &numCols,
                       uint8_t &numRows,
                       uint8_t &inALine) {
  if (getPduType(pdu) != MSG_START_GAME) return false;

  mtoBits  = (pdu >> 11) & 0x03;
  numCols  = (pdu >> 7)  & 0x0F;
  numRows  = (pdu >> 3)  & 0x0F;
  inALine  = (pdu >> 0)  & 0x07;
  return true;
}

bool parseMovePdu(uint16_t pdu, uint8_t &seq, uint8_t &moveCol) {
  if (getPduType(pdu) != MSG_MOVE) return false;

  seq     = (pdu >> 5) & 0xFF;
  moveCol = (pdu >> 0) & 0x1F;
  return true;
}

bool isValidBoardConfig(uint8_t numCols, uint8_t numRows, uint8_t inALine) {
  return (numCols >= 6 && numCols <= 15) &&
         (numRows >= 6 && numRows <= 15) &&
         (inALine >= 3 && inALine <= 7) &&
         (inALine <= numCols) &&
         (inALine <= numRows);
}

uint16_t timeoutBitsToMs(uint8_t mtoBits) {
  switch (mtoBits & 0x03) {
    case 0b00: return 100;
    case 0b01: return 200;
    case 0b10: return 400;
    case 0b11: return 600;
  }
  return 0;
}

void clearAcceptedHistory() {
  acceptedMoveCount = 0;
}

bool storeAcceptedMove(uint8_t col, bool mine) {
  if (acceptedMoveCount >= 225) return false;
  acceptedMoves[acceptedMoveCount].col = col;
  acceptedMoves[acceptedMoveCount].mine = mine;
  acceptedMoveCount++;
  return true;
}

bool addMoveToLiveBoard(uint8_t col, bool mine) {
  TokenColor color = mine ? MY_COLOR : OPP_COLOR;

  PutMoveResult r = gameBoard.putMove(col, color);

  if (r == PutMoveResult::VALID_MOVE || r == PutMoveResult::OUT_OF_SEQUENCE) {
    return true;
  }

  Serial.print("putMove failed, code = ");
  Serial.println((int)r);
  return false;
}

void rebuildBoardFromAcceptedHistory() {
  gameBoard.reset();

  for (uint16_t i = 0; i < acceptedMoveCount; i++) {
    addMoveToLiveBoard(acceptedMoves[i].col, acceptedMoves[i].mine);
  }
}

void resetForNewGame(uint8_t rows, uint8_t cols, uint8_t inALine, uint8_t mtoBits) {
  gameBoard.reset(rows, cols, inALine);
  clearAcceptedHistory();
  myPlayer.gameReset();

  gameConfigured = true;
  lastRxSeq = 0;
  moveTimeoutMs = timeoutBitsToMs(mtoBits);

  myMovePending = false;
  pendingBaseRxSeq = 0;
  pendingMoveCol = 255;

  Serial.println("New game configured");
  Serial.print("Rows: ");
  Serial.println(rows);
  Serial.print("Cols: ");
  Serial.println(cols);
  Serial.print("Connect: ");
  Serial.println(inALine);
  Serial.print("Move timeout ms: ");
  Serial.println(moveTimeoutMs);
}

uint8_t chooseMyMove() {
  return myPlayer.getNextMove();
}

void sendStartGameAck() {
  uint16_t ack = makeStartGameAckPdu(BOT_ID);
  sendPDU(ack);

  Serial.print("TX START_GAME_ACK 0x");
  Serial.println(ack, HEX);
}

void sendMyMove(uint8_t col) {
  uint16_t pdu = makeMovePdu(0, col);
  sendPDU(pdu);

  Serial.print("TX MOVE col=");
  Serial.print(col);
  Serial.print(" pdu=0x");
  Serial.println(pdu, HEX);
}

void sendInvalidMyMove() {
  uint16_t pdu = makeMovePdu(0, INVALID_MOVE_COL);
  sendPDU(pdu);

  Serial.print("TX INVALID MOVE pdu=0x");
  Serial.println(pdu, HEX);
}

void resolvePendingMoveAgainstIncoming(uint8_t incomingSeq, bool incomingInvalid) {
  if (!myMovePending) return;

  uint8_t delta = incomingSeq - pendingBaseRxSeq;

  if (delta >= 2 || (delta == 1 && incomingInvalid)) {
    if (pendingMoveCol != 255) {
      storeAcceptedMove(pendingMoveCol, true);
      Serial.println("Previous local move ACCEPTED");
    }
  }
  else if (delta == 1 && !incomingInvalid) {
    Serial.println("Previous local move REJECTED or TOO LATE");
    rebuildBoardFromAcceptedHistory();
  }
  else {
    Serial.print("Unexpected seq delta while resolving pending move: ");
    Serial.println(delta);
  }

  myMovePending = false;
  pendingMoveCol = 255;
}

void playMyTurnAndTransmit() {
  if (!gameConfigured) return;

  if (gameBoard.isGameOver()) {
    Serial.println("Game over locally. Waiting for next START_GAME.");
    return;
  }

  uint8_t myCol = chooseMyMove();

  if (myCol == 255) {
    Serial.println("No legal move found");
    sendInvalidMyMove();
    return;
  }

  if (!addMoveToLiveBoard(myCol, true)) {
    Serial.println("Local add of my move failed");
    sendInvalidMyMove();
    return;
  }

  myMovePending = true;
  pendingBaseRxSeq = lastRxSeq;
  pendingMoveCol = myCol;

  sendMyMove(myCol);
}

void handleStartGamePdu(uint16_t pdu) {
  uint8_t mtoBits, numCols, numRows, inALine;

  if (!parseStartGamePdu(pdu, mtoBits, numCols, numRows, inALine)) {
    return;
  }

  Serial.println("RX START_GAME");

  if (!isValidBoardConfig(numCols, numRows, inALine)) {
    Serial.println("Invalid START_GAME config");
    return;
  }

  resetForNewGame(numRows, numCols, inALine, mtoBits);
  sendStartGameAck();
}

void handleMovePdu(uint16_t pdu) {
  uint8_t seq, moveCol;

  if (!parseMovePdu(pdu, seq, moveCol)) {
    return;
  }

  if (!gameConfigured) {
    Serial.println("MOVE received before START_GAME, ignoring");
    return;
  }

  bool incomingInvalid = (moveCol == INVALID_MOVE_COL);

  Serial.print("RX MOVE seq=");
  Serial.print(seq);
  Serial.print(" col=");
  Serial.println(moveCol);

  resolvePendingMoveAgainstIncoming(seq, incomingInvalid);

  if (seq == 0) {
    Serial.println("Initial MOVE seq=0, not added to board");
  }
  else if (incomingInvalid) {
    Serial.println("Opponent timeout / invalid move marker received");
  }
  else {
    if (addMoveToLiveBoard(moveCol, false)) {
      storeAcceptedMove(moveCol, false);
      Serial.println("Opponent move added");
    } else {
      Serial.println("Failed to add opponent move");
    }
  }

  lastRxSeq = seq;

  playMyTurnAndTransmit();
}

void handleIncomingPdu(uint16_t pdu) {
  uint8_t type = getPduType(pdu);

  Serial.print("RX PDU HEX: 0x");
  Serial.println(pdu, HEX);

  Serial.print("RX PDU BIN: ");
  printBinary16(pdu);

  if (type == MSG_START_GAME) {
    handleStartGamePdu(pdu);
  }
  else if (type == MSG_MOVE) {
    handleMovePdu(pdu);
  }
  else {
    Serial.println("Unknown or unexpected PDU type");
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);

  nrf_gpio_cfg_output(TX_PIN_NRF);
  nrf_gpio_pin_write(TX_PIN_NRF, 1);

  nrf_gpio_cfg_input(RX_PIN_NRF, NRF_GPIO_PIN_NOPULL);

  Serial.println("AI Bot main sketch starting...");
}

void loop() {
  uint16_t rxPdu = 0;

  if (readPDU(rxPdu)) {
    handleIncomingPdu(rxPdu);
  }
}