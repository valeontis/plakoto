# Plakoto AI Engine

A command-line engine for the game of Plakoto (a Backgammon variant), written entirely in C. The program maintains a continuous game state and calculates optimal move sequences using the Expectiminimax algorithm. 

## Features

*   **Rule Implementation:** Fully supports Plakoto-specific mechanics, including pinning opponent pieces, the "Mana" rule (immediate penalty if the starting piece is pinned), blockades, and bearing off.
*   **Move Generation:** Exhaustive legal move generation for standard dice rolls and doubles.
*   **Search Algorithm:** Expectiminimax search to handle the stochastic nature of dice rolls.
*   **Heuristic Evaluation:** Static board evaluation prioritising pip count differentials, pinning advantages, and structural blockades.
*   **Interactive CLI:** A continuous state loop allowing manual move input and AI move calculation.
*   **Zero External Dependencies:** Built using only the standard C library.

## Compilation

The engine requires maximum compiler optimization to run the Expectiminimax search efficiently. Compile using GCC:

```bash
gcc -O3 -Wall plakoto_ai.c -o plakoto_ai
```

## Usage

Run the compiled executable from the terminal:

*   **Linux / macOS:** `./plakoto_ai`
*   **Windows:** `plakoto_ai.exe`

### Command-Line Interface

Upon execution, the engine displays the initial board state and waits for user input. The interactive loop supports the following commands:

*   `A` **(AI Move):** Calculates and executes the optimal move.
    *   *Input format:* `<Player A/B> <Die 1> <Die 2>`
    *   *Example:* `A 6 4`
*   `M` **(Manual Move):** Applies a specific move to update the internal board state.
    *   *Input format:* `<Player A/B> <From> <To>`
    *   *Example:* `B 23 18`
*   `Q` **(Quit):** Exits the program.

### Board Representation

The board is represented as an array of 24 points (0 to 23).
*   **Player A** moves from 0 towards 23.
*   **Player B** moves from 23 towards 0.
*   Pinned pieces are indicated by `[PINNED OPPONENT]` in the console output.

## Algorithm Details

### State Evaluation

The heuristic function evaluates a given board state by calculating a composite score based on:
1.  **Mana Rule:** ±500,000 points if the starting point (0 for A, 23 for B) is pinned.
2.  **Bearing Off:** Large bonuses for pieces successfully removed from the board.
3.  **Pinning:** Significant bonuses for trapping opponent pieces and penalties for being trapped.
4.  **Pip Count:** The total distance remaining for all pieces. The engine seeks to minimise its own pips while maximising the opponent's.

### Search Depth

The default implementation evaluates at depth 1 (calculating the current player's immediate moves and the opponent's expected responses based on all 21 possible dice combinations). Increasing the depth inside the `expectiminimax` call improves tactical accuracy but exponentially increases computation time.
