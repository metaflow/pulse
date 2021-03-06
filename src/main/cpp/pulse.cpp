/*
 * Copyright (C) 2013-2016 Phokham Nonava
 *
 * Use of this source code is governed by the MIT license that can be
 * found in the LICENSE file.
 */

#include "pulse.h"

#include <iostream>
#include <memory>
#include <queue>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace pulse {

// std::pair<int, int> toXY(int square) {
// return std::make_pair(square % 16, square / 16);
// }

void printSquare(int square) {
  int x = square % 16;
  int y = square / 16;
  std::cout << square << ' ' << Notation::fromSquare(square) << " (" << x
            << ", " << y << ")";
}

void printMove(int m) {
  std::cout << "type " << Move::getType(m);
  std::cout << " from ";
  printSquare(Move::getOriginSquare(m));
  std::cout << " to ";
  printSquare(Move::getTargetSquare(m));
  std::cout << " piece " << Move::getOriginPiece(m) << ' '
            << Notation::fromPiece(Move::getOriginPiece(m));
  std::cout << " target " << Move::getTargetPiece(m);
  std::cout << " promotion " << Move::getPromotion(m);
  if (Piece::NOPIECE != Move::getPromotion(m))
    std::cout << ' ' << Notation::fromPiece(Move::getPromotion(m));
  std::cout << std::endl;
}

int sSquare(int s) {
  if (!Square::isValid(s)) return s;
  int x = Square::getFile(s);
  int y = Square::getRank(s);
  y = 7 - y;
  return Square::valueOf(x, y);
}

int sPiece(int p) {
  if (!Piece::isValid(p)) return p;
  return Piece::valueOf(Color::opposite(Piece::getColor(p)), Piece::getType(p));
}

int sMove(int move) {
  int type = Move::getType(move);
  int originSquare = Move::getOriginSquare(move);
  int targetSquare = Move::getTargetSquare(move);
  int originPiece = Move::getOriginPiece(move);
  int targetPiece = Move::getTargetPiece(move);
  int promotion = Move::getPromotion(move);
  return Move::valueOf(type, sSquare(originSquare), sSquare(targetSquare),
                       sPiece(originPiece), sPiece(targetPiece),
                       promotion);
}

bool record(int m, std::shared_ptr<Position> t,
            std::unordered_map<int, std::shared_ptr<Position>>& z) {
  int piece = Move::getOriginPiece(m);
  if (t->isPromoted(Move::getTargetSquare(m))) piece = Piece::NOPIECE;
  if (z.count(piece)) return false;
  auto s = Notation::fromPosition(*t);
  std::cout << std::endl;
  std::cout << "checkmate ";
  if (piece != Piece::NOPIECE)
    std::cout << Notation::fromPiece(piece);
  else
    std::cout << Notation::fromPiece(Move::getOriginPiece(m)) << " promoted";
  std::cout << std::endl;
  std::cout << s << std::endl;
  std::cout << "promoted squares ";
  for (int i = 0; i < Square::VALUES_LENGTH; i++) {
    if (t->isPromoted(i)) std::cout << Notation::fromSquare(i) << ' ';
  }
  std::cout << std::endl;
  // std::cout << t->movedPawns << " pawns touched" << std::endl;
  printMove(m);
  std::cout << t->moves.size() << " moves" << std::endl;
  int turn = 1;
  for (auto x : t->moves) {
    turn++;
    if (turn % 2 == 0) std::cout << ' ' << turn / 2 << ".";
    std::cout << " ";
    int type = Piece::getType(Move::getOriginPiece(x));
    if (type != PieceType::PAWN) { std::cout << Notation::fromPieceType(type); }
    std::cout << Notation::fromSquare(Move::getOriginSquare(x));
    if (Move::getTargetPiece(x) != Piece::NOPIECE) { std::cout << 'x'; }
    std::cout << Notation::fromSquare(Move::getTargetSquare(x));
    if (Move::getType(x) == MoveType::PAWNPROMOTION) {
      std::cout << '=' << Notation::fromPiece(Move::getPromotion(x));
    }
    if (Move::getType(x) == MoveType::CASTLING) {
      std::cout << "CASTLING!!!" << std::endl;
    }
  }
  std::cout << "#" << std::endl;
  z.emplace(piece, new Position(*t));
  std::cout << "found " << z.size() << " answers " << std::endl;
  return z.size() == 7;
}

