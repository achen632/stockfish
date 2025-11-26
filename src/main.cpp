#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sstream>     // for std::stringstream
#include <algorithm>   // for std::find

// ------------------------
// Utility: read until keyword
// ------------------------
std::string readUntil(int fd, const std::string& keyword) {
    std::string acc;
    char buf[4096];

    while (true) {
        int n = read(fd, buf, sizeof(buf) - 1);
        if (n <= 0) continue;
        buf[n] = 0;
        acc += buf;

        if (acc.find(keyword) != std::string::npos)
            return acc;
    }
}

// ------------------------
// Parse Stockfish best move
// ------------------------
std::string parseBestMove(const std::string& out) {
    size_t pos = out.find("bestmove ");
    if (pos == std::string::npos) return "";
    pos += 9;
    return out.substr(pos, 4);  // e2e4
}

// ------------------------
// Display ASCII board
// ------------------------
void printBoard(const std::vector<std::string>& board) {
    std::cout << "\n  +-----------------+\n";
    for (int r = 7; r >= 0; r--) {
        std::cout << r + 1 << " | ";
        for (int c = 0; c < 8; c++)
            std::cout << board[r][c] << " ";
        std::cout << "|\n";
    }
    std::cout << "  +-----------------+\n";
    std::cout << "    a b c d e f g h\n\n";
}

// ------------------------
// Apply a move (very minimal, only string copy)
// ------------------------
void applyMove(std::vector<std::string>& board, const std::string& mv) {
    int c1 = mv[0] - 'a';
    int r1 = mv[1] - '1';
    int c2 = mv[2] - 'a';
    int r2 = mv[3] - '1';

    char piece = board[r1][c1];
    board[r1][c1] = '.';
    board[r2][c2] = piece;
}

std::vector<std::string> getLegalMoves(int fd, int toEngineFd, const std::string& moves) {
    auto send = [&](const std::string& s) {
        write(toEngineFd, s.c_str(), s.size());
    };

    // Query legal moves with perft
    send("position startpos moves" + moves + "\n");
    send("go perft 1\n");

    std::vector<std::string> legal;
    std::string out;
    char buf[4096];

    while (true) {
        int n = read(fd, buf, sizeof(buf) - 1);
        if (n <= 0) continue;
        buf[n] = 0;
        out += buf;

        if (out.find("Nodes searched") != std::string::npos)
            break;
    }

    // Parse moves: they appear like "e2e4: 1"
    std::stringstream ss(out);
    std::string token;
    while (ss >> token) {
        if (token.size() >= 4 && token[2] >= 'a' && token[2] <= 'h')
            legal.push_back(token.substr(0,4));
    }

    return legal;
}

// ------------------------
// Main Program
// ------------------------
int main() {
    std::cout << "Starting\n";
    // Pipes
    int toEngine[2];
    int fromEngine[2];
    pipe(toEngine);
    pipe(fromEngine);

    pid_t pid = fork();
    if (pid == 0) {
        // CHILD → Stockfish process
        dup2(toEngine[0], STDIN_FILENO);
        dup2(fromEngine[1], STDOUT_FILENO);

        close(toEngine[1]);
        close(fromEngine[0]);

        execlp("../src/stockfish", "stockfish", nullptr);
        perror("execlp failed");
        exit(1);
    }

    // PARENT
    close(toEngine[0]);
    close(fromEngine[1]);

    auto send = [&](const std::string& s) {
        write(toEngine[1], s.c_str(), s.size());
    };

    // Initialize Stockfish silently
    send("uci\n");
    readUntil(fromEngine[0], "uciok");
    send("isready\n");
    readUntil(fromEngine[0], "readyok");

    // Starting board
    std::vector<std::string> board = {
        "rnbqkbnr",
        "pppppppp",
        "........",
        "........",
        "........",
        "........",
        "PPPPPPPP",
        "RNBQKBNR"
    };

    std::string moves;

    while (true) {
        printBoard(board);

        // -----------------------
        // GET LEGAL MOVES
        // -----------------------
        auto legalMoves = getLegalMoves(fromEngine[0], toEngine[1], moves);

        std::cout << "Legal moves:";
        for (auto& m : legalMoves) std::cout << " " << m;
        std::cout << "\n";

        // -----------------------
        // USER MOVE
        // -----------------------
        std::string userMove;
        while (true) {
            std::cout << "Your move: ";
            std::cin >> userMove;

            if (userMove == "quit") {
                send("quit\n");
                return 0;
            }

            // Validate against legal moves
            if (std::find(legalMoves.begin(), legalMoves.end(), userMove) != legalMoves.end())
                break;

            std::cout << "Illegal move. Try again.\n";
        }

        // Apply move
        applyMove(board, userMove);
        moves += " " + userMove;

        // -----------------------
        // ENGINE MOVE
        // -----------------------
        send("position startpos moves" + moves + "\n");
        send("go depth 12\n");

        std::string result = readUntil(fromEngine[0], "bestmove");
        std::string engineMove = parseBestMove(result);

        std::cout << "Engine plays: " << engineMove << "\n";

        applyMove(board, engineMove);
        moves += " " + engineMove;
    }


    close(toEngine[1]);
    close(fromEngine[0]);
    wait(nullptr);

    return 0;
}
