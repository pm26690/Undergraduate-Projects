import java.util.Scanner;
import java.lang.Math;

public class ConnectFour {
	
    // stores the master copy of the board in a two dimen. integer array 6 rows
    // by 7 columns, this should be copied wherever used to avoid overwriting the
    // master copy of the gameBoard.
    int [][] gameBoard = new int[6][7];

    // global used by seekMove to find the next optimal move
    // for the given player
    int MAX_DEPTH = 5; // look-ahead depth used by recursive step to stop if
                       // computation becomes too hardware intensive, this value
                       // is quite difficult to pin point for any given machine.
                       // values as low as 5 should be fine for any modern system,
                       // our statistics include values as high as 7.

    int SEARCH_SIZE = 0; // size of the search in question, should be made 0 just
                         // before seekMove() and incremented inside of minimax
                         // examining after minimax is complete will show the total
                         // number of "nodes" explored

    int SEARCH_TIME = 0; // seconds of search, should be made 0 just before seekMove()
                         // and examined just after. used to calculate average time
                         // per move of a game

    // global variables used for mass testing and debugging as well as gathering
    // statistics on the AI
    int TEST_SIZE = 100; // default 100
    double [] AVG_SEARCH_SIZES;
    long [] AVG_TIMES;
    int [][][] TEST_BOARDS;
    int [] PLAYER_WON;                  

    /*
     * validMove attempts to play a move at the target column on the master
     * copy of the game board
     *
     * int col: the column the player would like to play
     *
     * returns the row the piece would be located at if valid
     * returns -1 if not valid
    */
    public int validMove(int col) {
        // travel up the column looking for a place the piece
        // can go, keep in mind the top of the column is 0 and
        // the bottom is 5.
        for(int i = 5; i > -1; i--) {
            if(gameBoard[i][col] == 0) {
                return i;
            }
        }

        // for loop never found a spot
        return -1;
    }

    /*
     * seekValid attempts to play a move at the target column on the given
     * copy of the connectfour board
     *
     * int [][] board: the board to be played on
     * int col: the column the player would like to play
     *
     * returns the row the piece would be located at if valid
     * returns -1 if not valid
    */
    public int seekValid(int [][] board, int col) {
        // travel up the column looking for a place the piece
        // can go, keep in mind the top of the column is 0 and
        // the bottom is 5.
        for(int i = 5; i > -1; i--) {
            if(board[i][col] == 0) {
                return i;
            }
        }

        // for loop never found a spot
        return -1;
    }

    /*
     * seekMove is the call to the recursive minimax function, seekMove
     * performs some necessary checks on the board before undergoing the
     * immense computation of minimax.
     *
     * int [][] board: the board at a given recursive step
     * int player: the player going on this turn depth
     *
     * returns the optimal move defined when the recursion bottoms out
     * and all of the scores are calculated for the "terminal" states.
    */
    public int seekMove(int [][] board, int player) {
        int oPlayer; // player's opponnent

        if(player == 1) {
            oPlayer = 2;
        } else {
            oPlayer = 1;
        }

        // operations that must be done before recursion begins

        // copy the master copy of the gameboard
        int [][] boardCopy = new int[6][7];

        // create a copy of the master copy of the board
        for(int i = 0; i < 6; i++) {
            for(int j = 0; j < 7; j++) {
                boardCopy[i][j] = board[i][j];
            }
        }

        // if the given player's next move can be a win, don't perform
        // recursion, just play that move. acts as a short circuit from
        // performing immense amounts of computation just to find this
        // out later anyway.
        for(int i = 0; i < 7; i++) {
            int row = seekValid(boardCopy, i);
            if(row >= 0) {
                int win = seekWin(boardCopy, player, row, i);
                if(win == 0) {
                    //System.out.println("AI wants to play " + i + " because its a win!");
                    return i;
                }
            }
        }

        // if the given player's opponent's next move could be a win,
        // our move is forced and we have to play that column. acts as a 
        // short circuit from performing immense amounts of computation
        // just to find this out later anyway.
        //
        // Note: this is done AFTER looking to see if the player can win,
        // which is important, because if we waste a turn blocking the
        // opponents win when we could have won on that turn, that is
        // illogical.
        //
        // From what I can tell, this is never bad, because if we happened
        // to play a fatal column and lost on the next turn, then we would
        // have lost anyway.
        //
        for(int i = 0; i < 7; i++) {
            int row = seekValid(boardCopy, i);
            if(row >= 0) {     
                int win = seekWin(boardCopy, oPlayer, row, i);
                if(win == 0) {
                    //System.out.println("AI wants to play " + i + " or else it will lose!");
                    return i;
                }
            }
        }

        int [] tuple;

        // pass the copy to minimax along with the player who called seekMove()
        // and the depth (always 0) and 0, 0 for row and column to signify
        // that this is the initial call to minimax.
        tuple = minimax(boardCopy, player, 0, 0, 0);
        
        // tuple[1] should never be an invalid column but for some reason sometimes
        // it is, here I ensure that at least some column is passed back in case
        if(tuple[1] < 0 || tuple[1] > 6) {
            int row = -1;
            int col = -1;
            for(int i = 0; i < 7; i++) {
                row = seekValid(board, i);
                
                if(row != -1 && col == -1) {
                    col = i;
                }
            }
            return col;
        }

        //System.out.println("AI has decided that " + tuple[1] + " is the best column!");
        //System.out.println("AI has calculated a score of " + tuple[0] + ".");
        return tuple[1]; // the optimum move found by our recursive minimax
                         // which assumes that both the player and opposing
                         // player made the best possible move at every depth
                         // based off of our utility function seekScore()
    }

