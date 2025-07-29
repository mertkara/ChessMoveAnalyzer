#include "stockfish_interface.h"
#include <iostream>
#include <sstream>

StockfishEngine::StockfishEngine(const std::string& engine_path)
    : engine_path(engine_path) {}

bool StockfishEngine::start() {
    SECURITY_ATTRIBUTES sa{ sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };

    HANDLE hStdoutWrite, hStdinRead;

    // stdout yönlendirmesi
    if (!CreatePipe(&hChildStdoutRead, &hStdoutWrite, &sa, 0))
        return false;
    SetHandleInformation(hChildStdoutRead, HANDLE_FLAG_INHERIT, 0);

    // stdin yönlendirmesi
    if (!CreatePipe(&hStdinRead, &hChildStdinWrite, &sa, 0))
        return false;
    SetHandleInformation(hChildStdinWrite, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFO si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hStdoutWrite;
    si.hStdError  = hStdoutWrite;
    si.hStdInput  = hStdinRead;

    std::string cmd = engine_path;
    if (!CreateProcessA(NULL, cmd.data(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
        return false;

    // Parent process'te kullanılmayan handle'ları kapat
    CloseHandle(hStdoutWrite);
    CloseHandle(hStdinRead);

    // Stockfish'e UCI başlatma komutu
    send_command("uci");
    return true;
}

void StockfishEngine::stop() {
    send_command("quit");
    WaitForSingleObject(pi.hProcess, INFINITE);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hChildStdinWrite);
    CloseHandle(hChildStdoutRead);
}

void StockfishEngine::send_command(const std::string& cmd) {
    std::string c = cmd + "\n";
    DWORD written;
    WriteFile(hChildStdinWrite, c.c_str(), c.size(), &written, NULL);
    //std::cout << "Sent command: " << c  << std::endl;
}

void StockfishEngine::read_uci_output(MoveEvaluation* eval) {
    char buffer[4096];
    DWORD bytesRead;

    while (true) {
        if (!ReadFile(hChildStdoutRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) || bytesRead == 0) {
            std::cerr << "Stockfish output read error!" << std::endl;
            return;
        }
        buffer[bytesRead] = '\0';
        output_buffer_ += buffer; // Add new data to our persistent buffer

        size_t newline_pos;
        // Process all complete lines in the buffer
        while ((newline_pos = output_buffer_.find('\n')) != std::string::npos) {
            std::string line = output_buffer_.substr(0, newline_pos);
            output_buffer_.erase(0, newline_pos + 1); // Remove the processed line

            // Trim potential '\r' from the end of the line
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }

            // 1. Check for evaluation info
            size_t scorePos = line.find("score cp ");
            if (scorePos != std::string::npos) {
                eval->mate_in = 0; // Reset mate score if we get a cp score
                eval->eval_cp = std::stoi(line.substr(scorePos + 9));
            }

            // 2. Check for mate info
            size_t matePos = line.find("score mate ");
            if (matePos != std::string::npos) {
                eval->mate_in = std::stoi(line.substr(matePos + 11));
            }

            // 3. Check for the final bestmove command
            if (line.rfind("bestmove", 0) == 0) { // Check if line starts with "bestmove"
                std::stringstream ss(line);
                std::string command;
                ss >> command; // "bestmove"
                ss >> eval->best_move;

                std::string ponder_token;
                if (ss >> ponder_token && ponder_token == "ponder") {
                    ss >> eval->ponder_move;
                }
                return; // We are done, exit the function
            }
        }
    }
}

MoveEvaluation StockfishEngine::evaluate_move(const std::string& fen, std::string moves) {
    MoveEvaluation eval;
    eval.move_str = moves;
    eval.eval_cp = 0;
    eval.mate_in = 0;

    // Send the position and start the calculation
    send_command("position startpos moves" + moves);
    send_command("go depth 9");

    // Read and parse output until 'bestmove' is found
    read_uci_output(&eval);

    eval.fen_after = fen; // This is still not updated, as in your original code
    return eval;
}
