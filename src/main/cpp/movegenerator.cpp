/*
 * Copyright (C) 2013-2016 Phokham Nonava
 *
 * Use of this source code is governed by the MIT license that can be
 * found in the LICENSE file.
 */

#include "movegenerator.h"
#include "rank.h"

namespace pulse {

  void MoveGenerator::getLegalMoves(Position& position, int depth, bool isCheck, MoveEntryList& legalMoves) {
  getMoves(position, depth, isCheck, legalMoves);

	int size = legalMoves.size;
	legalMoves.size = 0;
	for (int i = 0; i < size; i++) {
		int move = legalMoves.entries[i].move;

		position.makeMoveR(move);
    if (Piece::getType(Move::getOriginPiece(move)) == PieceType::KING) {
      if (!position.isCheck(Color::opposite(position.activeColor))) {
        legalMoves.entries[legalMoves.size++].move = move;
		}
    } else {
		if (!position.isCheckR(Color::opposite(position.activeColor))) {
			legalMoves.entries[legalMoves.size++].move = move;
		}
    }
		position.undoMoveR(move);
	}
}

void MoveGenerator::getMoves(Position& position, int depth, bool isCheck, MoveEntryList& moves) {
	moves.size = 0;

	if (depth > 0) {
		// Generate main moves

		addMoves(moves, position);

		// if (!isCheck) {
			// int square = Bitboard::next(position.pieces[position.activeColor][PieceType::KING]);
			// addCastlingMoves(moves, square, position);
		// }
	} else {
		// Generate quiescent moves

		addMoves(moves, position);

		if (!isCheck) {
			int size = moves.size;
			moves.size = 0;
			for (int i = 0; i < size; i++) {
				if (Move::getTargetPiece(moves.entries[i].move) != Piece::NOPIECE) {
					// Add only capturing moves
					moves.entries[moves.size++].move = moves.entries[i].move;
				}
			}
		}
	}

	// moves.rateFromMVVLVA();
	// moves.sort();
}

void MoveGenerator::addMoves(MoveEntryList& list, Position& position) {
	int activeColor = position.activeColor;

	for (auto squares = position.pieces[activeColor][PieceType::PAWN];
		 squares != 0; squares = Bitboard::remainder(squares)) {
		int square = Bitboard::next(squares);
		addPawnMoves(list, square, position);
	}
	for (auto squares = position.pieces[activeColor][PieceType::KNIGHT];
		 squares != 0; squares = Bitboard::remainder(squares)) {
		int square = Bitboard::next(squares);
		addMoves(list, square, Square::knightDirections, position);
	}
	for (auto squares = position.pieces[activeColor][PieceType::BISHOP];
		 squares != 0; squares = Bitboard::remainder(squares)) {
		int square = Bitboard::next(squares);
		addMoves(list, square, Square::bishopDirections, position);
	}
	for (auto squares = position.pieces[activeColor][PieceType::ROOK];
		 squares != 0; squares = Bitboard::remainder(squares)) {
		int square = Bitboard::next(squares);
		addMoves(list, square, Square::rookDirections, position);
	}
	for (auto squares = position.pieces[activeColor][PieceType::QUEEN];
		 squares != 0; squares = Bitboard::remainder(squares)) {
		int square = Bitboard::next(squares);
		addMoves(list, square, Square::queenDirections, position);
	}
	int square = Bitboard::next(position.pieces[activeColor][PieceType::KING]);
	addMoves(list, square, Square::kingDirections, position);
}

void MoveGenerator::addMoves(MoveEntryList& list, int originSquare, const std::vector<int>& directions,
							 Position& position) {
	int originPiece = position.board[originSquare];
	bool sliding = PieceType::isSliding(Piece::getType(originPiece));
	int oppositeColor = Color::opposite(Piece::getColor(originPiece));

	// Go through all move directions for this piece
	for (auto direction : directions) {
		int targetSquare = originSquare + direction;

		// Check if we're still on the board
		while (Square::isValid(targetSquare)) {
			int targetPiece = position.board[targetSquare];

			if (targetPiece == Piece::NOPIECE) {
				// quiet move
				list.entries[list.size++].move = Move::valueOf(
						MoveType::NORMAL, originSquare, targetSquare, originPiece, Piece::NOPIECE,
						PieceType::NOPIECETYPE);

				if (!sliding) {
					break;
				}

				targetSquare += direction;
			} else {
				if (Piece::getColor(targetPiece) == oppositeColor) {
					// capturing move
					list.entries[list.size++].move = Move::valueOf(
							MoveType::NORMAL, originSquare, targetSquare, originPiece, targetPiece,
							PieceType::NOPIECETYPE);
				}

				break;
			}
		}
	}
}

void MoveGenerator::addPawnMoves(MoveEntryList& list, int pawnSquare, Position& position) {
	int pawnPiece = position.board[pawnSquare];
	int pawnColor = Piece::getColor(pawnPiece);

	// Generate only capturing moves first (i = 1)
	for (unsigned int i = 1; i < Square::pawnDirections[pawnColor].size(); i++) {
		int direction = Square::pawnDirections[pawnColor][i];

		int targetSquare = pawnSquare + direction;
		if (Square::isValid(targetSquare)) {
			int targetPiece = position.board[targetSquare];

			if (targetPiece != Piece::NOPIECE) {
				if (Piece::getColor(targetPiece) == Color::opposite(pawnColor)) {
					// Capturing move

					if ((pawnColor == Color::WHITE && Square::getRank(targetSquare) == Rank::r8)
						|| (pawnColor == Color::BLACK && Square::getRank(targetSquare) == Rank::r1)) {
						// Pawn promotion capturing move

						list.entries[list.size++].move = Move::valueOf(
								MoveType::PAWNPROMOTION, pawnSquare, targetSquare, pawnPiece, targetPiece,
								PieceType::QUEEN);
						list.entries[list.size++].move = Move::valueOf(
								MoveType::PAWNPROMOTION, pawnSquare, targetSquare, pawnPiece, targetPiece,
								PieceType::ROOK);
						list.entries[list.size++].move = Move::valueOf(
								MoveType::PAWNPROMOTION, pawnSquare, targetSquare, pawnPiece, targetPiece,
								PieceType::BISHOP);
						list.entries[list.size++].move = Move::valueOf(
								MoveType::PAWNPROMOTION, pawnSquare, targetSquare, pawnPiece, targetPiece,
								PieceType::KNIGHT);
					} else {
						// Normal capturing move

						list.entries[list.size++].move = Move::valueOf(
								MoveType::NORMAL, pawnSquare, targetSquare, pawnPiece, targetPiece,
								PieceType::NOPIECETYPE);
					}
				}
			} else if (targetSquare == position.enPassantSquare) {
				// En passant move
				int captureSquare = targetSquare + (pawnColor == Color::WHITE ? Square::S : Square::N);
				targetPiece = position.board[captureSquare];

				list.entries[list.size++].move = Move::valueOf(
						MoveType::ENPASSANT, pawnSquare, targetSquare, pawnPiece, targetPiece, PieceType::NOPIECETYPE);
			}
		}
	}

	// Generate non-capturing moves
	int direction = Square::pawnDirections[pawnColor][0];

	// Move one rank forward
	int targetSquare = pawnSquare + direction;
	if (Square::isValid(targetSquare) && position.board[targetSquare] == Piece::NOPIECE) {
		if ((pawnColor == Color::WHITE && Square::getRank(targetSquare) == Rank::r8)
			|| (pawnColor == Color::BLACK && Square::getRank(targetSquare) == Rank::r1)) {
			// Pawn promotion move

			list.entries[list.size++].move = Move::valueOf(
					MoveType::PAWNPROMOTION, pawnSquare, targetSquare, pawnPiece, Piece::NOPIECE, PieceType::QUEEN);
			list.entries[list.size++].move = Move::valueOf(
					MoveType::PAWNPROMOTION, pawnSquare, targetSquare, pawnPiece, Piece::NOPIECE, PieceType::ROOK);
			list.entries[list.size++].move = Move::valueOf(
					MoveType::PAWNPROMOTION, pawnSquare, targetSquare, pawnPiece, Piece::NOPIECE, PieceType::BISHOP);
			list.entries[list.size++].move = Move::valueOf(
					MoveType::PAWNPROMOTION, pawnSquare, targetSquare, pawnPiece, Piece::NOPIECE, PieceType::KNIGHT);
		} else {
			// Normal move

			list.entries[list.size++].move = Move::valueOf(
					MoveType::NORMAL, pawnSquare, targetSquare, pawnPiece, Piece::NOPIECE, PieceType::NOPIECETYPE);

			// Move another rank forward
			targetSquare += direction;
			if (Square::isValid(targetSquare) && position.board[targetSquare] == Piece::NOPIECE) {
				if ((pawnColor == Color::WHITE && Square::getRank(targetSquare) == Rank::r4)
					|| (pawnColor == Color::BLACK && Square::getRank(targetSquare) == Rank::r5)) {
					// Pawn double move

					list.entries[list.size++].move = Move::valueOf(
							MoveType::PAWNDOUBLE, pawnSquare, targetSquare, pawnPiece, Piece::NOPIECE,
							PieceType::NOPIECETYPE);
				}
			}
		}
	}
}

void MoveGenerator::addCastlingMoves(MoveEntryList& list, int kingSquare, Position& position) {
	int kingPiece = position.board[kingSquare];

	if (Piece::getColor(kingPiece) == Color::WHITE) {
		// Do not test g1 whether it is attacked as we will test it in isLegal()
		if ((position.castlingRights & Castling::WHITE_KINGSIDE) != Castling::NOCASTLING
			&& position.board[Square::f1] == Piece::NOPIECE
			&& position.board[Square::g1] == Piece::NOPIECE
			&& !position.isAttacked(Square::f1, Color::BLACK)) {
			list.entries[list.size++].move = Move::valueOf(
					MoveType::CASTLING, kingSquare, Square::g1, kingPiece, Piece::NOPIECE, PieceType::NOPIECETYPE);
		}
		// Do not test c1 whether it is attacked as we will test it in isLegal()
		if ((position.castlingRights & Castling::WHITE_QUEENSIDE) != Castling::NOCASTLING
			&& position.board[Square::b1] == Piece::NOPIECE
			&& position.board[Square::c1] == Piece::NOPIECE
			&& position.board[Square::d1] == Piece::NOPIECE
			&& !position.isAttacked(Square::d1, Color::BLACK)) {
			list.entries[list.size++].move = Move::valueOf(
					MoveType::CASTLING, kingSquare, Square::c1, kingPiece, Piece::NOPIECE, PieceType::NOPIECETYPE);
		}
	} else {
		// Do not test g8 whether it is attacked as we will test it in isLegal()
		if ((position.castlingRights & Castling::BLACK_KINGSIDE) != Castling::NOCASTLING
			&& position.board[Square::f8] == Piece::NOPIECE
			&& position.board[Square::g8] == Piece::NOPIECE
			&& !position.isAttacked(Square::f8, Color::WHITE)) {
			list.entries[list.size++].move = Move::valueOf(
					MoveType::CASTLING, kingSquare, Square::g8, kingPiece, Piece::NOPIECE, PieceType::NOPIECETYPE);
		}
		// Do not test c8 whether it is attacked as we will test it in isLegal()
		if ((position.castlingRights & Castling::BLACK_QUEENSIDE) != Castling::NOCASTLING
			&& position.board[Square::b8] == Piece::NOPIECE
			&& position.board[Square::c8] == Piece::NOPIECE
			&& position.board[Square::d8] == Piece::NOPIECE
			&& !position.isAttacked(Square::d8, Color::WHITE)) {
			list.entries[list.size++].move = Move::valueOf(
					MoveType::CASTLING, kingSquare, Square::c8, kingPiece, Piece::NOPIECE, PieceType::NOPIECETYPE);
		}
	}
}

}
