#pragma once
#include"UCTNode.hpp"
#include<ctime>
#include<cmath>
#include<cstdlib>
#include<cstring>
#include<utility>
#include<iostream>

// #define DEBUG_SIM
// #define DEBUG_BP
// #define DEBUG_TREE

const double TIME_LIMIT = 1.8 * CLOCKS_PER_SEC;
const int ITER_LIMIT = 1000000;
const double COEFF = 0.707;

#ifdef DEBUG_TREE
void printBoard(int **board, int h, int w, int noX, int noY)
{
    for (int i = 0; i < w; i++)
        std::cout << "- ";
    std::cout << std::endl;
	for (int i = 0; i < h; i++)
	{
		for (int j = 0; j < w; j++)
		{
			if (board[i][j] == 0)
			{
				if (i == noX && j == noY)
				{
					std::cout << "X ";
				}
				else
				{
					std::cout << ". ";
				}
			}
			else if (board[i][j] == 2)
			{
				std::cout << "A ";
			}
			else if (board[i][j] == 1)
			{
				std::cout << "B ";
			}
		}
		std::cout << std::endl;
	}
	return;
}
#endif

// upper confidence tree
class UCT {
private:
    UCTNode *root;
    int h, w; // height and width of the board
    int noX, noY; // banned spot
    clock_t start_time; // starting time
    int* position_weight;
    int total_weight;

public:
    UCT(int **_board, int _h, int _w, const int *_top, int _noX, int _noY, int _lastX, int _lastY) : h(_h), w(_w), noX(_noX), noY(_noY) {
        root = new UCTNode(_board, _h, _w, _top, noX, noY, _lastX, _lastY);
        start_time = clock();

        position_weight = new int[w];
        int mid = (w - 1) / 2;

        int i = 0;
        while (i <= mid) {
            position_weight[i] = i + 1;
            i++;
        }
        total_weight = i * (i + 1);
        if (w % 2) {
            total_weight -= i;
        }
        while (i < w) {
            position_weight[i] = position_weight[w-i-1];
            i++;
        }
    }
    ~UCT() {
        delete root;
    }

    //perform UCT search
    std::pair<int, int> search() {
        //next move ai can win
        for (int i = 0; i < w; i++) {
            if (root->top[i] > 0) {
                int row = root->top[i] - 1;
                root->board[row][i] = 2;
                if (machineWin(row, i, h, w, root->board)) {
                    root->board[row][i] = 0;
                    return std::pair<int, int>(row, i);
                }
                root->board[row][i] = 0;
            }
        }
        //next move user can win
        for (int i = 0; i < w; i++) {
            if (root->top[i] > 0) {
                int row = root->top[i] - 1;
                root->board[row][i] = 1;
                if (userWin(row, i, h, w, root->board)) {
                    root->board[row][i] = 0;
                    return std::pair<int, int>(row, i);
                }
                root->board[row][i] = 0;
            }
        }

        //keep track of the memory limit by keeping track of the iterations
        int iter = 0;
        while ((clock() - start_time < TIME_LIMIT) && (++iter < ITER_LIMIT)) {
            UCTNode *selected_node = treePolicy(); // selection and expansion
            double result = defaultPolicy(selected_node);// simulation
            backpropagate(selected_node, result);// backpropagation
        }
        // return the move to the best child
        UCTNode* best = bestMove();
        return std::pair<int, int>(best->move_x, best->move_y);
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
        int *new_top = new int[w];
        memcpy(new_top, node->top, sizeof(int) * w);

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

        #ifdef DEBUG_SIM
        std::cout << "Expanding a new node" << std::endl;
        printBoard(node->children[y]->board, h, w, noX, noY);
        #endif
        
        return node->children[y];   
    }

