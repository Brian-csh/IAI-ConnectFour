#pragma once
#include"Judge.h"

/**
 * a node in UCT
 * stores the current board and other data relevant to the UCT algorithm
 */
class UCTNode {
    friend class UCT;
private:
    int **board; // 0: empty, 1: user, 2: ai (strategy)
    int h, w; // dimensions of the board
    int *top; // top spot of each column
    int move_x; // x-coordinate of the move
    int move_y; // y-coordinate of the move
    int noX, noY; // banned coordinates
    bool ai_turn; // whether it is the turn of the ai
    int visit_count; // number of times visited
    double profit;
    UCTNode *parent; // parent node
    UCTNode **children; // stores children (expanded nodes)
    int expandable_count; // number of nodes that can be expanded
    int *expandable_nodes; // list of nodes that can be expanded
    int terminal; // is terminal node (i.e., win, lose, tie)
public:
    // constructor
    UCTNode(int **_board, int _h, int _w, const int *_top, int _noX = -1, int _noY = -1, int _move_x = -1, int _move_y = -1, bool _ai_turn = true, UCTNode *_parent = nullptr)
        : h(_h), w(_w), noX(_noX), noY(_noY), move_x(_move_x), move_y(_move_y), ai_turn(_ai_turn),
          parent(_parent), children(new UCTNode*[_w]), expandable_nodes(new int[_w]), expandable_count(0), terminal(-1) {
        // set up the board
        board = new int*[h];
        for (int i = 0; i < h; i++) {
            board[i] = new int[w];
            for (int j = 0; j < w; j++) {
                board[i][j] = _board[i][j];
            }
        }

        top = new int[w];
        for (int i = 0; i < w; i++) {
            top[i] = _top[i];
            if (top[i]) { // if not full
                expandable_nodes[expandable_count++] = i; // add as a possible move
            }
            children[i] = nullptr;
        }
    }
    // destructor
    ~UCTNode() {
        delete[] expandable_nodes;
        delete[] top;
        for (int i = 0; i < h; i++)
            delete[] board[i];
        delete[] board;
        for (int i = 0; i < w; i++) {
            if (children[i])
                delete children[i];
        }
        delete[] children;
    }
    bool expandable() {
        return expandable_count > 0;
    }
    bool isTerminal() {
        if (terminal != -1) { // already set
            return terminal;
        }
        // determine whether game is over
        if (move_x == -1 && move_y == -1) { // game just started
            terminal = 0;
        } else if ((!ai_turn && userWin(move_x, move_y, h, w, board)) || (ai_turn && machineWin(move_x, move_y, h, w, board)) || isTie(w, top)){
            terminal = 1;
        } else {
            terminal = 0;
        }
        return terminal;
    }
    void removeExpandableNode(int rank) {
        expandable_nodes[rank] = expandable_nodes[--expandable_count];
    }
    int getMoveX() {
        return move_x;
    }
    int getMoveY() {
        return move_y;
    }
};