bool dfs(std::unique_ptr<Position>                           p,
         std::unordered_map<int, std::shared_ptr<Position>>& z, int depth,
         std::unordered_map<uint64_t, int>& visited) {
  if (depth < 0) return false;
  MoveGenerator moveGenerator;
  MoveEntryList moves;
  moveGenerator.getLegalMoves(*p, 1, p->isCheck(), moves);
  for (int i = 0; i < moves.size; i++) {
    int                       m = moves.entries[i].move;
    int                       originSquare = Move::getOriginSquare(m);
    int                       ox = originSquare % 16;
    int                       oy = originSquare / 16;
    int                       targetSquare = Move::getTargetSquare(m);
    int                       tx = targetSquare % 16;
    int                       ty = targetSquare / 16;
    int                       originPiece = Move::getOriginPiece(m);
    std::shared_ptr<Position> t(new Position(*p));
    t->makeMove(m);
    if ((originPiece != Piece::WHITE_PAWN && p->touched[originPiece] > 1) ||
        t->touched[originPiece] > 3)
      continue;
    MoveEntryList bmoves;
    bool check = t->isCheck();
    moveGenerator.getLegalMoves(*t, 1, check, bmoves);
    // find symmeric move for black.
    int w = Move::NOMOVE;
    for (int j = 0; j < bmoves.size; j++) {
      int bm = bmoves.entries[j].move;
      int originSquare2 = Move::getOriginSquare(bm);
      int o2x = originSquare2 % 16;
      int o2y = originSquare2 / 16;
      int targetSquare2 = Move::getTargetSquare(bm);
      int t2x = targetSquare2 % 16;
      int t2y = targetSquare2 / 16;
      // auto bsource = toXY(Move::getOriginSquare(bm));
      // auto btarget = toXY(Move::getTargetSquare(bm));
      if (ox != o2x || tx != t2x || (oy != 7 - o2y) || (ty != 7 - t2y))
        continue;
      if (Move::getType(m) == MoveType::PAWNPROMOTION) {
        if (Move::getPromotion(m) != Move::getPromotion(bm)) { continue; }
      }
      w = bm;
    }
    if (w == Move::NOMOVE) {
      if (bmoves.size == 0) {
        if (record(m, t, z)) return true;
      }
      continue;
    }
    /*
    int  w = sMove(m);
    if (check) {
      MoveEntryList bmoves;
      moveGenerator.getLegalMoves(*t, 1, check, bmoves);
      if (bmoves.size == 0) { if (record(m, t, z)) return true; }
      bool found = false;
      for (int j = 0; j < bmoves.size; j++) {
        found = found || (w == moves.entries[i].move);
      }
      if (!found) continue;
    } else {
      if (t->board[Move::getOriginSquare(w)] != Move::getOriginPiece(w) ||
          !moveGenerator.isValidMove(*t, check, w)) {
        continue;
      }
    }
    */
    t->makeMove(w);
    auto s = t->hash();
    // auto s = Notation::fromPosition(*t);
    if (visited[s] < depth) {
      visited[s] = depth;
      std::unique_ptr<Position> a(new Position(*t));
      if (dfs(std::move(a), z, depth - 1, visited)) {
        return true;
      }
    }
  }
  return false;
}  // namespace pulse

void Pulse::run() {
  std::cin.exceptions(std::iostream::eofbit | std::iostream::failbit |
                      std::iostream::badbit);
  while (false) {
    std::string line;
    std::getline(std::cin, line);
    std::istringstream input(line);

    std::string token;
    input >> std::skipws >> token;
    if (token == "uci") {
      receiveInitialize();
    } else if (token == "isready") {
      receiveReady();
    } else if (token == "ucinewgame") {
      receiveNewGame();
    } else if (token == "position") {
      receivePosition(input);
    } else if (token == "go") {
      receiveGo(input);
    } else if (token == "stop") {
      receiveStop();
    } else if (token == "ponderhit") {
      receivePonderHit();
    } else if (token == "quit") {
      receiveQuit();
      break;
    }
  }
  std::queue<std::shared_ptr<Position>> q;
  // q.emplace(new Position(Notation::toPosition(Notation::STANDARDPOSITION)));
  // q.emplace(new Position(
  // Notation::toPosition("1k1r2Q1/ppp1P3/8/8/8/8/PPP1p3/1K1R2q1 w - -")));
  int                                                timer = 0;
  std::unordered_map<int, std::shared_ptr<Position>> z;
  int                                                max_moves = 0;
  int                                                d = 1;
  while (z.size() < 7) {
    std::cout << "max depth " << d << std::endl;
    std::unordered_map<uint64_t, int> visited;
    std::unique_ptr<Position>         p(
        new Position(Notation::toPosition(Notation::STANDARDPOSITION)));
    dfs(std::move(p), z, d, visited);
    d++;
  }
  receiveQuit();
}

