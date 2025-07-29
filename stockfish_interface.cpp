#include "stockfish_interface.h"
#include <iostream>

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

std::string StockfishEngine::read_output(MoveEvaluation* eval) {
    char buffer[4096];
    DWORD bytesRead;
    std::string output;

    while (true) {
        if (!ReadFile(hChildStdoutRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) || bytesRead == 0){
            std::cout << "Stockfish output read error!" << std::endl;
            break;
        }
        if (bytesRead == 0) break; // EOF
        buffer[bytesRead] = '\0';
        output += buffer;

        // bestmove satırı bulunursa eval bilgilerini çekelim
        size_t pos = output.find("bestmove");
        if (pos != std::string::npos) {
            if (eval) {
                // bestmove <move> [ponder <move>]
                std::string line = output.substr(pos);
                size_t end = line.find("\n");
                if (end != std::string::npos) line = line.substr(0, end);

                // parçala
                size_t space1 = line.find(" ");
                if (space1 != std::string::npos) {
                    size_t space2 = line.find(" ", space1 + 1);
                    if (space2 == std::string::npos) {
                        //eval->best_move = line.substr(space1+1);
                        
                        //eval->best_move = line.substr(space1 + 1); 
                        //std::cout << "DEBUG"; //<< line ; //<<", space 1: " << space1 << std::endl;
                        return output; 
                        break;
                    }
                    eval->best_move = line.substr(space1 + 1, space2 - (space1 + 1));

                    size_t ponderPos = line.find("ponder");
                    if (ponderPos != std::string::npos) {
                        eval->ponder_move = line.substr(ponderPos + 7);
                    }
                }
            }
            break;
        }
    }
    return output;
}

MoveEvaluation StockfishEngine::evaluate_move(const std::string& fen, std::string move) {
    MoveEvaluation eval;
    eval.move_str = move;

    // Hamleyi uygula ve hesaplat
    send_command("position startpos moves" + move);
    send_command("go depth 9");

    std::string output = read_output(&eval);
    //std::cout << "Stockfish output:\n" << output << std::endl;

    // Eval CP değeri
    eval.eval_cp = 0;
    eval.mate_in = 0;
    size_t matePos = output.rfind("score mate ");
    size_t scorePos = output.rfind("score cp ");
    if(matePos != std::string::npos) {
        eval.mate_in = std::stoi(output.substr(matePos + 11));
    } else if (scorePos != std::string::npos) {
        eval.eval_cp = std::stoi(output.substr(scorePos + 9));
    }

    eval.fen_after = fen; // şimdilik güncellenmiyor

    return eval;
}
