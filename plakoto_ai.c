#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define BOARD_SIZE 24
#define PLAYER_A 0
#define PLAYER_B 1
#define EMPTY 2

#define MAX_MOVES_PER_TURN 4
#define MAX_SEQUENCES 256

#define INF 1000000
#define MANA_PENALTY 500000

typedef struct {
    uint8_t count;
    uint8_t owner;
    bool pinned;
} Point;

typedef struct {
    Point board[BOARD_SIZE];
    uint8_t borne_off[2];
    uint8_t active_player;
} GameState;

typedef struct {
    int8_t from;
    int8_t to;
} Move;

typedef struct {
    Move moves[MAX_MOVES_PER_TURN];
    uint8_t count;
} MoveSequence;

typedef struct {
    MoveSequence seqs[MAX_SEQUENCES];
    uint32_t count;
    uint8_t max_moves;
} MoveList;

void print_board(const GameState *state);
bool is_home_board_only(const GameState *state, uint8_t player);
bool is_valid_move(const GameState *state, uint8_t player, int8_t from, int8_t to);
void apply_move(GameState *state, Move m);
void generate_moves_recursive(const GameState *state, uint8_t *dice, uint8_t num_dice, MoveSequence *current_seq, MoveList *list);
void get_all_legal_moves(const GameState *state, uint8_t d1, uint8_t d2, MoveList *list);
int32_t evaluate_board(const GameState *state);
double expectiminimax(GameState *state, int depth, uint8_t current_player);

bool is_home_board_only(const GameState *state, uint8_t player) {
    uint8_t count = state->borne_off[player];
    int8_t start = (player == PLAYER_A) ? 18 : 0;
    int8_t end = (player == PLAYER_A) ? 23 : 5;

    for (int i = start; i <= end; i++) {
        if (state->board[i].owner == player) {
            count += state->board[i].count;
        } else if (state->board[i].pinned && state->board[i].owner != player) {
            count += 1; 
        }
    }
    return count == 15;
}

bool is_valid_move(const GameState *state, uint8_t player, int8_t from, int8_t to) {
    if (from < 0 || from >= BOARD_SIZE) return false;
    
    if (state->board[from].owner != player && !(state->board[from].pinned && state->board[from].owner != player && state->board[from].count == 0)) {
        return false; 
    }
    if (state->board[from].count == 0 && !state->board[from].pinned) return false;

    if (to >= BOARD_SIZE || to < 0) {
        return is_home_board_only(state, player);
    }

    const Point *dest = &state->board[to];

    if (dest->owner == EMPTY) return true;
    if (dest->owner == player) return true;

    if (dest->owner != player && dest->count == 1 && !dest->pinned) {
        return true;
    }

    return false;
}

void apply_move(GameState *state, Move m) {
    uint8_t p = state->active_player;
    uint8_t opp = (p == PLAYER_A) ? PLAYER_B : PLAYER_A;

    if (state->board[m.from].count > 0) {
        state->board[m.from].count--;
    }
    if (state->board[m.from].count == 0) {
        if (state->board[m.from].pinned) {
            state->board[m.from].owner = opp;
            state->board[m.from].count = 1;
            state->board[m.from].pinned = false;
        } else {
            state->board[m.from].owner = EMPTY;
        }
    }

    if (m.to >= BOARD_SIZE || m.to < 0) {
        state->borne_off[p]++;
    } else {
        Point *dest = &state->board[m.to];
        if (dest->owner == EMPTY) {
            dest->owner = p;
            dest->count = 1;
        } else if (dest->owner == p) {
            dest->count++;
        } else if (dest->owner == opp && dest->count == 1) {
            dest->owner = p;
            dest->count = 1;
            dest->pinned = true;
        }
    }
}