    /*
     * minimax is the recursive call that goes through all possible
     * permutations of the game state down to a given depth (hardware
     * dependent) and calculates the score at that depth based upon
     * our set of weighted values.
     *
     * NOTE THAT MY MINIMAX TAKES FUNCTIONAL INSPIRATION FROM A FUNCTION USED
     * BY Lukas Vermeer, NONE OF IT IS COPIED, BUT MY CALLS HAVE A SIMILAR FORMAT
     * The GitHub in which this function is located: https://github.com/lukasvermeer/minimax
     *
     * int [][] board: the permutation of the game board at a given depth
     * int player: the player the move is being calculated for, alternates
     *             as minimax deepens into the gamestate space
     * int depth: depth a given minimax call is searching at, starts at 0
     *            and works its way down to MAX_DEPTH
     * int x: the row that was last played by the player passed to the
     *        function (used to short circuit wins/losses with score)
     * int y: the column that was last played by the player passed to the
     *        function (used to short circuit wins/losses with score)
     *
     * returns a two tuple containing firstly the score of the best column
     * and secondly the column for which this score was found. if the player
     * is 1, the score should be positive, if the player is 2, the score should
     * be negative. if the two tuple has a -1 for column, a terminal state
     * has been reached for the recursion.
    */
    public int [] minimax(int [][] board, int player, int depth, int x, int y) {
        SEARCH_SIZE++; // increment search size to find total searches made
                       // if the search terminated in seekMove() and minimax
                       // was never called this will appear as 0 in the print
                       // in main().
                       
        // figure out the next player to go similarly to the main method
        //
        int nextPlayer;
        if (depth > 0) {             
            if(player % 2 == 0) {
                nextPlayer = 1;
            } else {
                nextPlayer = 2;
            }
        } else {
            nextPlayer = player;
        }

        // maxScore and minScore values for the explored gamespace of the tree
        // these are calculated from the bottom up once all nodes are explored
        // down to the bottom of the gamespace until MAX_DEPTH (unless a short
        // circuit occurs) 
        // * maxScore contains the maximum score for player 1
        // * minScore contains the maximum (called minimum as this was originally
        //   coded assuming the point of view of player 1) score for player 2

        int maxScore = Integer.MIN_VALUE + 1; // absurd values mark yet to be defined
        int minScore = Integer.MAX_VALUE - 1;

        // columns that go along with the maximum scores
        int maxColumn = -1; // -1 to mark that theyve yet to be defined
        int minColumn = -1;

        // curScore is the score of the current board
        // tuple is a two tuple which will contain the maximum score for
        // whatever player went last and the move corresponding to that score
        int [] tuple;

        // this is the score of the board in which the given player made
        // the given move at the given row and column at the given iteration
        // depth
        int curScore = seekScore(board, player, x, y);

        // check to see if the recursion has reached a terminal state:
        // * a terminal state has been reached if the score value is int.MAX
        //   which denotes a win for player 1 (loss for player 2)
        // * a terminal state has been reached if the score value is int.MIN
        //   which denotes a win for player 2 (loss for player 1)
        // * a terminal state has been reached if the depth is MAX_DEPTH
        // * a terminal state has been reached if there are no valid columns
        //   left to play (draw)
        //
        if(curScore == Integer.MAX_VALUE || curScore == Integer.MIN_VALUE) {
            // column is -1 because the game is already over for this game
            // space
           return new int [] {curScore, -1};
        }

        if(depth == MAX_DEPTH) {
           // column is -1 because the MAX_DEPTH has been reached for this
           // game space
           // System.out.println("Terminal state of max depth reached with score: " + curScore);
           return new int [] {curScore, -1};
        }

        if(seekDraw(board) == 0) {
           // column is -1 because the game is already over
           // (in this case curScore isn't well defined, but that's okay
           // because the game is over)
           return new int [] {curScore, -1}; 
        }

        // if the above optimizations and checks pass, start calling minimax
        // recursively for all valid columns from left to right
        //
        for(int i = 0; i < 7; i++) {
            int row = seekValid(board, i);
            if(row >= 0) {
                // copy the board
                int [][] newBoard = new int[6][7];
                for(int j = 0; j < 6; j++) {
                    for(int k = 0; k < 7; k++){
                            newBoard[j][k] = board[j][k];
                    }
                }

                // place the piece where its supposed to go for this new state
                // in the copy of the board
                newBoard[row][i] = player;

                // explore the next players gamespace at the next lookahead
                // depth for this board copy with the current players move,
                // returns either the maximum score and move associated with
                // that score, or just the score in the case of a terminal
                // state
                //
                tuple = minimax(newBoard, nextPlayer, depth+1, row, i);

                // as recursion wraps up, compare max and min values
                if(maxScore == Integer.MIN_VALUE + 1 || tuple[0] > maxScore) {
                    maxScore = tuple[0]; // from the return tuple
                    maxColumn = i; // from the loop above
                }

                if(minScore == Integer.MAX_VALUE - 1 || tuple[0] < minScore) {
                    minScore = tuple[0]; // from the return tuple
                    minColumn = i; // from the loop above
                }
            }
        }

        // return value based on the current player, values and scores are divided
        // by two so that deeper values are not preferred over shallower values
        // ie an optimal win 3 turns away should be better than a win 5 turns away
        if(player % 2 == 1) {
            return new int [] {(maxScore/2 + curScore/2), maxColumn};
        } else {
            return new int [] {(minScore/2 + curScore/2), minColumn};
        }
    }