void Pulse::receiveQuit() {
  // We received a quit command. Stop calculating now and
  // cleanup!
  search->quit();
}

void Pulse::receiveInitialize() {
  search->stop();

  // We received an initialization request.

  // We could do some global initialization here. Probably it would be best
  // to initialize all tables here as they will exist until the end of the
  // program.

  // We must send an initialization answer back!
  std::cout << "id name Pulse 1.7.0-cpp" << std::endl;
  std::cout << "id author Phokham Nonava" << std::endl;
  std::cout << "uciok" << std::endl;
}

void Pulse::receiveReady() {
  // We received a ready request. We must send the token back as soon as we
  // can. However, because we launch the search in a separate thread, our main
  // thread is able to handle the commands asynchronously to the search. If we
  // don't answer the ready request in time, our engine will probably be
  // killed by the GUI.
  std::cout << "readyok" << std::endl;
}

void Pulse::receiveNewGame() {
  search->stop();

  // We received a new game command.

  // Initialize per-game settings here.
  *currentPosition = Notation::toPosition(Notation::STANDARDPOSITION);
}

void Pulse::receivePosition(std::istringstream& input) {
  search->stop();

  // We received an position command. Just setup the position.

  std::string token;
  input >> token;
  if (token == "startpos") {
    *currentPosition = Notation::toPosition(Notation::STANDARDPOSITION);

    if (input >> token) {
      if (token != "moves") {
        std::cerr << "ERROR " << __FILE__ << ' ' << __LINE__ << std::endl;
        throw std::exception();
      }
    }
  } else if (token == "fen") {
    std::string fen;

    while (input >> token) {
      if (token == "moves") {
        break;
      } else {
        fen += token + " ";
      }
    }

    *currentPosition = Notation::toPosition(fen);
  } else {
    std::cerr << "ERROR " << __FILE__ << ' ' << __LINE__ << std::endl;
    throw std::exception();
  }

  MoveGenerator moveGenerator;

  while (input >> token) {
    // Verify moves
    MoveEntryList moves;
    moveGenerator.getLegalMoves(*currentPosition, 1, currentPosition->isCheck(),
                                moves);
    bool found = false;
    for (int i = 0; i < moves.size; i++) {
      int move = moves.entries[i].move;
      if (fromMove(move) == token) {
        currentPosition->makeMove(move);
        found = true;
        break;
      }
    }

    if (!found) {
      std::cerr << "ERROR " << __FILE__ << ' ' << __LINE__ << std::endl;
      throw std::exception();
    }
  }

  // Don't start searching though!
}

void Pulse::receiveGo(std::istringstream& input) {
  search->stop();

  // We received a start command. Extract all parameters from the
  // command and start the search.
  std::string token;
  input >> token;
  if (token == "depth") {
    int searchDepth;
    if (input >> searchDepth) {
      search->newDepthSearch(*currentPosition, searchDepth);
    } else {
      std::cerr << "ERROR " << __FILE__ << ' ' << __LINE__ << std::endl;
      throw std::exception();
    }
  } else if (token == "nodes") {
    uint64_t searchNodes;
    if (input >> searchNodes) {
      search->newNodesSearch(*currentPosition, searchNodes);
    }
  } else if (token == "movetime") {
    uint64_t searchTime;
    if (input >> searchTime) {
      search->newTimeSearch(*currentPosition, searchTime);
    }
  } else if (token == "infinite") {
    search->newInfiniteSearch(*currentPosition);
  } else {
    uint64_t whiteTimeLeft = 1;
    uint64_t whiteTimeIncrement = 0;
    uint64_t blackTimeLeft = 1;
    uint64_t blackTimeIncrement = 0;
    int      searchMovesToGo = 40;
    bool     ponder = false;

    do {
      if (token == "wtime") {
        if (!(input >> whiteTimeLeft)) {
          std::cerr << "ERROR " << __FILE__ << ' ' << __LINE__ << std::endl;
          throw std::exception();
        }
      } else if (token == "winc") {
        if (!(input >> whiteTimeIncrement)) {
          std::cerr << "ERROR " << __FILE__ << ' ' << __LINE__ << std::endl;
          throw std::exception();
        }
      } else if (token == "btime") {
        if (!(input >> blackTimeLeft)) {
          std::cerr << "ERROR " << __FILE__ << ' ' << __LINE__ << std::endl;
          throw std::exception();
        }
      } else if (token == "binc") {
        if (!(input >> blackTimeIncrement)) {
          std::cerr << "ERROR " << __FILE__ << ' ' << __LINE__ << std::endl;
          throw std::exception();
        }
      } else if (token == "movestogo") {
        if (!(input >> searchMovesToGo)) {
          std::cerr << "ERROR " << __FILE__ << ' ' << __LINE__ << std::endl;
          throw std::exception();
        }
      } else if (token == "ponder") {
        ponder = true;
      }
    } while (input >> token);

    if (ponder) {
      search->newPonderSearch(*currentPosition, whiteTimeLeft,
                              whiteTimeIncrement, blackTimeLeft,
                              blackTimeIncrement, searchMovesToGo);
    } else {
      search->newClockSearch(*currentPosition, whiteTimeLeft,
                             whiteTimeIncrement, blackTimeLeft,
                             blackTimeIncrement, searchMovesToGo);
    }
  }

  // Go...
  search->start();
  startTime = std::chrono::system_clock::now();
  statusStartTime = startTime;
}

