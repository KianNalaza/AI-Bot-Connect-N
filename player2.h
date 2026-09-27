#ifndef PLAYER2_H_INCLUDED
#define PLAYER2_H_INCLUDED

#include "Player.h"

class SolverPlayer : public Player
{
public:
    SolverPlayer(const TokenColor & color, const GameBoard & game_board)
        : Player(color, game_board)
    {
    }

    void gameReset() override
    {
    }

    uint8_t getNextMove() override
    {
        // 1) Win immediately if possible
        uint8_t winMove = findWinningMove(myColor);
        if (winMove != MOVE_NOT_FOUND)
            return winMove;

        // 2) Block opponent immediate win
        uint8_t blockMove = findWinningMove(opponentColor);
        if (blockMove != MOVE_NOT_FOUND)
            return blockMove;

        // 3) If opponent just played somewhere legal, mirror that column
        uint8_t oppCol = gameBoard.prevMove();
        if (oppCol < gameBoard.cols() && !gameBoard.isColumnFull(oppCol))
            return oppCol;

        // 4) Otherwise play centre-first
        return centreFirstMove();
    }

private:
    uint8_t findWinningMove(TokenColor color) const
    {
        for (uint8_t c = 0; c < gameBoard.cols(); c++)
        {
            if (gameBoard.isColumnFull(c))
                continue;

            GameBoard testBoard = gameBoard;
            PutMoveResult r = testBoard.putMove(c, color);

            if (r == PutMoveResult::VALID_MOVE ||
                r == PutMoveResult::OUT_OF_SEQUENCE)
            {
                if (testBoard.isWinner(color))
                    return c;
            }
        }

        return MOVE_NOT_FOUND;
    }

    uint8_t centreFirstMove() const
    {
        uint8_t cols = gameBoard.cols();
        if (cols == 0)
            return MOVE_NOT_FOUND;

        int center = cols / 2;

        if (!gameBoard.isColumnFull((uint8_t)center))
            return (uint8_t)center;

        for (int d = 1; d < cols; d++)
        {
            int left = center - d;
            int right = center + d;

            if (left >= 0 && !gameBoard.isColumnFull((uint8_t)left))
                return (uint8_t)left;

            if (right < cols && !gameBoard.isColumnFull((uint8_t)right))
                return (uint8_t)right;
        }

        return MOVE_NOT_FOUND;
    }
};

#endif