    /*
     * seekScore calculates the score for a given board and player, if the
     * board was a win for the given player who made the move at row x and
     * column y, then don't bother calculating score because the game is
     * is over
     *
     * int [][] board: the board that contains the last move made by the
     *                 player at the given row and column
     * int player: the player that made the last move
     * int x: the row in which the move was made
     * int y: the column in which the move was made
    */
    public int seekScore(int [][] board, int player, int x, int y) {
        int score; // value for the score

        // last players move was a win for them, this means the game is over
        // for this explored game space. return max and min integer values
        // respectively
        //
        if(seekWin(board, player, x, y) == 0) {
            if(player == 1) {
                return Integer.MAX_VALUE;
            } else {
                return Integer.MIN_VALUE;
            }
        } 

        // last move was not a win, so calculate score normally
        //

        int [] tuple; // two tuple containing disjoint counts

        int threes = 0; // number of disjoint three stripes
        int threew = 100; // weight applied to three stripes to calculate score

        int twos = 0; // number of disjoint two stripes
        int twow = 10; // weight applied to two stripes to calculate score

        int ones = 0; // number of disjoint one stripes
        int onew = 1; // weight applied to one stripes to calculate score

        // populate the tuple the number of twos and threes with seekDisjoint()
        tuple = seekDisjoints(board, player);

        ones = tuple[0];
        twos = tuple[1];
        threes = tuple[2];

        int fatalc = 0; // number of fatal columns
        int fatalw = 1000; // weight applied to fatal columns to calculate score

        fatalc = seekFatal(board, player);

        // score is calculated with attributes and their respective weights
        score = threes*threew +
                twos*twow +
                ones*onew +
                fatalc*fatalw;

        // returns positive score for player 1, negative score for player 2
        if(player == 1) {
            return score;
        } else {
            return -score;
        }
    }

