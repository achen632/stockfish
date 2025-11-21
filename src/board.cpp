#include <SFML/Graphics.hpp>
#include <array>
#include <string>
#include <iostream>

std::array<std::array<char, 8>, 8> board =
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

void displayBoard(sf::RenderWindow &window, float tileSize, sf::Font &font)
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

int main()
{
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

    displayBoard(window, tileSize, font);
    
    /*
    Main Loop
    */
    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        // get user input
        // make user move
        // display board
        // get computer move
        // make computer move
        // wait 1s
        // display board
    }
    return 0;
}
