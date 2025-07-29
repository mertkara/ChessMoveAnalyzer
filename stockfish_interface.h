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
    int mate_in;
};

class StockfishEngine {
public:
    StockfishEngine(const std::string& engine_path);
    bool start();
    void stop();
    void send_command(const std::string& cmd);
    void read_uci_output(MoveEvaluation* eval = nullptr);
    MoveEvaluation evaluate_move(const std::string& fen, std::string moves);

private:
    std::string output_buffer_; // Buffer for incomplete lines from stdout
    std::string engine_path;
    HANDLE hChildStdinWrite = NULL;
    HANDLE hChildStdoutRead = NULL;
    PROCESS_INFORMATION pi{};
};