    /*
     * seekDisjoints counts the number of disjoint sets on a board belonging
     * to the given player. the concept of a disjoint set is explained in more
     * detail in our final report, but for now the move stripe [O,-,O,O] can
     * be thought of as disjoint set of three player 2 pieces, while the set
     * [O,X,O,O] cannot because the second column of the set is filled and
     * cannot be played. with this in mind we choose to check columns that
     * only have blank (0 in our case '-' in the example) spaces.
     *
     * int [][] board: the current permutation of the game board
     * int player: the player whose sets are being counted
     *
     * returns a two tuple of the number of disjoint two in a row and three
     * in a row sets.
    */
    public int [] seekDisjoints(int [][] board, int player) {
        int ones = 0;
        int twos = 0;
        int threes = 0;

        // horizontally and vertically
        for(int i = 0; i < 6; i++) {
            for(int j = 0; j < 7; j++) {
                // check horizontal boundary (don't go off the board)
                if(j+3 <= 6) {
                    int horiz = 0;
                    boolean none = false;

                    // each if checks a consecutive piece on the four stripe
                    // examined, for a total of four stripes checked per row,
                    // 24 in the entire board.

                    // check the first piece in the stripe
                    if(board[i][j] == player) {
                        horiz++;
                    } else if (board[i][j] != player && board[i][j] != 0) {
                        // if theres an opponents piece, there can be neither
                        // a disjoint two connection or a disjoint three
                        // connection along this four stripe
                        none = true;
                        horiz = 0;
                    }

                    // check the second piece in the stripe
                    if(none == false && board[i][j+1] == player) {
                        horiz++;
                    } else if (board[i][j+1] != player && board[i][j+1] != 0) {
                        none = true;
                        horiz = 0;
                    }

                    // check the third piece in the stripe
                    if(none == false && board[i][j+2] == player) {
                        horiz++;
                    } else if (board[i][j+2] != player && board[i][j+2] != 0) {
                        none = true;
                        horiz = 0;
                    }

                    // check the fourth piece in the stripe
                    if(none == false && board[i][j+3] == player) {
                        horiz++;
                    } else if (board[i][j+3] != player && board[i][j+3] != 0) {
                        none = true;
                        horiz = 0;
                    }

                    // if there were three pieces in a disjoint set, then this
                    // is a three, if there were only two, then this is a two,
                    // if there was only one, this is a one.
                    // three ex: [O,-,O,O]
                    // two ex: [O,-,-,O]
                    // one ex: [O,-,-,-]
                    // if any of the above sets contain an opponents piece,
                    // they are neither of any of the disjoint set types.
                    if(none == false && horiz == 3) {
                        threes++;
                    }

                    if(none == false && horiz == 2) {
                        twos++;
                    }

                    if(none == false && horiz == 1) {
                        ones++;
                    }
                }

                // check vertical boundary
                if(i+3 <= 5) {
                    int verti = 0;
                    boolean none = false;

                    // each if checks a consecutive piece on the four stripe
                    // for a total of 3 stripes per column, and 21 total
                    // in the entire board

                    // check the first piece in the stripe
                    if(board[i][j] == player) {
                        verti++;
                    } else if (board[i][j] != player && board[i][j] != 0) {
                        // if theres an opponents piece, there can be neither
                        // a disjoint two connection or a disjoint three
                        // connection here.
                        none = true;
                        verti = 0;
                    }

                    // check the second piece in the stripe
                    if(none == false && board[i+1][j] == player) {
                        verti++;
                    } else if (board[i+1][j] != player && board[i+1][j] != 0) {
                        none = true;
                        verti = 0;
                    }

                    // check the third piece in the stripe
                    if(none == false && board[i+2][j] == player) {
                        verti++;
                    } else if (board[i+2][j] != player && board[i+2][j] != 0) {
                        none = true;
                        verti= 0;
                    }

                    // check the fourth piece in the stripe
                    if(none == false && board[i+3][j] == player) {
                        verti++;
                    } else if (board[i+3][j] != player && board[i+3][j] != 0) {
                        none = true;
                        verti = 0;
                    }

                    if(none == false && verti == 3) {
                        threes++;
                    }

                    if(none == false && verti == 2) {
                        twos++;
                    }

                    if(none == false && verti == 1) {
                        ones++;
                    }
                }

                // check right diagonal boundary (right diagonal is from the
                // bottom left to the top right)
                if(i >= 3 && j <= 3) {
                    int rightDiag = 0;
                    boolean none = false;

                    // each if checks a consecutive piece on the four stripe
                    // for a total of 12 stripes on the entire board.

                    // check the first piece in the stripe
                    if(board[i][j] == player) {
                        rightDiag++;
                    } else if (board[i][j] != player && board[i][j] != 0) {
                        // if theres an opponents piece, there can be neither
                        // a disjoint two connection or a disjoint three
                        // connection here.
                        none = true;
                        rightDiag = 0;
                    }

                    // check the second piece in the stripe
                    if(none == false && board[i-1][j+1] == player) {
                        rightDiag++;
                    } else if (board[i-1][j+1] != player && board[i-1][j+1] != 0) {
                        none = true;
                        rightDiag = 0;
                    }

                    // check the third piece in the stripe
                    if(none == false && board[i-2][j+2] == player) {
                        rightDiag++;
                    } else if (board[i-2][j+2] != player && board[i-2][j+2] != 0) {
                        none = true;
                        rightDiag = 0;
                    }

                    // check the fourth piece in the stripe
                    if(none == false && board[i-3][j+3] == player) {
                        rightDiag++;
                    } else if (board[i-3][j+3] != player && board[i-3][j+3] != 0) {
                        none = true;
                        rightDiag = 0;
                    }

                    if(none == false && rightDiag == 3) {
                        threes++;
                    }

                    if(none == false && rightDiag == 2) {
                        twos++;
                    }

                    if(none == false && rightDiag == 1) {
                        ones++;
                    }
                }

                // check left diagonal boundary (left diagonal is from top
                // left to the bottom right)
                if(i <= 2 && j <= 3) {
                    int leftDiag = 0;
                    boolean none = false;

                    // each if checks a consecutive piece on the four stripe
                    // for a total of 12 stripes on the entire board.

                    // check the first piece in the stripe
                    if(board[i][j] == player) {
                        leftDiag++;
                    } else if (board[i][j] != player && board[i][j] != 0) {
                        // if theres an opponents piece, there can be neither
                        // a disjoint two connection or a disjoint three
                        // connection here.
                        none = true;
                        leftDiag = 0;
                    }

                    // check the second piece in the stripe
                    if(none == false && board[i+1][j+1] == player) {
                        leftDiag++;
                    } else if (board[i+1][j+1] != player && board[i+1][j+1] != 0) {
                        none = true;
                        leftDiag = 0;
                    }

                    // check the third piece in the stripe
                    if(none == false && board[i+2][j+2] == player) {
                        leftDiag++;
                    } else if (board[i+2][j+2] != player && board[i+2][j+2] != 0) {
                        none = true;
                        leftDiag = 0;
                    }

                    // check the fourth piece in the stripe
                    if(none == false && board[i+3][j+3] == player) {
                        leftDiag++;
                    } else if (board[i+3][j+3] != player && board[i+3][j+3] != 0) {
                        none = true;
                        leftDiag = 0;
                    }

                    if(none == false && leftDiag == 3) {
                        threes++;
                    }

                    if(none == false && leftDiag == 2) {
                        twos++;
                    }

                    if(none == false && leftDiag == 1) {
                        ones++;
                    }
                }
            }
        }

        return new int [] {ones, twos, threes};
    }