void generate_moves_recursive(const GameState *state, uint8_t *dice, uint8_t num_dice, MoveSequence *current_seq, MoveList *list) {
    bool move_found = false;

    if (num_dice > 0) {
        uint8_t d = dice[0];
        int8_t dir = (state->active_player == PLAYER_A) ? 1 : -1;

        for (int8_t i = 0; i < BOARD_SIZE; i++) {
            if (state->board[i].owner == state->active_player || (state->board[i].owner == EMPTY && state->board[i].pinned == false)) {
                if (state->board[i].count == 0 && !state->board[i].pinned) continue;
            }

            int8_t to = i + (d * dir);
            
            if (is_valid_move(state, state->active_player, i, to)) {
                move_found = true;
                GameState next_state = *state;
                Move m = {i, to};
                apply_move(&next_state, m);

                current_seq->moves[current_seq->count++] = m;
                generate_moves_recursive(&next_state, dice + 1, num_dice - 1, current_seq, list);
                current_seq->count--;
            }
        }
    }

    if (!move_found || num_dice == 0) {
        if (current_seq->count > list->max_moves) {
            list->max_moves = current_seq->count;
            list->count = 0;
        }
        if (current_seq->count == list->max_moves && list->count < MAX_SEQUENCES) {
            list->seqs[list->count++] = *current_seq;
        }
    }
}

void get_all_legal_moves(const GameState *state, uint8_t d1, uint8_t d2, MoveList *list) {
    list->count = 0;
    list->max_moves = 0;
    MoveSequence current_seq = { .count = 0 };

    if (d1 == d2) {
        uint8_t dice[4] = {d1, d1, d1, d1};
        generate_moves_recursive(state, dice, 4, &current_seq, list);
    } else {
        uint8_t dice1[2] = {d1, d2};
        generate_moves_recursive(state, dice1, 2, &current_seq, list);
        
        MoveSequence seq2 = { .count = 0 };
        uint8_t dice2[2] = {d2, d1};
        generate_moves_recursive(state, dice2, 2, &seq2, list);
    }
}

int32_t evaluate_board(const GameState *state) {
    int32_t score = 0;

    if (state->board[0].pinned && state->board[0].owner == PLAYER_B) return -MANA_PENALTY;
    if (state->board[23].pinned && state->board[23].owner == PLAYER_A) return MANA_PENALTY;

    int32_t pips_A = 0;
    int32_t pips_B = 0;

    for (int i = 0; i < BOARD_SIZE; i++) {
        const Point *pt = &state->board[i];
        
        if (pt->owner == PLAYER_A) {
            pips_A += pt->count * (24 - i);
        } else if (pt->owner == PLAYER_B) {
            pips_B += pt->count * (i + 1);
        }

        if (pt->pinned) {
            if (pt->owner == PLAYER_A) {
                score += 500;
                pips_B += 1 * (i + 1);
            } else {
                score -= 500;
                pips_A += 1 * (24 - i);
            }
        }
        
        if (pt->count > 1) {
            if (pt->owner == PLAYER_A) score += 10;
            if (pt->owner == PLAYER_B) score -= 10;
        }
    }

    score += (pips_B - pips_A) * 2;
    score += state->borne_off[PLAYER_A] * 1000;
    score -= state->borne_off[PLAYER_B] * 1000;

    return score;
}

double expectiminimax(GameState *state, int depth, uint8_t current_player) {
    if (depth == 0 || state->borne_off[PLAYER_A] == 15 || state->borne_off[PLAYER_B] == 15) {
        return evaluate_board(state);
    }

    double expected_value = 0.0;
    
    for (uint8_t d1 = 1; d1 <= 6; d1++) {
        for (uint8_t d2 = d1; d2 <= 6; d2++) {
            double prob = (d1 == d2) ? (1.0 / 36.0) : (2.0 / 36.0);
            
            MoveList list;
            state->active_player = current_player;
            get_all_legal_moves(state, d1, d2, &list);

            if (list.count == 0) {
                expected_value += prob * evaluate_board(state);
                continue;
            }

            double best_val = (current_player == PLAYER_A) ? -INF : INF;

            for (uint32_t i = 0; i < list.count; i++) {
                GameState next_state = *state;
                for (uint8_t j = 0; j < list.seqs[i].count; j++) {
                    apply_move(&next_state, list.seqs[i].moves[j]);
                }
                
                double val = expectiminimax(&next_state, depth - 1, (current_player == PLAYER_A) ? PLAYER_B : PLAYER_A);
                
                if (current_player == PLAYER_A) {
                    if (val > best_val) best_val = val;
                } else {
                    if (val < best_val) best_val = val;
                }
            }
            expected_value += prob * best_val;
        }
    }
    return expected_value;
}

