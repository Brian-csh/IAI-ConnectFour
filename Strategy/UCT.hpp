#pragma once
#include"UCTNode.hpp"
#include<ctime>
#include<cmath>
#include<cstdlib>

#define TIME_LIMIT 2000
#define ITER_LIMIT 1000000
#define COEFF 0.707
// upper confidence tree
class UCT {
private:
    UCTNode *root;
    int h, w; // height and width of the board
    int noX, noY; // banned spot
    clock_t time; // starting time

public:
    UCT(int **_board, int _h, int _w, const int *_top, int _noX, int _noY) : h(_h), w(_w), noX(_noX), noY(_noY) {
        root = new UCTNode(_board, _h, _w, _top, noX, noY);
        time = clock();
    }
    ~UCT() {
        delete root;
    }

    //perform UCT search
    UCTNode* search() {
        //keep track of the memory limit by keeping track of the iterations
        int iter = 0;
        while (clock() - time < TIME_LIMIT && iter++ < ITER_LIMIT) {
            UCTNode *selected_node = treePolicy(); // selection and expansion
            double result = defaultPolicy(selected_node);// simulation
            backpropagate(selected_node, result);// backpropagation
        }
        // return the move to the best child
        return bestMove();
    }

    UCTNode* treePolicy() {
        UCTNode* curr = root;
        while (!curr->isTerminal()) {
            if (curr->expandable()) {
                return expand(curr);
            } else {
                curr = bestChild(curr);
            }
        }
        return curr;
    }

    // add one expandable node as a child of the current node
    // we randomly choose one from all the possible moves
    UCTNode* expand(UCTNode *node) {
        // choose one move and create a node for it
        int chosen_rank = rand() % node->expandable_count;
        int *new_top = new int[node->w];
        for (int i = 0; i < node->w; i++) {
            new_top[i] = node->top[i];
        }

        int y = node->expandable_nodes[chosen_rank]; // randomly chosen column
        int x = --new_top[y]; // fill the corresponding row

        // check if the next spot in the column is banned
        if (y == this->noY && x-1 == this->noX) {
            new_top[y]--;
        }

        node->children[y] = new UCTNode(node->board, h, w, new_top, noX, noY, x, y, !node->ai_turn, node); // create the node
        node->children[y]->board[x][y] = node->ai_turn ? 2 : 1; // apply the move
        node->removeExpandableNode(chosen_rank);
        delete[] new_top;
        
        return node->children[y];   
    }

    // find the best child based on UCB
    UCTNode* bestChild(UCTNode *node) {
        double best_UCB = -RAND_MAX;
        UCTNode* best = nullptr;

        for (int i = 0; i < w; i++) {
            if (node->children[i]) {
                // we use negative instead of complement
                double temp_UCB = (node->ai_turn ? 1 : -1) * node->children[i]->profit / (double)(node->children[i]->visit_count)
                    + COEFF * sqrt(2 * log((double)(node->visit_count)) / (double)(node->children[i]->visit_count));
                if (temp_UCB > best_UCB) {
                    best = node->children[i];
                    best_UCB = temp_UCB;
                }
            }
        }

        return best;
    }

    // perform simulation until a winner is decided
    // starting from node, simulate the game until a result is determined
    double defaultPolicy(UCTNode *node) {
        //set up a copy of board and top for simulation
        int **current_board = new int*[h];
        for (int i = 0; i < h; i++) {
            current_board[i] = new int[w];
            for (int j = 0; j < w; j++) {
                current_board[i][j] = node->board[i][j];
            }
        }

        int *current_top = new int[w];
        int *valid_columns = new int[w];
        int valid_column_count = 0;
        for (int i = 0; i < w; i++) {
            current_top[i] = node->top[i];
            if (current_top[i]) {
                valid_columns[valid_column_count++] = current_top[i];
            }
        }

        double profit = 0;
        bool ai_turn = node->ai_turn;

        int last_x = node->move_x;
        int last_y = node->move_y;

 
        //keep playing until the game is over
        while (true) {
            if (ai_turn && machineWin(last_x, last_y, h, w, current_board)) {
                profit = 1;
                break;
            } else if (!ai_turn && userWin(last_x, last_y, h, w, current_board)) {
                profit = -1;
                break;
            } else if (isTie(w, current_top)) {
                profit = 0;
                break;
            }

            //simulate one turn
            ai_turn = !ai_turn;
            int chosen_rank = rand() % valid_column_count;
            last_y = valid_columns[chosen_rank];
            last_x = --current_top[last_y];

            //remove full column
            if (current_top[last_y] == 0) { 
                valid_columns[chosen_rank] = valid_columns[--valid_column_count];
            }

            //if banned spot, update top
            if (last_y == noY && last_x - 1 == noX) {
                current_top[last_y]--;
            }

            current_board[last_x][last_y] = ai_turn? 2 : 1;
        }

        delete[] current_top;
        for (int i = 0; i < h; i++)
            delete[] current_board[i];
        delete[] current_board;
        return profit;
    }

    //update the profit along the ancestor nodes
    void backpropagate(UCTNode* current_node, double profit) {
        while (current_node) {
            current_node->visit_count++;
            current_node->profit += profit;
            current_node = current_node->parent;
        }
    }

    //determine the best move from root to next
    //i.e. select the move_x and move_y of the best child
    UCTNode* bestMove() {
        double best_UCB = -RAND_MAX;
        UCTNode* best = nullptr;

        for (int i = 0; i < w; i++) {
            if (root->children[i]) {
                // we use negative instead of complement
                double temp_UCB = (root->ai_turn ? 1 : -1) * root->children[i]->profit / (double)(root->children[i]->visit_count);
                if (temp_UCB > best_UCB) {
                    best = root->children[i];
                    best_UCB = temp_UCB;
                }
            }
        }

        return best;
    }
};