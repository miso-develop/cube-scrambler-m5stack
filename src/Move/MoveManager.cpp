#include "MoveManager.h"

namespace move {

bool MoveManager::parseAndConvert(const char* sequence, MoveSequence& parsed,
                                  RobotMoveSequence& converted, Stream* log) {
  if (!MoveParser::parseSequence(sequence, parsed, log)) return false;
  return converter_.convert(parsed, converted, log);
}

bool MoveManager::convertText(const char* sequence,
                              RobotMoveSequence& converted, Stream* log) {
  MoveSequence parsed{};
  return parseAndConvert(sequence, parsed, converted, log);
}

bool MoveManager::startConverted(const RobotMoveSequence& converted,
                                 Stream& out) {
  return runner_.start(converted, out);
}

bool MoveManager::startText(const char* sequence, Stream& out) {
  MoveSequence parsed{};
  RobotMoveSequence converted{};
  if (!parseAndConvert(sequence, parsed, converted, &out)) return false;

  out.printf("Parsed moves: %u\n", static_cast<unsigned>(parsed.count));
  printRobotSequence(converted, out);
  return runner_.start(converted, out);
}

bool MoveManager::runTextBlocking(const char* sequence, Stream& out) {
  MoveSequence parsed{};
  RobotMoveSequence converted{};
  if (!parseAndConvert(sequence, parsed, converted, &out)) return false;

  out.printf("Parsed moves: %u\n", static_cast<unsigned>(parsed.count));
  printRobotSequence(converted, out);
  return runner_.runBlocking(converted, out);
}

void MoveManager::printRobotSequence(const RobotMoveSequence& sequence,
                                     Stream& out) const {
  out.printf("Robot moves (%u): ", static_cast<unsigned>(sequence.count));
  for (size_t i = 0; i < sequence.count; ++i) {
    char notation[4]{};
    MoveParser::formatRobotMove(sequence.items[i], notation, sizeof(notation));
    if (i > 0) out.print(' ');
    out.print(notation);
  }
  out.println();
  out.flush();
}

void MoveManager::printRunnerStatus(Stream& out) const {
  const MoveRunner::Status status = runner_.status();
  out.println();
  out.println("Move runner status");
  out.printf("State: %s\n", MoveRunner::stateName(status.state));
  out.printf("Robot ready: %s\n", robotReady() ? "YES" : "NO");
  out.printf("Progress: %u/%u\n", static_cast<unsigned>(status.currentMove),
             static_cast<unsigned>(status.totalMoves));
  out.printf("Stop requested: %s\n", status.stopRequested ? "YES" : "NO");
  if (status.startedMs != 0) {
    const uint32_t endMs = status.finishedMs != 0 ? status.finishedMs : millis();
    out.printf("Elapsed: %u ms\n",
               static_cast<unsigned>(endMs - status.startedMs));
  }
  if (status.workerStackHighWater != 0) {
    out.printf("Worker stack high-water: %u\n",
               static_cast<unsigned>(status.workerStackHighWater));
  }
  out.flush();
}

bool MoveManager::selfTest(Stream& out) {
  out.println();
  out.println("MoveManager self-test");

  MoveSequence parsed{};
  const bool parserBasic = MoveParser::parseSequence("R U R' U'", parsed, &out) &&
                           parsed.count == 4;
  out.printf("Move parser basic sequence: %s\n", parserBasic ? "PASS" : "FAIL");

  MoveToken wide{};
  const bool parserWide = MoveParser::parseMoveToken("F'w2", wide) &&
                          wide.kind == MoveKind::Wide && wide.position == 'F' &&
                          wide.prime && wide.wide && wide.count == 2;
  out.printf("Move parser wide notation: %s\n", parserWide ? "PASS" : "FAIL");

  MoveToken invalid{};
  const bool parserReject = !MoveParser::parseMoveToken("Q", invalid);
  out.printf("Move parser invalid rejection: %s\n",
             parserReject ? "PASS" : "FAIL");

  const bool converterPassed = converter_.selfTest(out);
  const bool passed = parserBasic && parserWide && parserReject && converterPassed;
  out.printf("MoveManager self-test: %s\n", passed ? "PASS" : "FAIL");
  out.flush();
  return passed;
}

}  // namespace move
