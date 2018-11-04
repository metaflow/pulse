/*
 * Copyright (C) 2013-2016 Phokham Nonava
 *
 * Use of this source code is governed by the MIT license that can be
 * found in the LICENSE file.
 */
#ifndef PULSE_MOVEGENERATOR_H
#define PULSE_MOVEGENERATOR_H

#include "position.h"
#include "movelist.h"

namespace pulse {

class MoveGenerator {
public:
	void getLegalMoves(Position& position, int depth, bool isCheck, MoveEntryList& moves);

	void getMoves(Position& position, int depth, bool isCheck, MoveEntryList& moves);

private:

	void addMoves(MoveEntryList& list, Position& position);

	void addMoves(MoveEntryList& list, int originSquare, const std::vector<int>& directions, Position& position);

	void addPawnMoves(MoveEntryList& list, int pawnSquare, Position& position);

	void addCastlingMoves(MoveEntryList& list, int kingSquare, Position& position);
};

}

#endif
