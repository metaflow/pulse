/*
 * Copyright (C) 2013-2016 Phokham Nonava
 *
 * Use of this source code is governed by the MIT license that can be
 * found in the LICENSE file.
 */

#include "position.h"
#include "file.h"
#include "move.h"
#include "rank.h"

#include <algorithm>
#include <iostream>
#include <sstream>

namespace pulse {

// Initialize the zobrist keys
/*
Position::Zobrist::Zobrist() {
for (auto piece : Piece::values) {
  for (int i = 0; i < Square::VALUES_LENGTH; i++) {
    board[piece][i] = next();
  }
}

castlingRights[Castling::WHITE_KINGSIDE] = next();
castlingRights[Castling::WHITE_QUEENSIDE] = next();
castlingRights[Castling::BLACK_KINGSIDE] = next();
castlingRights[Castling::BLACK_QUEENSIDE] = next();
castlingRights[Castling::WHITE_KINGSIDE | Castling::WHITE_QUEENSIDE] =
    castlingRights[Castling::WHITE_KINGSIDE] ^
    castlingRights[Castling::WHITE_QUEENSIDE];
castlingRights[Castling::BLACK_KINGSIDE | Castling::BLACK_QUEENSIDE] =
    castlingRights[Castling::BLACK_KINGSIDE] ^
    castlingRights[Castling::BLACK_QUEENSIDE];

for (int i = 0; i < Square::VALUES_LENGTH; i++) {
  enPassantSquare[i] = next();
}

activeColor = next();
}

Position::Zobrist& Position::Zobrist::instance() {
static Zobrist* instance = new Zobrist();
return *instance;
}

uint64_t Position::Zobrist::next() {
std::array<uint64_t, 16> bytes;
for (int i = 0; i < 16; i++) { bytes[i] = generator(); }

uint64_t hash = 0;
for (int i = 0; i < 16; i++) { hash ^= bytes[i] << ((i * 8) % 64); }

return hash;
}
*/
Position::Position() : promoted(128, false) { board.fill(+Piece::NOPIECE); }

Position::Position(const Position& position) : Position() {
  this->board = position.board;
  this->pieces = position.pieces;

  // this->material = position.material;

  this->castlingRights = position.castlingRights;
  this->enPassantSquare = position.enPassantSquare;
  this->activeColor = position.activeColor;
  this->halfmoveClock = position.halfmoveClock;

  // this->zobristKey = position.zobristKey;

  this->halfmoveNumber = position.halfmoveNumber;

  // this->statesSize = 0;
  this->promoted = position.promoted;
  this->moves = position.moves;
  this->touched = position.touched;
}

Position& Position::operator=(const Position& position) {
  this->board = position.board;
  this->pieces = position.pieces;

  // this->material = position.material;

  this->castlingRights = position.castlingRights;
  this->enPassantSquare = position.enPassantSquare;
  this->activeColor = position.activeColor;
  this->halfmoveClock = position.halfmoveClock;

  // this->zobristKey = position.zobristKey;

  this->halfmoveNumber = position.halfmoveNumber;

  // this->statesSize = 0;
  this->promoted = position.promoted;
  this->moves = position.moves;
  this->touched = position.touched;

  return *this;
}

bool Position::operator==(const Position& position) const {
  return this->board == position.board &&
         this->pieces == position.pieces

         // && this->material == position.material

         && this->castlingRights == position.castlingRights &&
         this->enPassantSquare == position.enPassantSquare &&
         this->activeColor == position.activeColor &&
         this->halfmoveClock == position.halfmoveClock

         // && this->zobristKey == position.zobristKey

         && this->halfmoveNumber == position.halfmoveNumber;
}

bool Position::operator!=(const Position& position) const {
  return !(*this == position);
}

void Position::setActiveColor(int activeColor) {
  if (this->activeColor != activeColor) {
    this->activeColor = activeColor;
    // zobristKey ^= zobrist.activeColor;
  }
}

void Position::setCastlingRight(int castling) {
  if ((castlingRights & castling) == Castling::NOCASTLING) {
    castlingRights |= castling;
    // zobristKey ^= zobrist.castlingRights[castling];
  }
}

void Position::setEnPassantSquare(int enPassantSquare) {
  if (this->enPassantSquare != Square::NOSQUARE) {
    // zobristKey ^= zobrist.enPassantSquare[this->enPassantSquare];
  }
  if (enPassantSquare != Square::NOSQUARE) {
    // zobristKey ^= zobrist.enPassantSquare[enPassantSquare];
  }
  this->enPassantSquare = enPassantSquare;
}

void Position::setHalfmoveClock(int halfmoveClock) {
  this->halfmoveClock = halfmoveClock;
}

int Position::getFullmoveNumber() const { return halfmoveNumber / 2; }

void Position::setFullmoveNumber(int fullmoveNumber) {
  halfmoveNumber = fullmoveNumber * 2;
  if (activeColor == Color::BLACK) { halfmoveNumber++; }
}

bool Position::isRepetition() {
  return false;
  // Search back until the last halfmoveClock reset
  // int j = std::max(0, statesSize - halfmoveClock);
  // for (int i = statesSize - 2; i >= j; i -= 2) {
  // if (zobristKey == states[i].zobristKey) { return true; }
  // }

  return false;
}

bool Position::hasInsufficientMaterial() {
  // If there is only one minor left, we are unable to checkmate
  return Bitboard::size(pieces[Color::WHITE][PieceType::PAWN]) == 0 &&
         Bitboard::size(pieces[Color::BLACK][PieceType::PAWN]) == 0 &&
         Bitboard::size(pieces[Color::WHITE][PieceType::ROOK]) == 0 &&
         Bitboard::size(pieces[Color::BLACK][PieceType::ROOK]) == 0 &&
         Bitboard::size(pieces[Color::WHITE][PieceType::QUEEN]) == 0 &&
         Bitboard::size(pieces[Color::BLACK][PieceType::QUEEN]) == 0 &&
         (Bitboard::size(pieces[Color::WHITE][PieceType::KNIGHT]) +
              Bitboard::size(pieces[Color::WHITE][PieceType::BISHOP]) <=
          1) &&
         (Bitboard::size(pieces[Color::BLACK][PieceType::KNIGHT]) +
              Bitboard::size(pieces[Color::BLACK][PieceType::BISHOP]) <=
          1);
}

/**
 * Puts a piece at the square. We need to update our board and the appropriate
 * piece type list.
 *
 * @param piece  the Piece.
 * @param square the Square.
 */
void Position::put(int piece, int square) {
  int piecetype = Piece::getType(piece);
  int color = Piece::getColor(piece);

  board[square] = piece;
  pieces[color][piecetype] = Bitboard::add(square, pieces[color][piecetype]);
  // material[color] += PieceType::getValue(piecetype);

  // zobristKey ^= zobrist.board[piece][square];
}

void Position::putR(int piece, int square) {
  int piecetype = Piece::getType(piece);
  int color = Piece::getColor(piece);

  board[square] = piece;
  pieces[color][piecetype] = Bitboard::add(square, pieces[color][piecetype]);
  // material[color] += PieceType::getValue(piecetype);
}

/**
 * Removes a piece from the square. We need to update our board and the
 * appropriate piece type list.
 *
 * @param square the Square.
 * @return the Piece which was removed.
 */
int Position::remove(int square) {
  int piece = board[square];

  int piecetype = Piece::getType(piece);
  int color = Piece::getColor(piece);

  board[square] = Piece::NOPIECE;
  pieces[color][piecetype] = Bitboard::remove(square, pieces[color][piecetype]);
  // material[color] -= PieceType::getValue(piecetype);

  // zobristKey ^= zobrist.board[piece][square];

  return piece;
}

int Position::removeR(int square) {
  int piece = board[square];

  int piecetype = Piece::getType(piece);
  int color = Piece::getColor(piece);

  board[square] = Piece::NOPIECE;
  pieces[color][piecetype] = Bitboard::remove(square, pieces[color][piecetype]);
  // material[color] -= PieceType::getValue(piecetype);

  return piece;
}

void Position::makeMove(int move) {
  moves.emplace_back(move);
  // Save state
  // State& entry = states[statesSize];
  // entry.zobristKey = zobristKey;
  // entry.castlingRights = castlingRights;
  // entry.enPassantSquare = enPassantSquare;
  // entry.halfmoveClock = halfmoveClock;

  // statesSize++;

  // Get variables
  int type = Move::getType(move);
  int originSquare = Move::getOriginSquare(move);
  int targetSquare = Move::getTargetSquare(move);
  int originPiece = Move::getOriginPiece(move);
  int originColor = Piece::getColor(originPiece);
  int targetPiece = Move::getTargetPiece(move);
  if (Piece::getColor(originPiece) == Color::WHITE) {
    if (originPiece == Piece::WHITE_PAWN && originSquare / 16 == 1) {
      touched[originPiece]++;
    } else if (originSquare / 16 == 0) {
      touched[originPiece]++;
    }
  }

  // Remove target piece and update castling rights
  if (targetPiece != Piece::NOPIECE) {
    int captureSquare = targetSquare;
    if (type == MoveType::ENPASSANT) {
      captureSquare += (originColor == Color::WHITE ? Square::S : Square::N);
    }
    remove(captureSquare);

    clearCastling(captureSquare);
  }

  // Move piece
  remove(originSquare);
  if (type == MoveType::PAWNPROMOTION) {
    put(Piece::valueOf(originColor, Move::getPromotion(move)), targetSquare);
    promoted[targetSquare] = true;
  } else {
    put(originPiece, targetSquare);
    promoted[targetSquare] = promoted[originSquare];
    promoted[originSquare] = false;
  }

  // Move rook and update castling rights
  if (type == MoveType::CASTLING) {
    int rookOriginSquare;
    int rookTargetSquare;
    switch (targetSquare) {
      case Square::g1:
        rookOriginSquare = Square::h1;
        rookTargetSquare = Square::f1;
        break;
      case Square::c1:
        rookOriginSquare = Square::a1;
        rookTargetSquare = Square::d1;
        break;
      case Square::g8:
        rookOriginSquare = Square::h8;
        rookTargetSquare = Square::f8;
        break;
      case Square::c8:
        rookOriginSquare = Square::a8;
        rookTargetSquare = Square::d8;
        break;
      default:
        std::cerr << "ERROR " << __FILE__ << ' ' << __LINE__ << std::endl;
        throw std::exception();
    }

    int rookPiece = remove(rookOriginSquare);
    put(rookPiece, rookTargetSquare);
  }

  // Update castling
  clearCastling(originSquare);

  // Update enPassantSquare
  if (enPassantSquare != Square::NOSQUARE) {
    // zobristKey ^= zobrist.enPassantSquare[enPassantSquare];
  }
  if (type == MoveType::PAWNDOUBLE) {
    enPassantSquare =
        targetSquare + (originColor == Color::WHITE ? Square::S : Square::N);
    // zobristKey ^= zobrist.enPassantSquare[enPassantSquare];
  } else {
    enPassantSquare = Square::NOSQUARE;
  }

  // Update activeColor
  activeColor = Color::opposite(activeColor);
  // zobristKey ^= zobrist.activeColor;

  // Update halfmoveClock
  if (Piece::getType(originPiece) == PieceType::PAWN ||
      targetPiece != Piece::NOPIECE) {
    halfmoveClock = 0;
  } else {
    halfmoveClock++;
  }

  // Update fullMoveNumber
  halfmoveNumber++;
}

uint64_t Position::hash() {
  uint64_t fen = 0;

  // Pieces
  for (auto iter = Rank::values.rbegin(); iter != Rank::values.rend(); iter++) {
    int          rank = *iter;
    unsigned int emptySquares = 0;

    for (auto file : File::values) {
      int piece = board[Square::valueOf(file, rank)];

      if (piece == Piece::NOPIECE) {
        emptySquares++;
      } else {
        if (emptySquares > 0) {
          fen = fen * 13 + emptySquares;
          emptySquares = 0;
        }
        fen = fen * 13 + piece;
      }
    }

    fen = fen * 13 + emptySquares;
  }

  // Color

  fen = fen * 13 + activeColor;
  fen = fen * 13 + enPassantSquare;
  fen = fen * 13 + halfmoveNumber;

  return fen;
}

void Position::makeMoveR(int move) {
  // Get variables
  int type = Move::getType(move);
  int originSquare = Move::getOriginSquare(move);
  int targetSquare = Move::getTargetSquare(move);
  int originPiece = Move::getOriginPiece(move);
  int originColor = Piece::getColor(originPiece);
  int targetPiece = Move::getTargetPiece(move);

  // Remove target piece and update castling rights
  if (targetPiece != Piece::NOPIECE) {
    int captureSquare = targetSquare;
    if (type == MoveType::ENPASSANT) {
      captureSquare += (originColor == Color::WHITE ? Square::S : Square::N);
    }
    removeR(captureSquare);
  }

  // Move piece
  removeR(originSquare);
  if (type == MoveType::PAWNPROMOTION) {
    putR(Piece::valueOf(originColor, Move::getPromotion(move)), targetSquare);
  } else {
    putR(originPiece, targetSquare);
  }

  // Move rook and update castling rights
  /*
  if (type == MoveType::CASTLING) {
    int rookOriginSquare;
    int rookTargetSquare;
    switch (targetSquare) {
      case Square::g1:
        rookOriginSquare = Square::h1;
        rookTargetSquare = Square::f1;
        break;
      case Square::c1:
        rookOriginSquare = Square::a1;
        rookTargetSquare = Square::d1;
        break;
      case Square::g8:
        rookOriginSquare = Square::h8;
        rookTargetSquare = Square::f8;
        break;
      case Square::c8:
        rookOriginSquare = Square::a8;
        rookTargetSquare = Square::d8;
        break;
      default: throw std::exception();
    }

    int rookPiece = remove(rookOriginSquare);
    putR(rookPiece, rookTargetSquare);
  }
  */
  // Update activeColor
  activeColor = Color::opposite(activeColor);
}

void Position::undoMoveR(int move) {
  // Get variables
  int type = Move::getType(move);
  int originSquare = Move::getOriginSquare(move);
  int targetSquare = Move::getTargetSquare(move);
  int originPiece = Move::getOriginPiece(move);
  int originColor = Piece::getColor(originPiece);
  int targetPiece = Move::getTargetPiece(move);

  // Update activeColor
  activeColor = Color::opposite(activeColor);

  // Undo move rook
  /*
  if (type == MoveType::CASTLING) {
    int rookOriginSquare;
    int rookTargetSquare;
    switch (targetSquare) {
      case Square::g1:
        rookOriginSquare = Square::h1;
        rookTargetSquare = Square::f1;
        break;
      case Square::c1:
        rookOriginSquare = Square::a1;
        rookTargetSquare = Square::d1;
        break;
      case Square::g8:
        rookOriginSquare = Square::h8;
        rookTargetSquare = Square::f8;
        break;
      case Square::c8:
        rookOriginSquare = Square::a8;
        rookTargetSquare = Square::d8;
        break;
      default: throw std::exception();
    }

    int rookPiece = remove(rookTargetSquare);
    put(rookPiece, rookOriginSquare);
  }
  */

  // Undo move piece
  removeR(targetSquare);
  putR(originPiece, originSquare);

  // Restore target piece
  if (targetPiece != Piece::NOPIECE) {
    int captureSquare = targetSquare;
    if (type == MoveType::ENPASSANT) {
      captureSquare += (originColor == Color::WHITE ? Square::S : Square::N);
    }
    putR(targetPiece, captureSquare);
  }
}
/*
void Position::undoMove(int move) {
moves.pop_back();
// Get variables
int type = Move::getType(move);
int originSquare = Move::getOriginSquare(move);
int targetSquare = Move::getTargetSquare(move);
int originPiece = Move::getOriginPiece(move);
int originColor = Piece::getColor(originPiece);
int targetPiece = Move::getTargetPiece(move);

// Update fullMoveNumber
halfmoveNumber--;

// Update activeColor
activeColor = Color::opposite(activeColor);

// Undo move rook
if (type == MoveType::CASTLING) {
  int rookOriginSquare;
  int rookTargetSquare;
  switch (targetSquare) {
    case Square::g1:
      rookOriginSquare = Square::h1;
      rookTargetSquare = Square::f1;
      break;
    case Square::c1:
      rookOriginSquare = Square::a1;
      rookTargetSquare = Square::d1;
      break;
    case Square::g8:
      rookOriginSquare = Square::h8;
      rookTargetSquare = Square::f8;
      break;
    case Square::c8:
      rookOriginSquare = Square::a8;
      rookTargetSquare = Square::d8;
      break;
    default: throw std::exception();
  }

  int rookPiece = remove(rookTargetSquare);
  put(rookPiece, rookOriginSquare);
}

if (type == MoveType::PAWNPROMOTION) {
  promoted[targetSquare] = false;
} else {
  promoted[originSquare] = promoted[targetSquare];
  promoted[targetSquare] = false;
}

// Undo move piece
remove(targetSquare);
put(originPiece, originSquare);

// Restore target piece
if (targetPiece != Piece::NOPIECE) {
  int captureSquare = targetSquare;
  if (type == MoveType::ENPASSANT) {
    captureSquare += (originColor == Color::WHITE ? Square::S : Square::N);
  }
  put(targetPiece, captureSquare);
}

// Restore state
statesSize--;

State& entry = states[statesSize];
halfmoveClock = entry.halfmoveClock;
enPassantSquare = entry.enPassantSquare;
castlingRights = entry.castlingRights;
zobristKey = entry.zobristKey;
}
*/

void Position::clearCastling(int square) {
  int newCastlingRights = castlingRights;

  switch (square) {
    case Square::a1: newCastlingRights &= ~Castling::WHITE_QUEENSIDE; break;
    case Square::a8: newCastlingRights &= ~Castling::BLACK_QUEENSIDE; break;
    case Square::h1: newCastlingRights &= ~Castling::WHITE_KINGSIDE; break;
    case Square::h8: newCastlingRights &= ~Castling::BLACK_KINGSIDE; break;
    case Square::e1:
      newCastlingRights &=
          ~(Castling::WHITE_KINGSIDE | Castling::WHITE_QUEENSIDE);
      break;
    case Square::e8:
      newCastlingRights &=
          ~(Castling::BLACK_KINGSIDE | Castling::BLACK_QUEENSIDE);
      break;
    default: return;
  }

  if (newCastlingRights != castlingRights) {
    castlingRights = newCastlingRights;
    // zobristKey ^= zobrist.castlingRights[newCastlingRights ^ castlingRights];
  }
}

bool Position::isCheck() {
  // Check whether our king is attacked by any opponent piece
  return isAttacked(Bitboard::next(pieces[activeColor][PieceType::KING]),
                    Color::opposite(activeColor));
}

bool Position::isCheck(int color) {
  // Check whether the king for color is attacked by any opponent piece
  return isAttacked(Bitboard::next(pieces[color][PieceType::KING]),
                    Color::opposite(color));
}

bool Position::isCheckR(int color) {
  // Check whether the king for color is attacked by any opponent piece after
  // own move.
  return isAttackedR(Bitboard::next(pieces[color][PieceType::KING]),
                     Color::opposite(color));
}

/**
 * Returns whether the targetSquare is attacked by any piece from the
 * attackerColor. We will backtrack from the targetSquare to find the piece.
 *
 * @param targetSquare  the target Square.
 * @param attackerColor the attacker Color.
 * @return whether the targetSquare is attacked.
 */
bool Position::isAttacked(int targetSquare, int attackerColor) {
  // Pawn attacks
  int pawnPiece = Piece::valueOf(attackerColor, PieceType::PAWN);
  for (unsigned int i = 1; i < Square::pawnDirections[attackerColor].size();
       i++) {
    int attackerSquare =
        targetSquare - Square::pawnDirections[attackerColor][i];
    if (Square::isValid(attackerSquare)) {
      int attackerPawn = board[attackerSquare];

      if (attackerPawn == pawnPiece) { return true; }
    }
  }

  return isAttacked(targetSquare,
                    Piece::valueOf(attackerColor, PieceType::KNIGHT),
                    Square::knightDirections)

         // The queen moves like a bishop, so check both piece types
         || isAttacked(targetSquare,
                       Piece::valueOf(attackerColor, PieceType::BISHOP),
                       Piece::valueOf(attackerColor, PieceType::QUEEN),
                       Square::bishopDirections)

         // The queen moves like a rook, so check both piece types
         || isAttacked(targetSquare,
                       Piece::valueOf(attackerColor, PieceType::ROOK),
                       Piece::valueOf(attackerColor, PieceType::QUEEN),
                       Square::rookDirections)

         || isAttacked(targetSquare,
                       Piece::valueOf(attackerColor, PieceType::KING),
                       Square::kingDirections);
}

bool Position::isAttackedR(int targetSquare, int attackerColor) {
  return
      // The queen moves like a bishop, so check both piece types
      isAttackedBishop(targetSquare,
                       Piece::valueOf(attackerColor, PieceType::BISHOP),
                       Piece::valueOf(attackerColor, PieceType::QUEEN))
      // The queen moves like a rook, so check both piece types
      || isAttackedRook(targetSquare,
                        Piece::valueOf(attackerColor, PieceType::ROOK),
                        Piece::valueOf(attackerColor, PieceType::QUEEN));
}

/**
 * Returns whether the targetSquare is attacked by a non-sliding piece.
 */
bool Position::isAttacked(int targetSquare, int attackerPiece,
                          const std::vector<int>& directions) {
  for (auto direction : directions) {
    int attackerSquare = targetSquare + direction;

    if (Square::isValid(attackerSquare) &&
        board[attackerSquare] == attackerPiece) {
      return true;
    }
  }

  return false;
}

/**
 * Returns whether the targetSquare is attacked by a sliding piece.
 */
bool Position::isAttacked(int targetSquare, int attackerPiece, int queenPiece,
                          const std::vector<int>& directions) {
  for (auto direction : directions) {
    int attackerSquare = targetSquare + direction;

    while (Square::isValid(attackerSquare)) {
      int piece = board[attackerSquare];
      if (Piece::isValid(piece)) {
        if (piece == attackerPiece || piece == queenPiece) { return true; }

        break;
      } else {
        attackerSquare += direction;
      }
    }
  }

  return false;
}

bool Position::isAttackedRook(int targetSquare, int attackerPiece,
                              int queenPiece) {
  for (auto direction : Square::rookDirections) {
    int attackerSquare = targetSquare + direction;

    while (Square::isValid(attackerSquare)) {
      int piece = board[attackerSquare];
      if (Piece::isValid(piece)) {
        if (piece == attackerPiece || piece == queenPiece) { return true; }

        break;
      } else {
        attackerSquare += direction;
      }
    }
  }

  return false;
}

bool Position::isAttackedBishop(int targetSquare, int attackerPiece,
                                int queenPiece) {
  for (auto direction : Square::bishopDirections) {
    int attackerSquare = targetSquare + direction;

    while (Square::isValid(attackerSquare)) {
      int piece = board[attackerSquare];
      if (Piece::isValid(piece)) {
        if (piece == attackerPiece || piece == queenPiece) { return true; }

        break;
      } else {
        attackerSquare += direction;
      }
    }
  }

  return false;
}
bool Position::isPromoted(int square) { return this->promoted[square]; }

}  // namespace pulse
