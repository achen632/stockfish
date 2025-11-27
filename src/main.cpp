/*
Author: Andrew Chen
Class: ECE6122
Last Date Modified: 11/26/2025
Description: Chess game using Stockfish engine, SFML for graphics, and command-line for user input
*/

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

// Reads from file descriptor until keyword appears
// Input: int fd, std::string keyword
// Output: std::string containing accumulated output
std::string readUntil(int fd, const std::string& keyword)
{
    std::string acc;
    char buf[4096];

    while (true)
    {
        int n = read(fd, buf, sizeof(buf) - 1);
        if (n <= 0) continue;
        buf[n] = 0;
        acc += buf;

        if (acc.find(keyword) != std::string::npos)
            return acc;
    }
}

// Extracts best move from Stockfish
// Input: std::string output from engine
// Output: 4-character move string like "e2e4"
std::string parseBestMove(const std::string& out)
{
    size_t pos = out.find("bestmove ");
    if (pos == std::string::npos) return "";
    pos += 9;
    return out.substr(pos, 4);
}

// Prints ASCII board
// Input: board array
// Output: None (prints to stdout)
void printBoard(const std::array<std::array<char,8>,8>& board)
{
    std::cout << "\n  +-----------------+\n";
    for (int r = 0; r < 8; r++)
    {
        std::cout << 8 - r << " | ";
        for (int c = 0; c < 8; c++)
        {
            char p = board[r][c];
            if (p == ' ') p = '.';
            std::cout << p << " ";
        }
        std::cout << "|\n";
    }
    std::cout << "  +-----------------+\n";
    std::cout << "    a b c d e f g h\n\n";
}

// Applies a move to the board array
// Input: board array (by reference), move string like "e2e4"
// Output: board is updated in place
void applyMove(std::array<std::array<char,8>,8>& board, const std::string& mv)
{
    int c1 = mv[0] - 'a';
    int r1 = 7 - (mv[1] - '1');  // flip row for white at bottom
    int c2 = mv[2] - 'a';
    int r2 = 7 - (mv[3] - '1');

    char piece = board[r1][c1];
    board[r1][c1] = ' ';
    board[r2][c2] = piece;
}

// Queries Stockfish for legal moves
// Input: engine fd, write fd, moves string
// Output: vector of legal move strings
std::vector<std::string> getLegalMoves(int fd, int toEngineFd, const std::string& moves)
{
    auto send = [&](const std::string& s)
    {
        write(toEngineFd, s.c_str(), s.size());
    };

    send("position startpos moves" + moves + "\n");
    send("go perft 1\n");

    std::vector<std::string> legal;
    std::string out;
    char buf[4096];

    while (true)
    {
        int n = read(fd, buf, sizeof(buf) - 1);
        if (n <= 0) continue;
        buf[n] = 0;
        out += buf;

        if (out.find("Nodes searched") != std::string::npos)
            break;
    }

    std::stringstream ss(out);
    std::string token;
    while (ss >> token)
    {
        if (token.size() >= 4 && token[2] >= 'a' && token[2] <= 'h')
            legal.push_back(token.substr(0,4));
    }

    return legal;
}

// Maps board char to chess font char
// Input: char piece
// Output: char used in chess font
char mapPieceToFont(char piece)
{
    switch(piece)
    {
        case 'B': return 'n'; case 'N': return 'j'; case 'K': return 'l';
        case 'Q': return 'w'; case 'R': return 't'; case 'P': return 'o';
        case 'b': return 'n'; case 'n': return 'j'; case 'k': return 'l';
        case 'q': return 'w'; case 'r': return 't'; case 'p': return 'o';
        case ' ': return ' ';
        default: return ' ';
    }
}

// Renders SFML board
// Input: window, tileSize, font, board array
void displayBoard(sf::RenderWindow &window, float tileSize, sf::Font &font, const std::array<std::array<char,8>,8>& board)
{
    sf::Color lightColor(200, 180, 140);
    sf::Color darkColor(120, 80, 50);

    window.clear();

    for (int r = 0; r < 8; r++)
    {
        for (int c = 0; c < 8; c++)
        {
            sf::RectangleShape sq(sf::Vector2f(tileSize, tileSize));
            sq.setPosition(c * tileSize, r * tileSize);

            if ((r + c) % 2 == 0)
            {
                sq.setFillColor(lightColor);
            }
            else
            {
                sq.setFillColor(darkColor);
            }

            window.draw(sq);

            char p = board[r][c];
            if (p != ' ')
            {
                sf::Text piece;
                piece.setFont(font);
                piece.setString(std::string(1, mapPieceToFont(p)));
                piece.setCharacterSize(tileSize * 0.9f);

                sf::FloatRect b = piece.getLocalBounds();
                piece.setOrigin(b.left + b.width/2, b.top + b.height/2);
                piece.setPosition(c * tileSize + tileSize/2, r * tileSize + tileSize/2);

                piece.setFillColor(isupper(p) ? sf::Color::White : sf::Color::Black);
                window.draw(piece);
            }
        }
    }

    window.display();
}

// Main program loop
int main()
{
    // init window
    std::cout << "Starting up...\n";
    const float tileSize = 80.f;
    sf::RenderWindow window(sf::VideoMode(8 * tileSize, 8 * tileSize), "SFML Chess Board");

    // load font
    sf::Font font;
    if (!font.loadFromFile("../src/chess.ttf"))
    {
        std::cerr << "Failed to load font\n";
    }

    // create pipes for engine comm
    int toEngine[2]; int fromEngine[2];
    pipe(toEngine); pipe(fromEngine);
    pid_t pid = fork();
    if (pid == 0)
    {
        dup2(toEngine[0], STDIN_FILENO);
        dup2(fromEngine[1], STDOUT_FILENO);
        close(toEngine[1]);
        close(fromEngine[0]);

        execlp("../src/stockfish", "stockfish", nullptr);
        perror("execlp failed");
        exit(1);
    }

    // close unused pipe ends
    close(toEngine[0]);
    close(fromEngine[1]);
    auto send = [&](const std::string& s) { write(toEngine[1], s.c_str(), s.size()); };

    // init engine
    send("uci\n");
    readUntil(fromEngine[0], "uciok");
    send("setoption name Skill Level value 3\n");
    send("isready\n");
    readUntil(fromEngine[0], "readyok");

    // game loop
    std::string moves;
    while (true)
    {
        // show
        printBoard(board);
        displayBoard(window, tileSize, font, board);

        // get user move with validation check
        auto legalMoves = getLegalMoves(fromEngine[0], toEngine[1], moves);
        std::string userMove;
        while (true)
        {
            std::cout << "Your move: ";
            std::cin >> userMove;

            if (userMove == "quit")
            {
                send("quit\n");
                return 0;
            }

            if (std::find(legalMoves.begin(), legalMoves.end(), userMove) != legalMoves.end())
            {
                break;
            }

            std::cout << "Illegal move. Try again.\n";
        }

        // make move
        applyMove(board, userMove);
        moves += " " + userMove;

        // show
        printBoard(board);
        displayBoard(window, tileSize, font, board);
        
        // wait
        sleep(1);

        // get engine move
        send("position startpos moves" + moves + "\n");
        send("go depth 12\n");
        std::string result = readUntil(fromEngine[0], "bestmove");
        std::string engineMove = parseBestMove(result);

        // make engine move
        std::cout << "Engine plays: " << engineMove << "\n";
        applyMove(board, engineMove);
        moves += " " + engineMove;
    }

    close(toEngine[1]);
    close(fromEngine[0]);
    wait(nullptr);

    return 0;
}
