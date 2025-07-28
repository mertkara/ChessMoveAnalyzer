#pragma once
#include <string>
#include <vector>

// Move: UCI formatını döndürür
struct Move {
    std::string from;
    std::string to;
    char promotion = 0;

    std::string uci() const;
};

class Board {
public:
    Board();
    void reset();

    // PGN (SAN) formatındaki hamleyi parse edip UCI formatında döndürür
    Move parse_san(const std::string& san);

    // Hamle listesini komple "e2e4 e7e5 ..." şeklinde döndürür
    std::string moves_to_uci_string(const std::vector<std::string>& pgn_moves);

private:
    char board[8][8];
    bool whiteToMove;
    bool isCapture;
    int epFile;

    char get(int rank, int file) const;
    void set(int rank, int file, char piece);

    static int file_to_idx(char f);
    static int rank_to_idx(char r);
    std::string idx_to_square(int rank, int file) const;
    Move handle_promotion(char piece);
    Move handle_castling(bool kingside);
    bool find_piece(char piece, int toRank, int toFile, int& fromRank, int& fromFile);
    bool can_reach(char piece, int fr, int ff, int tr, int tf);
    bool isBlocked(int fr, int ff, int tr, int tf);
    bool isHardPinned(char piece, int fr, int ff, int tr, int tf);


};
