    #include <SFML/Graphics.hpp>
    #include <iostream>
    #include <string>
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/wait.h>

    int main() {
        int toEngine[2];      // parent → engine
        int fromEngine[2];    // engine → parent

        pipe(toEngine);
        pipe(fromEngine);

        pid_t pid = fork();

        if (pid == 0) {
            // -------------------
            // Child (engine)
            // -------------------
            
            // Redirect parent → engine pipe to stdin
            dup2(toEngine[0], STDIN_FILENO);

            // Redirect stdout to parent
            dup2(fromEngine[1], STDOUT_FILENO);

            // Close unused ends
            close(toEngine[1]);
            close(fromEngine[0]);

            // Start Komodo (Linux binary!)
            execlp("./output/bin/stockfish", "stockfish", nullptr);

            perror("execlp failed");
            exit(1);
        }

        // -------------------
        // Parent (your game)
        // -------------------

        close(toEngine[0]);
        close(fromEngine[1]);

        auto send = [&](const std::string& msg) {
            write(toEngine[1], msg.c_str(), msg.size());
        };

        char buffer[4096];

        // Initialize UCI
        send("uci\n");

        // Read until engine says "uciok"
        while (true) {
            int n = read(fromEngine[0], buffer, sizeof(buffer)-1);
            if (n > 0) {
                buffer[n] = 0;
                std::cout << buffer;
                if (std::string(buffer).find("uciok") != std::string::npos)
                    break;
            }
        }

        send("isready\n");
        while (true) {
            int n = read(fromEngine[0], buffer, sizeof(buffer)-1);
            if (n > 0) {
                buffer[n] = 0;
                std::cout << buffer;
                if (std::string(buffer).find("readyok") != std::string::npos)
                    break;
            }
        }

        std::cout << "Engine ready.\n";

        std::string moves;

        while (true) {
            std::string userMove;
            std::cout << "Your move: ";
            std::cin >> userMove;

            if (userMove == "quit") {
                send("quit\n");
                break;
            }

            moves += " " + userMove;

            send("position startpos moves" + moves + "\n");
            send("go depth 10\n");

            // Wait for bestmove
            while (true) {
                int n = read(fromEngine[0], buffer, sizeof(buffer)-1);
                if (n > 0) {
                    buffer[n] = 0;
                    std::cout << buffer;
                    std::string out = buffer;
                    if (out.find("bestmove") != std::string::npos)
                        break;
                }
            }
            std::cout << moves << std::endl;
        }

        close(toEngine[1]);
        close(fromEngine[0]);
        wait(nullptr);
    }