    /*
     * seekFatal returns the number of fatal columns controlled by the given
     * player, which is to say that they are good for the given player because
     * if the opposing player places a piece there, we can win next turn.
     *
     * int [][] board: the current permutation of the game board
     * int player: the player who would control a fatal column if one existed
     *
     * returns the number of fatal columns controlled by the given player
    */
    public int seekFatal(int [][] board, int player) {
        int fatalc = 0;

        // check only those rows which could have a fatal column, which is to
        // say they have at most four pieces in their columns.
        for(int i = 0; i < 7; i++) {
            if(board[1][i] == 0) {
                // the location at the row (row) and column (i) is where the opposing
                // player would have to go to see if this is a fatal column
                int row = seekValid(board, i);

                // if the position directly above the row (row) at the column (i)
                // results in a win for the given player, then this is a fatal
                // column. (notice that seekWin copies the given board, so this
                // will not alter the board).
                if(seekWin(board, player, row-1, i) == 0) {
                        fatalc++;
                }
            }
        }
        return fatalc;
    }

    /*
     * seekDraw finds if the given game is a draw w/o relying on turn counter
     *
     * int [][] board: the board at a given permutation of the game space
     *
     * returns 0 if a draw was found
     * returns -1 otherwise
    */
    public int seekDraw(int [][] board) {
        int rows = 0;

        // check to see if the top row has seven pieces in it
        for(int i = 0; i < 7; i++) {
            if(board[0][i] == 1 || board[0][i] == 2) {
                rows++;
            }
        }

        // if there were 7 spaces in the top row a draw was found
        if(rows == 7) {
            return 0;
        } else {
            return -1;
        }
    }

    /*
     * seekWin is used to tell if a given board contains a win for the
     * given player about to make the given move, or if the move has already
     * been made, then it checks if it was a win.
     *
     * int [][] oldBoard: the board at the given recursive step
     * int player: the given player about to make the move
     * int x: the row the piece would be placed in
     * int y: the column the piece would be placed in
     *
     * returns 0 if the given board contains a win for the given player with
     * a move in the given column and row
     * returns -1 otherwise
    */
    public int seekWin(int [][] oldBoard, int player, int x, int y) {
        // assumes that the row and column the that are being checked for
        // a hypothetical win are validated by validMove()

        // copy the board and place the hypothetical move
        int [][] board = new int[6][7];

        for(int i = 0; i < 6; i++) {
            for(int j = 0; j < 7; j++) {
                board[i][j] = oldBoard[i][j];
            }
        }

        // if we're using seekWin assuming that the move has already been
        // made by the player, this changes nothing
        board[x][y] = player;

        // same as checkWin, but with a hypothetical board instead of
        // the master copy

        // counters for all possible wins
        int leftDiag = 0; // diagonal going from top left to bottom right
        int rightDiag = 0; // diagonal going from bottom left to top right
        int horiz = 0; // horizontally
        int verti = 0; // vertically

        // run this loop seven times checking seven move
        // stripes along all the possible ways a player could
        // win
        for(int i = -3; i < 4; i++) {
            // keep in mind that x is a row from 0 to 5 and
            // y is a column from 0 to 6, they arent cartesian
            // coordinates.

            // boundary check for left diagonal
            if( ((x + i) >= 0 && (x + i) <= 5) && ((y + i) >= 0 && (y + i) <= 6) ) {
                // check if we have found another piece in a row
                if(board[x + i][y + i] == player) {
                    // increment counter if the piece is the players
                    leftDiag++;
                } else {
                    // reset the counter if the piece is not
                    leftDiag = 0;
                }
            }

            // boundary check for right diagonal
            if( ((x - i) >= 0 && (x - i) <= 5) && ((y + i) >= 0 && (y + i) <= 6)  ) {
                if(board[x - i][y + i] == player) {
                    rightDiag++;
                } else {
                    rightDiag = 0;
                }
            }

            // boundary check for horizontal
            if( (y + i) >= 0 && (y + i) <= 6 ) {
                if(board[x][y + i] == player) {
                    horiz++;
                } else {
                    horiz = 0;
                }
            }

            // boundary check for vertical
            if( (x + i) >= 0 && (x + i) <= 5 ) {
                if(board[x + i][y] == player) {
                    verti++;
                } else {
                    verti = 0;
                }
            }

            // if any of the counters have reached four
            // there was a win
            if(leftDiag == 4 || rightDiag == 4 || horiz == 4 || verti == 4) {
                return 0;
            }
        }

        // if the for loop completes and this function
        // hasn't yet returned 0, there were no wins
        return -1;
    }

