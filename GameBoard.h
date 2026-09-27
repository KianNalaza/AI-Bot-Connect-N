// EEN1092 - AI Bot Project
// C.McArdle, DCU, 2026
// PC and/or Arduino code
//
// Connect4 GameBoard Class
//
// Stores current location of tokens in board and provides logic for placing a player's token
// that is 'dropped' into a given column and for checking if the board state is a win or draw
// Provides additional methods for checking the current occupancy of the game board
//
// Simple usage example:
//
//   GameBoard gameBoard(10,10,5);                       // Make a 10x10 board. 5-in-line to win
//   gameBoard.putMove(0,TokenColor::RED);               // RED plays into column 0
//   gameBoard.putMove(3,TokenColor::YELLOW);            // YELLOW plays into column 3
//   gameBoard.putMove(3,TokenColor::RED);               // RED plays into column 3
//   cout << gameBoard.columnCount(3) << endl;           // prints 2 (2 tokens in column 3)
//   cout << gameBoard.isColumnFull(3) << endl;          // prints 0 (false, column not yet full)
//   cout << (gameBoard.result() == RED_WINS) << endl;   // prints 0 (false, RED hasn't won)
//   ...
//

#ifndef GAMEBOARD_H_INCLUDED
#define GAMEBOARD_H_INCLUDED
#include <cstdint>
#include <vector>

//
// Color of a player's playing pieces (tokens)
//
enum class TokenColor
{
    RED,
    YELLOW,
    NOT_SET
};

//
// Result status of game. Game is NOT_DECIDED if it is still in progress, not won or drawn as yet
//
enum class GameResult
{
    YELLOW_WINS,
    RED_WINS,
    DRAW_GAME,
    NOT_DECIDED
};

//
// Return/error codes GameBoard::putMove() method can return, in reponse to valid/invalid moves
//
enum class PutMoveResult
{
    VALID_MOVE,           // move is valid
    OUT_OF_SEQUENCE,      // move accepted but is out of sequence (same color was played in last move)
    COLUMN_OUT_OF_RANGE,  // move column number is out of range
    COLUMN_FULL,          // move attempted into full column
    BOARD_FULL,           // move attempted when board is already full
    GAME_ENDED            // move attempted after game ended (end of game but not full board)
};

class GameBoard
{
private:

    static const uint8_t VACANT_CELL;                  // the 3 possible occupancy states of a playing board cell
    static const uint8_t RED_CELL;                     // a cell is vacant or is occupied by a RED or a YELLOW token
    static const uint8_t YELLOW_CELL;

    uint8_t ROWS;                                      // number of gameboard rows
    uint8_t COLS;                                      // number of gameboard columns
    uint8_t NUM_TO_CONNECT;                            // number in-a-line needed to win

    std::vector<std::vector<uint8_t>> boardOccupancy;  // stores which board cells are occupied by which player's tokens
    std::vector<uint8_t> nextFreeRow;                  // next free row in each column, numbered bottom to top 0,1,2,...
    uint16_t numTokensPlayed;                          // total number of pieces that have been added to the board (from both players)

    uint8_t prev_move_row;                             // row number of the previous move that was made
    uint8_t prev_move_col;                             // column number of the previous move that was made
    TokenColor prev_move_color;                        // color of the previous move

    GameResult gameResult;                             // result of current game board state
    std::vector<uint8_t> winningLine;                  // (r1,c1) and (r2,c2) board points defining the winning line on board

public:

    //////////////////////////////////////////////////////////////////////////
    // Class constructor
    // Set the number of rows and columns the board has
    // Set the game objective, number in a line to connect to win
    //////////////////////////////////////////////////////////////////////////
    GameBoard(uint8_t numRows,
              uint8_t numCols,
              uint8_t num_to_connect);

    //////////////////////////////////////////////////////////////////////////
    // Class destructor
    //////////////////////////////////////////////////////////////////////////
    virtual ~GameBoard();

    //////////////////////////////////////////////////////////////////////////
    // Reset the board, removing all pieces
    //////////////////////////////////////////////////////////////////////////
    virtual void reset();