void print_board(const GameState *state) {
    printf("\n--- CURRENT BOARD ---\n");
    for (int i = 0; i < BOARD_SIZE; i++) {
        if (state->board[i].owner != EMPTY || state->board[i].pinned) {
            char owner = (state->board[i].owner == PLAYER_A) ? 'A' : 'B';
            printf("Pos %02d: %c (Count: %d) %s\n", 
                   i, owner, state->board[i].count, 
                   state->board[i].pinned ? "[PINNED OPPONENT]" : "");
        }
    }
    printf("Borne off - A: %d, B: %d\n", state->borne_off[PLAYER_A], state->borne_off[PLAYER_B]);
    printf("---------------------\n");
}

int main(void) {
    GameState state;
    memset(&state, 0, sizeof(GameState));
    for (int i = 0; i < BOARD_SIZE; i++) {
        state.board[i].owner = EMPTY;
    }

    // Standard starting position
    state.board[0].owner = PLAYER_A;
    state.board[0].count = 15;
    state.board[23].owner = PLAYER_B;
    state.board[23].count = 15;
    
    char command;
    
    while (1) {
        print_board(&state);
        printf("\nCommands: [A] AI Move calculation | [M] Manual move execution | [Q] Quit\n");
        printf("Choice> ");
        if (scanf(" %c", &command) != 1) break;
        
        if (command == 'Q' || command == 'q') {
            break;
        }
        
        if (command == 'A' || command == 'a') {
            char active;
            int d1, d2;
            printf("Enter player calculating (A/B) and dice (d1 d2) (e.g., A 5 3): ");
            if (scanf(" %c %d %d", &active, &d1, &d2) != 3) continue;
            
            state.active_player = (active == 'A' || active == 'a') ? PLAYER_A : PLAYER_B;
            
            MoveList list;
            get_all_legal_moves(&state, (uint8_t)d1, (uint8_t)d2, &list);
            
            if (list.count > 0) {
                int best_index = 0;
                double best_score = (state.active_player == PLAYER_A) ? -INF : INF;

                for (uint32_t i = 0; i < list.count; i++) {
                    GameState next_state = state;
                    for (uint8_t j = 0; j < list.seqs[i].count; j++) {
                        apply_move(&next_state, list.seqs[i].moves[j]);
                    }
                    
                    double score = expectiminimax(&next_state, 1, (state.active_player == PLAYER_A) ? PLAYER_B : PLAYER_A);
                    
                    if (state.active_player == PLAYER_A) {
                        if (score > best_score) { best_score = score; best_index = i; }
                    } else {
                        if (score < best_score) { best_score = score; best_index = i; }
                    }
                }

                printf("\nBEST MOVE EXECUTED (Expected Score: %.2f) :\n", best_score);
                for (uint8_t j = 0; j < list.seqs[best_index].count; j++) {
                    printf(" -> Move %d: %d to %d\n", j+1, list.seqs[best_index].moves[j].from, list.seqs[best_index].moves[j].to);
                    apply_move(&state, list.seqs[best_index].moves[j]);
                }
            } else {
                printf("\nNo legal moves possible for this roll.\n");
            }
        } 
        else if (command == 'M' || command == 'm') {
            char active;
            int from, to;
            printf("Enter moving player (A/B) and move (from to) (e.g., B 23 18): ");
            if (scanf(" %c %d %d", &active, &from, &to) != 3) continue;
            
            state.active_player = (active == 'A' || active == 'a') ? PLAYER_A : PLAYER_B;
            Move m = {(int8_t)from, (int8_t)to};
            
            if (is_valid_move(&state, state.active_player, from, to)) {
                apply_move(&state, m);
                printf("\nManual move applied successfully.\n");
            } else {
                printf("\nERROR: Invalid move according to Plakoto rules.\n");
            }
        }
    }

    return 0;
}