    /*
     * checks if the last move given with (x,y) made by the
     * given player is a win. checks the master copy of the board.
     *
     * int player: player who made the move
     * int x: row in which the move was made
     * int y: column in which the move was made
     *
     * returns 0 if the move was a win
     * returns -1 if the move was not
    */
    public int checkWin(int player, int x, int y) {
        // counters for all the possible ways a player could
        // win
        int leftDiag = 0; // diagonal going from top left to bottom right
        int rightDiag = 0; // diagonal going from bottom left to top right
        int horiz = 0; // horizontally
        int verti = 0; // vertically

        // run this loop seven times checking seven move
        // stripes along all the possible ways a player could
        // win
        for(int i = -3; i < 4; i++) {
            // keep in mind that x is a row from 0 to 5 and
            // y is a column from 0 to 6, they arent cartesian
            // coordinates.

            // boundary check for left diagonal
            if( ((x + i) >= 0 && (x + i) <= 5) && ((y + i) >= 0 && (y + i) <= 6) ) {
                // check if we have found another piece in a row
                if(gameBoard[x + i][y + i] == player) {
                    // increment counter if the piece is the players
                    leftDiag++;
                } else {
                    // reset the counter if the piece is not
                    leftDiag = 0;
                }
            }

            // boundary check for right diagonal
            if( ((x - i) >= 0 && (x - i) <= 5) && ((y + i) >= 0 && (y + i) <= 6)  ) {
                if(gameBoard[x - i][y + i] == player) {
                    rightDiag++;
                } else {
                    rightDiag = 0;
                }
            }

            // boundary check for horizontal
            if( (y + i) >= 0 && (y + i) <= 6 ) {
                if(gameBoard[x][y + i] == player) {
                    horiz++;
                } else {
                    horiz = 0;
                }
            }

            // boundary check for vertical
            if( (x + i) >= 0 && (x + i) <= 5 ) {
                if(gameBoard[x + i][y] == player) {
                    verti++;
                } else {
                    verti = 0;
                }
            }

            // if any of the counters have reached four
            // there was a win
            if(leftDiag == 4 || rightDiag == 4 || horiz == 4 || verti == 4) {
                return 0;
            }
        }

        // if the for loop completes and this function
        // hasn't yet returned 0, there were no wins
        return -1;
    }

    /*
     * prints a given board, used for debugging
     *
     * int [][] board: the board to print
    */
    public void printBoard(int [][] board) {
        System.out.println("    DEBUG BOARD:");
        for(int i = 0; i < 6; i++) {
            System.out.print("  ");
            for(int j = 0; j < 7; j++) {
                System.out.print(board[i][j] + " ");
            }
            System.out.println();
        }
        System.out.println("    DEBUG BOARD:");
    }

    /*
     * prints a human readable string for the master copy of the gameboard
    */
    public void print() {
        System.out.println();
        for(int i = 0; i < 6; i++) {
            for(int j = 0; j < 7; j++) {
                System.out.print(gameBoard[i][j] + " ");
            }
            System.out.println();
        }
        System.out.println();
    }
    
    /*
     * generates a random valid board and returns it as a 6 x 7 integer array
     *
     * int [][] board: the board to be overwritten and randomized
    */
    public void generateBoard(int [][] board) {
        // blank board
        for(int i = 0; i < 6; i++) {
            for(int j = 0; j < 7; j++) {
                board[i][j] = 0;
            }
        }
        
        // simply play a random valid column as long as it isn't a win, repeat
        // until there are 10 pieces on the board
        for(int i = 0; i < 10; i++) {
            int row = -1;
            int win = -1;
            int player = 0;
            
            if((i+1) % 2 == 0) {
                player = 2;
            } else {
                player = 1;
            }
            
            while(row < 0) {
                int col = (int) (Math.random() * 7);
                
                row = seekValid(board, col);
                
                if(row >= 0) {
                    win = seekWin(board, player, row, col);

                    // if the row was valid and not a win, place the piece on
                    // the board
                    if(win != 0) {
                        board[row][col] = player;
                    } else {
                        row = -1;
                    }
                }
            }
        }
    }
	
