/*
 * Copyright (C) 2013-2016 Phokham Nonava
 *
 * Use of this source code is governed by the MIT license that can be
 * found in the LICENSE file.
 */

#include "search.h"

#include <iostream>

namespace pulse {

Search::Timer::Timer(bool& timerStopped, bool& doTimeManagement, int& currentDepth, const int& initialDepth,
					 bool& abort)
		: timerStopped(timerStopped), doTimeManagement(doTimeManagement),
		  currentDepth(currentDepth), initialDepth(initialDepth), abort(abort) {
}

void Search::Timer::run(uint64_t searchTime) {
	std::unique_lock<std::mutex> lock(mutex);
	if (condition.wait_for(lock, std::chrono::milliseconds(searchTime)) == std::cv_status::timeout) {
		timerStopped = true;

		// If we finished the first iteration, we should have a result.
		// In this case abort the search.
		if (!doTimeManagement || currentDepth > initialDepth) {
			abort = true;
		}
	}
}

void Search::Timer::start(uint64_t searchTime) {
	thread = std::thread(&Search::Timer::run, this, searchTime);
}

void Search::Timer::stop() {
	condition.notify_all();
	thread.join();
}

Search::Semaphore::Semaphore(int permits)
		: permits(permits) {
}

void Search::Semaphore::acquire() {
	std::unique_lock<std::mutex> lock(mutex);
	while (permits == 0) {
		condition.wait(lock);
	}
	permits--;
}

void Search::Semaphore::release() {
	std::unique_lock<std::mutex> lock(mutex);
	permits++;
	condition.notify_one();
}

void Search::Semaphore::drainPermits() {
	std::unique_lock<std::mutex> lock(mutex);
	permits = 0;
}

void Search::newDepthSearch(Position& position, int searchDepth) {
	if (searchDepth < 1 || searchDepth > Depth::MAX_DEPTH) throw std::exception();
	if (running) throw std::exception();

	reset();

	this->position = position;
	this->searchDepth = searchDepth;
}

void Search::newNodesSearch(Position& position, uint64_t searchNodes) {
	if (searchNodes < 1) throw std::exception();
	if (running) throw std::exception();

	reset();

	this->position = position;
	this->searchNodes = searchNodes;
}

void Search::newTimeSearch(Position& position, uint64_t searchTime) {
	if (searchTime < 1) throw std::exception();
	if (running) throw std::exception();

	reset();

	this->position = position;
	this->searchTime = searchTime;
	this->runTimer = true;
}

void Search::newInfiniteSearch(Position& position) {
	if (running) throw std::exception();

	reset();

	this->position = position;
}

void Search::newClockSearch(Position& position,
							uint64_t whiteTimeLeft, uint64_t whiteTimeIncrement, uint64_t blackTimeLeft,
							uint64_t blackTimeIncrement, int movesToGo) {
	newPonderSearch(position,
			whiteTimeLeft, whiteTimeIncrement, blackTimeLeft, blackTimeIncrement, movesToGo
	);

	this->runTimer = true;
}

void Search::newPonderSearch(Position& position,
							 uint64_t whiteTimeLeft, uint64_t whiteTimeIncrement, uint64_t blackTimeLeft,
							 uint64_t blackTimeIncrement, int movesToGo) {
	if (whiteTimeLeft < 1) throw std::exception();
	if (whiteTimeIncrement < 0) throw std::exception();
	if (blackTimeLeft < 1) throw std::exception();
	if (blackTimeIncrement < 0) throw std::exception();
	if (movesToGo < 0) throw std::exception();
	if (running) throw std::exception();

	reset();

	this->position = position;

	uint64_t timeLeft;
	uint64_t timeIncrement;
	if (position.activeColor == Color::WHITE) {
		timeLeft = whiteTimeLeft;
		timeIncrement = whiteTimeIncrement;
	} else {
		timeLeft = blackTimeLeft;
		timeIncrement = blackTimeIncrement;
	}

	// Don't use all of our time. Search only for 95%. Always leave 1 second as
	// buffer time.
	uint64_t maxSearchTime = (uint64_t) (timeLeft * 0.95) - 1000L;
	if (maxSearchTime < 1) {
		// We don't have enough time left. Search only for 1 millisecond, meaning
		// get a result as fast as we can.
		maxSearchTime = 1;
	}

	// Assume that we still have to do movesToGo number of moves. For every next
	// move (movesToGo - 1) we will receive a time increment.
	this->searchTime = (maxSearchTime + (movesToGo - 1) * timeIncrement) / movesToGo;
	if (this->searchTime > maxSearchTime) {
		this->searchTime = maxSearchTime;
	}

	this->doTimeManagement = true;
}

Search::Search(Protocol& protocol)
		: protocol(protocol),
		  timer(timerStopped, doTimeManagement, currentDepth, initialDepth, abort),
		  wakeupSignal(0), runSignal(0), stopSignal(0) {

	reset();

	thread = std::thread(&Search::run, this);
}

void Search::reset() {
	searchDepth = Depth::MAX_DEPTH;
	searchNodes = std::numeric_limits<uint64_t>::max();
	searchTime = 0;
	runTimer = false;
	timerStopped = false;
	doTimeManagement = false;
	rootMoves.size = 0;
	abort = false;
	totalNodes = 0;
	currentDepth = initialDepth;
	currentMaxDepth = 0;
	currentMove = Move::NOMOVE;
	currentMoveNumber = 0;
}

void Search::start() {
	// std::unique_lock<std::recursive_mutex> lock(sync);

	if (!running) {
		wakeupSignal.release();
		runSignal.acquire();
	}
}

void Search::stop() {
	// std::unique_lock<std::recursive_mutex> lock(sync);

	if (running) {
		// Signal the search thread that we want to stop it
		abort = true;

		stopSignal.acquire();
	}
}

void Search::ponderhit() {
	// std::unique_lock<std::recursive_mutex> lock(sync);

	if (running) {
		// Enable time management
		runTimer = true;
		timer.start(searchTime);

		// If we finished the first iteration, we should have a result.
		// In this case check the stop conditions.
		if (currentDepth > initialDepth) {
			checkStopConditions();
		}
	}
}

void Search::quit() {
	// std::unique_lock<std::recursive_mutex> lock(sync);

	stop();

	shutdown = true;
	wakeupSignal.release();

	thread.join();
}

void Search::run() {
}

void Search::checkStopConditions() {
	// We will check the stop conditions only if we are using time management,
	// that is if our timer != null.
	if (runTimer && doTimeManagement) {
		if (timerStopped) {
			abort = true;
		} else {
			// Check if we have only one move to make
			if (rootMoves.size == 1) {
				abort = true;
			} else

				// Check if we have a checkmate
			if (Value::isCheckmate(rootMoves.entries[0]->value)
				&& currentDepth >= (Value::CHECKMATE - std::abs(rootMoves.entries[0]->value))) {
				abort = true;
			}
		}
	}
}

void Search::updateSearch(int ply) {
	totalNodes++;

	if (ply > currentMaxDepth) {
		currentMaxDepth = ply;
	}

	if (searchNodes <= totalNodes) {
		// Hard stop on number of nodes
		abort = true;
	}

	pv[ply].size = 0;

	protocol.sendStatus(currentDepth, currentMaxDepth, totalNodes, currentMove, currentMoveNumber);
}

void Search::searchRoot(int depth, int alpha, int beta) {
	int ply = 0;

	updateSearch(ply);

	// Abort conditions
	if (abort) {
		return;
	}

	// Reset all values, so the best move is pushed to the front
	for (int i = 0; i < rootMoves.size; i++) {
		rootMoves.entries[i]->value = -Value::INFINITE;
	}

	for (int i = 0; i < rootMoves.size; i++) {
		int move = rootMoves.entries[i]->move;

		currentMove = move;
		currentMoveNumber = i + 1;
		protocol.sendStatus(false, currentDepth, currentMaxDepth, totalNodes, currentMove, currentMoveNumber);

		position.makeMove(move);
		int value = -search(depth - 1, -beta, -alpha, ply + 1);
		position.undoMoveR(move);

		if (abort) {
			return;
		}

		// Do we have a better value?
		if (value > alpha) {
			alpha = value;

			// We found a new best move
			rootMoves.entries[i]->value = value;
			savePV(move, pv[ply + 1], rootMoves.entries[i]->pv);

			protocol.sendMove(*rootMoves.entries[i], currentDepth, currentMaxDepth, totalNodes);
		}
	}

	if (rootMoves.size == 0) {
		// The root position is a checkmate or stalemate. We cannot search
		// further. Abort!
		abort = true;
	}
}

int Search::search(int depth, int alpha, int beta, int ply) {
	return 0;
}

int Search::quiescent(int depth, int alpha, int beta, int ply) {
	return 0;
}

void Search::savePV(int move, MoveVariation& src, MoveVariation& dest) {
	dest.moves[0] = move;
	for (int i = 0; i < src.size; i++) {
		dest.moves[i + 1] = src.moves[i];
	}
	dest.size = src.size + 1;
}

}
