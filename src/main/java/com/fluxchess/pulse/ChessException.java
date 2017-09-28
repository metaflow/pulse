/*
 * Copyright (C) 2013-2017 Phokham Nonava
 *
 * Use of this source code is governed by the MIT license that can be
 * found in the LICENSE file.
 */
package com.fluxchess.pulse;

class ChessException extends RuntimeException {
	ChessException(String message) {
		super(message);
	}

	ChessException(Throwable cause) {
		super(cause);
	}
}