    // find the best child based on UCB
    UCTNode* bestChild(UCTNode *node) {
        double best_UCB = -RAND_MAX;
        UCTNode* best = nullptr;

        for (int i = 0; i < w; i++) {
            if (node->children[i]) {
                // we use negative instead of complement
                double temp_UCB = (node->ai_turn ? 1 : -1) * (double)(node->children[i]->profit) / (node->children[i]->visit_count)
                    + COEFF * sqrt(2 * log((double)(node->visit_count)) / (node->children[i]->visit_count));
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
        #ifdef DEBUG_SIM
            std::cout << "start simulation." << std::endl;
        #endif
        //set up a copy of board and top for simulation
        int **current_board = new int*[h];
        for (int i = 0; i < h; i++) {
            current_board[i] = new int[w];
            memcpy(current_board[i], node->board[i], sizeof(int) * w);
        }

        int *current_top = new int[w];
        memcpy(current_top, node->top, sizeof(int) * w);

        double profit = 0;
        bool ai_turn = node->ai_turn;

        int last_x = node->move_x;
        int last_y = node->move_y;

 
        //keep playing until the game is over
        while (true) {
            #ifdef DEBUG_SIM
            printBoard(current_board, h, w, noX, noY);
            #endif
            if (!ai_turn && machineWin(last_x, last_y, h, w, current_board)) { //cuz last x and last y is the last round
                profit = 1;
                #ifdef DEBUG_SIM
                std::cout << "AI won" << std::endl;
                std::cout << "profit: " << profit << std::endl;
                #endif
                break;
            } else if (ai_turn && userWin(last_x, last_y, h, w, current_board)) {
                profit = -1;
                #ifdef DEBUG_SIM
                std::cout << "USER won" << std::endl;
                std::cout << "profit: " << profit << std::endl;
                #endif
                break;
            } else if (isTie(w, current_top)) {
                profit = 0;
                break;
            }
            #ifdef DEBUG_SIM
            if (!ai_turn) {
                std::cout << "AI move" << std::endl;
            } else {
                std::cout << "USER move" << std::endl;
            }
            std::cout << "profit: " << profit << std::endl;
            #endif

            // simulate one turn
            ai_turn = !ai_turn;

            // choose a rank, middle spots have higher probability
            bool doneSelecting = false;
            while (!doneSelecting) {
                int lower_bound = rand() % total_weight;
                int tmp = 0;
                for (int i = 0; i < w; ++i) {
                    tmp += position_weight[i];
                    if (lower_bound < tmp) {
                        last_y = i;
                        break;
                    }
                }
                if (current_top[last_y] > 0) 
                    doneSelecting = true;
            }

            // int chosen_rank = rand() % valid_column_count;
            // last_y = valid_columns[chosen_rank];
            last_x = --current_top[last_y];

            current_board[last_x][last_y] = ai_turn? 1 : 2;

            //if banned spot, update top
            if (last_y == noY && last_x - 1 == noX) {
                current_top[last_y]--;
            }

            // //remove full column
            // if (current_top[last_y] == 0) { 
            //     valid_columns[chosen_rank] = valid_columns[--valid_column_count];
            // }

            
        }

        delete[] current_top;
        for (int i = 0; i < h; i++)
            delete[] current_board[i];
        delete[] current_board;
        // delete[] valid_columns;
        return profit;
    }

    //update the profit along the ancestor nodes
    void backpropagate(UCTNode* current_node, double profit) {
        while (current_node) {
            current_node->visit_count++;
            current_node->profit += profit;
            #ifdef DEBUG_BP
            std::cout << "bp: visit_count=" << current_node->visit_count << ", profit = " << current_node->profit << std::endl;
            #endif
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
                double temp_UCB = (root->ai_turn ? 1 : -1) * (double)(root->children[i]->profit) / (root->children[i]->visit_count);
                if (temp_UCB > best_UCB) {
                    best = root->children[i];
                    best_UCB = temp_UCB;
                }
                #ifdef DEBUG_TREE
                std::cout << "data:" << std::endl;
                std::cout << "profit: " << root->children[i]->profit << ", visit_count: " << root->children[i]->visit_count << std::endl;
                std::cout << "UCB: " << temp_UCB << std::endl;
                printBoard(root->children[i]->board, h, w, noX, noY);
                std::cout << std::endl;
                #endif
            }
        }

        return best;
    }
};