void Pulse::receivePonderHit() {
  // We received a ponder hit command. Just call ponderhit().
  search->ponderhit();
}

void Pulse::receiveStop() {
  // We received a stop command. If a search is running, stop it.
  search->stop();
}

void Pulse::sendBestMove(int bestMove, int ponderMove) {
  std::cout << "bestmove ";

  if (bestMove != Move::NOMOVE) {
    std::cout << fromMove(bestMove);

    if (ponderMove != Move::NOMOVE) {
      std::cout << " ponder " << fromMove(ponderMove);
    }
  } else {
    std::cout << "nomove";
  }

  std::cout << std::endl;
}

void Pulse::sendStatus(int currentDepth, int currentMaxDepth,
                       uint64_t totalNodes, int currentMove,
                       int currentMoveNumber) {
  if (std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now() - statusStartTime)
          .count() >= 1000) {
    sendStatus(false, currentDepth, currentMaxDepth, totalNodes, currentMove,
               currentMoveNumber);
  }
}

void Pulse::sendStatus(bool force, int currentDepth, int currentMaxDepth,
                       uint64_t totalNodes, int currentMove,
                       int currentMoveNumber) {
  auto timeDelta = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now() - startTime);

  if (force || timeDelta.count() >= 1000) {
    std::cout << "info";
    std::cout << " depth " << currentDepth;
    std::cout << " seldepth " << currentMaxDepth;
    std::cout << " nodes " << totalNodes;
    std::cout << " time " << timeDelta.count();
    std::cout << " nps "
              << (timeDelta.count() >= 1000
                      ? (totalNodes * 1000) / timeDelta.count()
                      : 0);

    if (currentMove != Move::NOMOVE) {
      std::cout << " currmove " << fromMove(currentMove);
      std::cout << " currmovenumber " << currentMoveNumber;
    }

    std::cout << std::endl;

    statusStartTime = std::chrono::system_clock::now();
  }
}

void Pulse::sendMove(RootEntry entry, int currentDepth, int currentMaxDepth,
                     uint64_t totalNodes) {
  auto timeDelta = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now() - startTime);

  std::cout << "info";
  std::cout << " depth " << currentDepth;
  std::cout << " seldepth " << currentMaxDepth;
  std::cout << " nodes " << totalNodes;
  std::cout << " time " << timeDelta.count();
  std::cout << " nps "
            << (timeDelta.count() >= 1000
                    ? (totalNodes * 1000) / timeDelta.count()
                    : 0);

  if (std::abs(entry.value) >= Value::CHECKMATE_THRESHOLD) {
    // Calculate mate distance
    int mateDepth = Value::CHECKMATE - std::abs(entry.value);
    std::cout << " score mate "
              << ((entry.value > 0) - (entry.value < 0)) * (mateDepth + 1) / 2;
  } else {
    std::cout << " score cp " << entry.value;
  }

  if (entry.pv.size > 0) {
    std::cout << " pv";
    for (int i = 0; i < entry.pv.size; i++) {
      std::cout << " " << fromMove(entry.pv.moves[i]);
    }
  }

  std::cout << std::endl;

  statusStartTime = std::chrono::system_clock::now();
}

std::string Pulse::fromMove(int move) {
  std::string notation;

  notation += Notation::fromSquare(Move::getOriginSquare(move));
  notation += Notation::fromSquare(Move::getTargetSquare(move));

  int promotion = Move::getPromotion(move);
  if (promotion != PieceType::NOPIECETYPE) {
    notation += std::tolower(Notation::fromPieceType(promotion));
  }

  return notation;
}

}  // namespace pulse
