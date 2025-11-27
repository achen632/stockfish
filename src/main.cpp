#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sstream>
#include <algorithm>
#include <array>
#include <SFML/Graphics.hpp>

std::array<std::array<char,8>,8> board =
    {{
        {{'r','n','b','q','k','b','n','r'}},
        {{'p','p','p','p','p','p','p','p'}},
        {{' ',' ',' ',' ',' ',' ',' ',' '}},
        {{' ',' ',' ',' ',' ',' ',' ',' '}},
        {{' ',' ',' ',' ',' ',' ',' ',' '}},
        {{' ',' ',' ',' ',' ',' ',' ',' '}},
        {{'P','P','P','P','P','P','P','P'}},
        {{'R','N','B','Q','K','B','N','R'}}
    }};

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
// Display ASCII board (array-based)
// ------------------------
void printBoard(const std::array<std::array<char,8>,8>& board) {
    std::cout << "\n  +-----------------+\n";
    for (int r = 0; r < 8; r++) {  // start from bottom
        std::cout << 8 - r << " | ";  // display rank from 8 down to 1
        for (int c = 0; c < 8; c++) {
            char p = board[r][c];
            if (p == ' ') p = '.';
            std::cout << p << " ";
        }
        std::cout << "|\n";
    }
    std::cout << "  +-----------------+\n";
    std::cout << "    a b c d e f g h\n\n";
}

// ------------------------
// Apply a move
// ------------------------
void applyMove(std::array<std::array<char,8>,8>& board, const std::string& mv) {
    int c1 = mv[0] - 'a';
    int r1 = 7 - (mv[1] - '1');  // flip row
    int c2 = mv[2] - 'a';
    int r2 = 7 - (mv[3] - '1');  // flip row

    char piece = board[r1][c1];
    board[r1][c1] = ' ';
    board[r2][c2] = piece;
}

std::vector<std::string> getLegalMoves(int fd, int toEngineFd, const std::string& moves) {
    auto send = [&](const std::string& s) {
        write(toEngineFd, s.c_str(), s.size());
    };

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

    std::stringstream ss(out);
    std::string token;
    while (ss >> token) {
        if (token.size() >= 4 && token[2] >= 'a' && token[2] <= 'h')
            legal.push_back(token.substr(0,4));
    }

    return legal;
}

char mapPieceToFont(char piece)
{
    switch(piece)
    {
        // White pieces
        case 'B': return 'n'; // Bishop
        case 'N': return 'j'; // Knight
        case 'K': return 'l'; // King
        case 'Q': return 'w'; // Queen
        case 'R': return 't'; // Rook
        case 'P': return 'o'; // Pawn

        // Black pieces
        case 'b': return 'n'; // Bishop
        case 'n': return 'j'; // Knight
        case 'k': return 'l'; // King
        case 'q': return 'w'; // Queen
        case 'r': return 't'; // Rook
        case 'p': return 'o'; // Pawn

        // Other
        case ' ': return ' '; 
        default: return ' ';
    }
}

void displayBoard(sf::RenderWindow &window, float tileSize, sf::Font &font, const std::array<std::array<char,8>,8>& board)
{
    // Colors
    sf::Color lightColor(200, 180, 140); // darker beige/light brown
    sf::Color darkColor(120, 80, 50);    // darker brown

    window.clear();

    // Draw board and pieces
    for (int r = 0; r < 8; r++)
    {
        for (int c = 0; c < 8; c++)
        {

            // --- Draw square ---
            sf::RectangleShape sq(sf::Vector2f(tileSize, tileSize));
            sq.setPosition(c * tileSize, r * tileSize);

            if ((r + c) % 2 == 0)
                sq.setFillColor(lightColor);
            else
                sq.setFillColor(darkColor);

            window.draw(sq);

            // --- Draw piece ---
            char p = board[r][c];
            if (p != ' ')
            {
                sf::Text piece;
                piece.setFont(font);

                piece.setString(std::string(1, mapPieceToFont(p)));
                piece.setCharacterSize(tileSize * 0.9f);

                // Center the piece in the square
                sf::FloatRect b = piece.getLocalBounds();
                piece.setOrigin(b.left + b.width/2, b.top + b.height/2);
                piece.setPosition(
                    c * tileSize + tileSize/2,
                    r * tileSize + tileSize/2
                );

                piece.setFillColor(
                    isupper(p) ? sf::Color::White : sf::Color::Black
                );

                window.draw(piece);
            }
        }
    }

    window.display();
}

// ------------------------
// Main Program
// ------------------------
int main() {
    std::cout << "Starting up...\n";
    // load game window
    const float tileSize = 80.f;
    sf::RenderWindow window(
        sf::VideoMode(8 * tileSize, 8 * tileSize),
        "SFML Chess Board"
    );

    // load chess font
    sf::Font font;
    if (!font.loadFromFile("../src/chess.ttf"))
    {
        std::cerr << "Failed to load font\n";
    }
    
    // Pipes
    int toEngine[2];
    int fromEngine[2];
    pipe(toEngine);
    pipe(fromEngine);

    pid_t pid = fork();
    if (pid == 0) {
        dup2(toEngine[0], STDIN_FILENO);
        dup2(fromEngine[1], STDOUT_FILENO);
        close(toEngine[1]);
        close(fromEngine[0]);

        execlp("../src/stockfish", "stockfish", nullptr);
        perror("execlp failed");
        exit(1);
    }

    close(toEngine[0]);
    close(fromEngine[1]);

    auto send = [&](const std::string& s) {
        write(toEngine[1], s.c_str(), s.size());
    };

    send("uci\n");
    readUntil(fromEngine[0], "uciok");
    send("setoption name Skill Level value 5\n");
    send("isready\n");
    readUntil(fromEngine[0], "readyok");


    std::string moves;

    while (true) {
        printBoard(board);
        displayBoard(window, tileSize, font, board);

        auto legalMoves = getLegalMoves(fromEngine[0], toEngine[1], moves);
        std::string userMove;
        while (true) {
            std::cout << "Your move: ";
            std::cin >> userMove;

            if (userMove == "quit") {
                send("quit\n");
                return 0;
            }

            if (std::find(legalMoves.begin(), legalMoves.end(), userMove) != legalMoves.end())
                break;

            std::cout << "Illegal move. Try again.\n";
        }

        applyMove(board, userMove);
        moves += " " + userMove;

        
        printBoard(board);
        displayBoard(window, tileSize, font, board);
        sleep(1);

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