    //////////////////////////////////////////////////////////////////////////
    // Reset the board, removing all pieces and reconfiguring board
    //////////////////////////////////////////////////////////////////////////
    virtual void reset(uint8_t numRows, uint8_t numCols, uint8_t num_to_connect);

    //////////////////////////////////////////////////////////////////////////
    // Get the number of rows the game board has
    //////////////////////////////////////////////////////////////////////////
    uint8_t rows() const;

    //////////////////////////////////////////////////////////////////////////
    // Get the number of columns the game board has
    //////////////////////////////////////////////////////////////////////////
    uint8_t cols() const;

    //////////////////////////////////////////////////////////////////////////
    // Current total number of tokens that have been played on board
    //////////////////////////////////////////////////////////////////////////
    uint16_t tokensCount() const;

    //////////////////////////////////////////////////////////////////////////
    // Returns last move made (the column number token was played into)
    //////////////////////////////////////////////////////////////////////////
    uint8_t prevMove() const;

    //////////////////////////////////////////////////////////////////////////
    // Returns colour of the last move that was made
    //////////////////////////////////////////////////////////////////////////
    TokenColor prevColor() const;

    //////////////////////////////////////////////////////////////////////////
    // Return number of tokens in a given column
    // Columns are numbered 0,1,2... left to right on the board
    // Returns 0 if colNum is out of range
    //////////////////////////////////////////////////////////////////////////
    int tokensInColumn(const uint8_t colNum) const;

    //////////////////////////////////////////////////////////////////////////
    // Check if a given column is full
    // Returns true if full or if colNum is out of range,
    // Returns false otherwise
    //////////////////////////////////////////////////////////////////////////
    bool isColumnFull(const uint8_t colNum) const;

    //////////////////////////////////////////////////////////////////////////
    // Find all non-full columns (all possible next moves)
    // Columns are number 0,1,2... left to right on the board
    // If all columns full, returns empty (size 0) vector
    //////////////////////////////////////////////////////////////////////////
    std::vector<uint8_t> availableColumns() const;

    //////////////////////////////////////////////////////////////////////////
    // Get current state of game (win, draw, not yet decided)
    // The state of the game is updated automatially by
    // putMove() each a new (valid) move is added to board
    //////////////////////////////////////////////////////////////////////////
    GameResult result() const;

    //////////////////////////////////////////////////////////////////////////
    // Check if game is over (won or drawn)
    //////////////////////////////////////////////////////////////////////////
    bool isGameOver() const;

    //////////////////////////////////////////////////////////////////////////
    // Check if given color has won
    //////////////////////////////////////////////////////////////////////////
    bool isWinner(TokenColor color) const;

    //////////////////////////////////////////////////////////////////////////
    // Total number of bytes used to store the GameBoard
    // object. This is only an estimate as std::vector may
    // pre-allocate additional memory, beyond its current size
    //////////////////////////////////////////////////////////////////////////
    size_t memSize() const;

    //////////////////////////////////////////////////////////////////////////
    // Add a token to the board, in given column, if move
    // is valid. Return value indicates validity of move.
    // Checks game result (won,drawn) after token is added
    // and updates gameResult variable accordingly.
    //////////////////////////////////////////////////////////////////////////
    virtual PutMoveResult putMove(const uint8_t column,
                                  const TokenColor & token_color);

protected:

    //////////////////////////////////////////////////////////////////////////
    // Return (r1,c1) and (r2,c2) of winning line on board
    // Utility method used for drawing the winning line
    //////////////////////////////////////////////////////////////////////////
    std::vector<uint8_t> getWinningLine() const;

private:

    //////////////////////////////////////////////////////////////////////////
    // Update result of game according to current board
    // state/occupancy. Checks win (or draw) for the given
    // player color. This method is called only by putMove(),
    // after each new move is added to the board
    //////////////////////////////////////////////////////////////////////////
    virtual void updateResult(const TokenColor color);

};

#endif