    /*
     * plays a connect four game in test mode, or regular mode
    */
    public static void main(String[] args) {
        // used later for reading input text
        Scanner move = new Scanner(System.in);

        // test mode implies that both players are AI, and that they
        // will play a designated amount of games to populate the statistics
        // global arrays
        System.out.println("Test mode: Two AI play each other on X randomly generated boards.");
        System.out.println("Regular play: User designates which players are AI, plays the game with them.");
        System.out.print("Enter 0 for test mode, Enter 1 for regular play: ");
        int gameMode = move.nextInt();

        while(gameMode < 0 || gameMode > 1) {
            System.out.print("Enter 0 for test mode, Enter 1 for regular play: ");
            gameMode = move.nextInt();
        }
        
        System.out.println();

        // test mode, play x games with both players as AI, store info in globals
        if(gameMode == 0) {
            ConnectFour game = new ConnectFour();
            
            System.out.println("Running in test mode, how many games should be played? (1-1000)");
            game.TEST_SIZE = move.nextInt();
            
            while(game.TEST_SIZE > 1000 || game.TEST_SIZE < 0) {
                System.out.println("Values 1 - 1000 please");
                game.TEST_SIZE = move.nextInt();
            } 
            
            game.TEST_BOARDS = new int [game.TEST_SIZE][6][7];
            game.AVG_SEARCH_SIZES = new double [game.TEST_SIZE];
            game.AVG_TIMES = new long [game.TEST_SIZE];
            game.PLAYER_WON = new int [game.TEST_SIZE];
            
            // randomly generate TEST_SIZE amount of game boards and then play them
            // in the next for loop
            System.out.println();
            System.out.println("Please be patient, generating all of the boards...");
            for(int i = 0; i < game.TEST_SIZE; i++) {
                game.generateBoard(game.TEST_BOARDS[i]);
            }
            System.out.println(game.TEST_SIZE + " boards generated successfully, playing them will take some time.");
            
            int printing = 0; // enable or disable printing by altering this value
            
            System.out.println();
            
            System.out.println("Would you like to enable printing of the boards as they are played? Reccomend to disable for data collection");
            System.out.println("Enter 0: printing off, Enter 1: printing on ");
            printing = move.nextInt();
            
            while(printing > 1 || printing < 0) {
                System.out.println("Enter 0: printing off, Enter 1: printing on ");
                printing = move.nextInt();
            }
            
            System.out.println();
            
            for(int i = 0; i < game.TEST_SIZE; i++) {
                System.out.println("Playing game " + (i+1) + "...");
                // new game, new board
                for(int j = 0; j < 6; j++) {
                    for(int k = 0; k < 7; k++) {
                        game.gameBoard[j][k] = game.TEST_BOARDS[i][j][k];
                    }
                }

                // controls the state of the game, 1 while playing,
                // 0 when the game finishes
                int gameState = 1;

                // number of turns passed (all of my generated boards are 10 turns in)
                int turn = 10;

                // number of player going, 0 for now but 1 and 2 later
                int player = 0;

                // valid returned by check win
                int win = 0;

                // plays the connect four game here
                while (gameState == 1) {
                    // debugging stuff
                    if(printing == 1) {
                        game.print();
                        int [] tuple = game.seekDisjoints(game.gameBoard, 1);
                        int [] tuple2 = game.seekDisjoints(game.gameBoard, 2);
                        System.out.println("Player 1 - Ones: " + tuple[0] + " Twos: " + tuple[1] + " Threes: " + tuple[2]);
                        System.out.println("Player 2 - Ones: " + tuple2[0] + " Twos: " + tuple2[1] + " Threes: " + tuple2[2]);
                    }

                    // player 1 goes on odd turns, player 2 goes on even
                    if(turn % 2 == 0) {
                        player = 1;
                    } else {
                        player = 2;
                    }

                    int x = -1;
                    int y = -1;
                    while(x < 0) {

                        // ai 1 (always ai in test mode, to play with the ai run in regular mode)
                        if(player == 1) {
                            game.SEARCH_SIZE = 0;
                            if(printing == 1) {
                                System.out.println("Player " + player + ", which column (0-6) would you like to play?");
                            }
                            long start = System.currentTimeMillis();
                            y = game.seekMove(game.gameBoard, player);
                            long end = System.currentTimeMillis();
                            game.AVG_TIMES[i] += end-start;
                        }

                        // ai 2 (always ai in test mode, to play with the ai run in regular mode)
                        if(player == 2) {
                            game.SEARCH_SIZE = 0;
                            if(printing == 1) {
                                System.out.println("Player " + player + ", which column (0-6) would you like to play?");
                            }
                            long start = System.currentTimeMillis();
                            y = game.seekMove(game.gameBoard, player);
                            long end = System.currentTimeMillis();
                            game.AVG_TIMES[i] += end-start;
                        }

                        // validate user input
                        while(y < 0 || y > 6) {
                            System.out.println("For some reason the AI selected an invalid column, very bad.");
                            y = move.nextInt();
                        }

                        // validate that the move is possible
                        x = game.validMove(y);
                    }

                    // move was validated, play the piece and increment turn
                    game.gameBoard[x][y] = player;
                    turn++;
                    
                    game.AVG_SEARCH_SIZES[i] += game.SEARCH_SIZE;

                    // check win condition
                    win = game.checkWin(player, x, y);
                    if(win == 0) {
                        if(printing == 1) {
                            game.print();
                            System.out.println("Game Over: Player " + player + " wins!");
                        }
                        gameState = 0;
                        game.PLAYER_WON[i] = player;
                    }

                    // check draw condition
                    if(turn == 42 && win == -1) {
                        if(printing == 1) {
                            game.print();
                            System.out.println("Game Over: Draw");
                        }
                        gameState = 0;
                        game.PLAYER_WON[i] = 0; // 0 for draw
                    }
                }
                
                // calculate other statistics (remember that the boards started
                // at turn 10)
                game.AVG_SEARCH_SIZES[i] /= (turn-10);
                game.AVG_TIMES[i] /= (turn-10);
            }
            
        double player1 = 0;
        double player2 = 0;
        double draw = 0;
        double avgSearch = 0;
        double avgTime = 0;
        
        // per game results, might get cut off because it's too large
        System.out.println("GAME RESULTS:");
        System.out.println("--------------------------------------------------------------");
        for(int i = 0; i < game.TEST_SIZE; i++) {
            System.out.println("Game " + (i+1) + " outcome: " + game.PLAYER_WON[i]);
            System.out.println("Game " + (i+1) + " average search size: " + game.AVG_SEARCH_SIZES[i]);
            System.out.println("Game " + (i+1) + " average search time: " + game.AVG_TIMES[i]);
            System.out.println("--");
            
            avgSearch += game.AVG_SEARCH_SIZES[i];
            avgTime += game.AVG_TIMES[i];
            
            if(game.PLAYER_WON[i] == 1) {
                player1++;
            }
            
            if(game.PLAYER_WON[i] == 2) {
                player2++;
            }
            
            if(game.PLAYER_WON[i] == 0) {
                draw++;
            }
        }
        
        // calculate percentages and averages
        player1 /= game.TEST_SIZE;
        player2 /= game.TEST_SIZE;
        draw /= game.TEST_SIZE;
        avgSearch /= game.TEST_SIZE;
        avgTime /= game.TEST_SIZE;
        
        // print overall statistics
        System.out.println();
        System.out.println("OVERALL STATISTICS:");
        System.out.println("--------------------------------------------------------------");
        System.out.println("Player 1 win percentage: " + player1);
        System.out.println("Player 2 win percentage: " + player2);
        System.out.println("Draw percentage: " + draw);
        System.out.println("Average search size per turn: " + avgSearch);
        System.out.println("Average time per turn: " + avgTime);
        
                
        } else {
            // regular operation
            ConnectFour game = new ConnectFour();

            // empty board
            for(int i = 0; i < 6; i++) {
                for(int j = 0; j < 7; j++) {
                    game.gameBoard[i][j] = 0;
                }
            }

            // controls the state of the game, 1 while playing,
            // 0 when the game finishes
            int gameState = 1;

            // number of turns passed
            int turn = 0;

            // number of player going, 0 for now but 1 and 2 later
            int player = 0;

            // valid returned by check win
            int win = 0;
            
            // ask game mode configuration:
            System.out.println("Game Mode Configurations:");
            System.out.println("0: Player 1 - Human, Player 2 - Human");
            System.out.println("1: Player 1 - Human, Player 2 - AI");
            System.out.println("2: Player 1 - AI,    Player 2 - Human");
            System.out.println("3: Player 1 - AI,    Player 2 - AI");
            System.out.print("Enter Preference: ");
            int config = move.nextInt();
            
            while(config < 0 || config > 3) {
                System.out.print("Enter Preference: ");
                config = move.nextInt();
            }
            
            System.out.println();
            
            int oneIsAI = 0;
            int twoIsAI = 0;
            
            if(config == 0) {
                oneIsAI = 0;
                twoIsAI = 0;
            }
            
            if(config == 1) {
                oneIsAI = 0;
                twoIsAI = 1;
            }
            
            if(config == 2) {
                oneIsAI = 1;
                twoIsAI = 0;
            }
            
            if(config == 3) {
                oneIsAI = 1;
                twoIsAI = 1;
            }

            // plays the connect four game here
            while (gameState == 1) {
                // show the game board
                game.print();
                int [] tuple = game.seekDisjoints(game.gameBoard, 1);
                int [] tuple2 = game.seekDisjoints(game.gameBoard, 2);
                System.out.println("Player 1 - Ones: " + tuple[0] + " Twos: " + tuple[1] + " Threes: " + tuple[2]);
                System.out.println("Player 2 - Ones: " + tuple2[0] + " Twos: " + tuple2[1] + " Threes: " + tuple2[2]);

                // debugging stuff
                //int [] tuple = game.seekDisjoints(game.gameBoard, 1);
                //int [] tuple2 = game.seekDisjoints(game.gameBoard, 2);
                //System.out.println("Player 1 - Ones: " + tuple[0] + " Twos: " + tuple[1] + "Threes: " + tuple[2]);
                //System.out.println("Player 2 - Ones: " + tuple2[0] + " Twos: " + tuple2[1] + "Threes: " + tuple2[2]);

                // player 1 goes on odd turns, player 2 goes on even
                if(turn % 2 == 0) {
                        player = 1;
                } else {
                        player = 2;
                }

                // for now, ask what move the player would like to
                // make with text
                int x = -1;
                int y = -1;
                while(x < 0) {

                    // player 1
                    if(player == 1) {
                        System.out.println("Player " + player + ", which column (0-6) would you like to play?");
                        if(oneIsAI == 1) {
                            y = game.seekMove(game.gameBoard, player);
                        } else {
                            y = move.nextInt();
                        }
                    }

                    // player 2
                    if(player == 2) {
                        System.out.println("Player " + player + ", which column (0-6) would you like to play?");
                        if(twoIsAI == 1) {
                            y = game.seekMove(game.gameBoard, player);
                        } else {
                            y = move.nextInt();
                        }
                    }

                    // validate user input
                    while(y < 0 || y > 6) {
                        System.out.println("Player " + player + ", enter a valid column (0-6): ");
                        y = move.nextInt();
                    }

                    // validate that the move is possible
                    x = game.validMove(y);
                }

                // move was validated, play the piece and increment turn
                game.gameBoard[x][y] = player;
                turn++;

                // check win condition
                win = game.checkWin(player, x, y);
                if(win == 0) {
                    game.print();
                    System.out.println("Game Over: Player " + player + " wins!");
                    gameState = 0;
                }

                // check draw condition
                if(turn == 42 && win == -1) {
                    game.print();
                    System.out.println("Game Over: Draw");
                    gameState = 0;
                }
            }
        }

        // close the scanner
        move.close();
    }
}
