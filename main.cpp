#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cctype>
#include "stockfish_interface.h"
#include "pgn_to_uci.h"

// ---- PGN Parser ----
std::vector<std::string> parse_pgn(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "PGN dosyası açılamadı: " << filename << std::endl;
        return {};
    }

    std::string line, moves_combined;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '[') continue;
        moves_combined += " " + line;
    }
    file.close();

    std::cout << moves_combined << std::endl;
    std::vector<std::string> moves;
    std::istringstream iss(moves_combined);
    std::string token;
    int count = 0;
    while (iss >> token) {
        if (std::isdigit(token[0]) && token.find('.') != std::string::npos)
            continue;
        if (token == "1-0" || token == "0-1" || token == "1/2-1/2")
            continue;

        count++;
        std::cout << count << " : " << token << std::endl;
        moves.push_back(token);
    }

    return moves;
}

int main(int argc, char** argv) {
    if(argc < 2)
    {     
        std::cout << "Missing PGN filename..." << std::endl;
        return -1;
    }
    std::cout << "Reading PGN file..." << std::endl;
    auto moves = parse_pgn(argv[1]);
    Board board;

    /*for (const auto& move : moves) {
        Move m = board.parse_san(move);
        std::cout << "PGN: " << move << " - UCI: " << m.uci() << std::endl;
    }*/
    std::cout << "Starting engine..." << std::endl;

    StockfishEngine engine("stockfish-windows-x86-64-avx2.exe");
    if (!engine.start()) 
    {
        std::cout << "Couldn't start engine..." << std::endl;
        return 1;
    }

    std::string current_fen = "startpos";
    std::string movesStr = "";

    for (const auto& move : moves) {
        Move m = board.parse_san(move);
        movesStr += " " + m.uci();
        
        auto result = engine.evaluate_move(current_fen, movesStr);
        std::cout << m.uci() 
                << " eval: " << result.eval_cp 
                << " mate_in :" << result.mate_in
                << " BestMove : " << result.best_move 
                << " Ponder Move: " << result.ponder_move
                << std::endl;
    }

    engine.stop();
}