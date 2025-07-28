#pragma once
#include <string>
#include <windows.h>
#include <vector>

struct MoveEvaluation {
    std::string move_str;          // Değerlendirilen hamle
    int eval_cp;                   // Centipawn değeri
    std::string fen_after;         // Hamleden sonraki FEN
    std::string best_move;         // Stockfish'in önerdiği en iyi hamle
    std::string ponder_move;       // Opsiyonel ponder hamlesi
};

class StockfishEngine {
public:
    StockfishEngine(const std::string& engine_path);
    bool start();
    void stop();
    void send_command(const std::string& cmd);
    std::string read_output(MoveEvaluation* eval = nullptr);
    MoveEvaluation evaluate_move(const std::string& fen, std::string move);

private:
    std::string engine_path;
    HANDLE hChildStdinWrite = NULL;
    HANDLE hChildStdoutRead = NULL;
    PROCESS_INFORMATION pi{};
};
