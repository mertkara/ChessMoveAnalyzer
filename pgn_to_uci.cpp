#include "pgn_to_uci.h"
#include <cctype>
#include <cmath>
#include <sstream>
#include <iostream>

// -------------------- Move --------------------
std::string Move::uci() const {
    std::string out = from + to;
    if (promotion) out.push_back(std::tolower(promotion));
    return out;
}

// -------------------- Board --------------------
Board::Board() { reset(); }

void Board::reset() {
    const char* start[8] = {
        "rnbqkbnr",
        "pppppppp",
        "........",
        "........",
        "........",
        "........",
        "PPPPPPPP",
        "RNBQKBNR"
    };
    for (int r = 0; r < 8; r++)
        for (int f = 0; f < 8; f++)
            board[r][f] = start[r][f];
    whiteToMove = true;
    epFile = -1;
}

char Board::get(int rank, int file) const { return board[rank][file]; }
void Board::set(int rank, int file, char piece) { board[rank][file] = piece; }

int Board::file_to_idx(char f) { return f - 'a'; }
int Board::rank_to_idx(char r) { return 8 - (r - '0'); }

std::string Board::idx_to_square(int rank, int file) const {
    std::string s;
    s += 'a' + file;
    s += '0' + (8 - rank);
    return s;
}

// -------------------- Parse SAN --------------------
Move Board::parse_san(const std::string& san) {
    // Castle
    if (san == "O-O" || san == "0-0") return handle_castling(true);
    if (san == "O-O-O" || san == "0-0-0") return handle_castling(false);
    
    isCapture = (san.find('x') != std::string::npos);

    // Parse backwarding seemed easier
    std::string santmp = san;
    if(san[san.size()-1] == '+' || san[san.size()-1] == '#') //these dont matter for uci 
    {
        santmp = santmp.substr(0, san.size() - 1);
    }

    // Check Promotion (e8=Q)
    char promotion = 0;
    if(std::isupper(santmp[santmp.size()-1]) && santmp[santmp.size()-2] == '=') 
    {     
        promotion = santmp[santmp.size()-1];
        santmp = santmp.substr(0, santmp.size() - 2);
    }

    std::string destSquare = santmp.substr(santmp.size() - 2); 
    int toRank = rank_to_idx(destSquare[1]);
    int toFile = file_to_idx(destSquare[0]);
    santmp = santmp.substr(0, santmp.size() - 2);

    // parse piece
    char piece = 'P';
    if ( !santmp.empty() && std::isupper(santmp[0]) )
    {     
        piece = santmp[0];
        santmp = santmp.substr(1);     
    }

    // parse if there is rank or file ambiguity
    int fromRank = -1, fromFile = -1;
    if( !santmp.empty() && santmp[0] != 'x' )
    {
        if( std::isdigit(santmp[0]) ) // its a rank info
            fromRank = rank_to_idx(santmp[0]);
        else
            fromFile = file_to_idx(santmp[0]);
    }

    if(!whiteToMove) 
        piece = std::tolower(piece);

    if( !find_piece(piece, toRank, toFile, fromRank, fromFile) ){
        std::cout << "PNG problem, is it a valid PNG or a bug?" << std::endl;
    }
         

    // Make the move
    Move move;
    move.from = idx_to_square(fromRank, fromFile);
    move.to = idx_to_square(toRank, toFile);
    move.promotion = promotion;

    char movingPiece = get(fromRank, fromFile);

    // en passant capture, then we dont automatically edit the captured pawns square so need to do it here
    if( std::toupper(piece) == 'P' && get(toRank,toFile) == '.' && isCapture ) 
        set(fromRank,toFile,'.');
        
    set(fromRank, fromFile, '.');
    set(toRank, toFile, promotion ? promotion : movingPiece);

    // En passant is possible in the next move, if so which file
    epFile = -1;
    if (std::toupper(piece) == 'P' && std::abs(toRank - fromRank) == 2)
        epFile = fromFile;

    whiteToMove = !whiteToMove;
    return move;
}

// -------------------- Move string builder --------------------
std::string Board::moves_to_uci_string(const std::vector<std::string>& pgn_moves) {
    std::ostringstream oss;
    for (size_t i = 0; i < pgn_moves.size(); i++) {
        Move m = parse_san(pgn_moves[i]);
        oss << m.uci();
        if (i != pgn_moves.size() - 1) oss << " ";
    }
    return oss.str();
}

// -------------------- Private helpers --------------------
Move Board::handle_castling(bool kingside) {
    Move move;
    if (whiteToMove) {
        move.from = "e1";
        move.to = kingside ? "g1" : "c1";
        set(7, 4, '.');
        set(7, kingside ? 6 : 2, 'K');
        set(7, kingside ? 5 : 3, kingside ? 'R' : 'R');
        set(7, kingside ? 7 : 0, '.');
    } else {
        move.from = "e8";
        move.to = kingside ? "g8" : "c8";
        set(0, 4, '.');
        set(0, kingside ? 6 : 2, 'k');
        set(0, kingside ? 5 : 3, kingside ? 'r' : 'r');
        set(0, kingside ? 7 : 0, '.');
    }
    whiteToMove = !whiteToMove;
    return move;
}

bool Board::find_piece(char piece, int toRank, int toFile, int& fromRank, int& fromFile) {  
    char p;
    for (int r = 0; r < 8; r++) {
        if( fromRank > 0 && r != fromRank ) continue;
        for (int f = 0; f < 8; f++) {
            if( fromFile > 0 && f != fromFile ) continue;
            p = get(r, f);
            if(p != piece) 
                continue;
            else if (!can_reach(piece, r, f, toRank, toFile)) 
                continue;                
            else
            {
                //seems like a legal move, but hard pin is possible so need to check that
                //exp: 2 knight might go to same place, one is pinned, then source rank/file is not given in pgn as it is not a legal move...
                if(isHardPinned(piece, r, f, toRank, toFile))
                    continue;
                else
                { 
                    //found the source square
                    fromRank = r;
                    fromFile = f;
                    return true;
                }
            }
        }
    }
    return false;
}

bool Board::can_reach(char piece, int fr, int ff, int tr, int tf) {
    int dr = tr - fr, df = tf - ff;
    piece = std::toupper(piece);
    switch (piece) {
    case 'P':
        if(!isCapture)
        {
            if(df != 0) 
                return false;
            else
            {
                return ( (whiteToMove ? (dr == -1) : (dr == 1) ) || 
                         (whiteToMove ? ((dr == -2) && (tr == rank_to_idx('4')) && !isBlocked(fr,ff,tr,tf)) : 
                                        ((dr == 2) && (tr == rank_to_idx('5') && !isBlocked(fr,ff,tr,tf)))) 
                        );
            }
        }
        else
        {
            return ((whiteToMove ? (dr == -1) : (dr == 1) ) && (std::abs(df) == 1)) || 
                ((epFile >= 0) && (std::abs(df) == 1) && (whiteToMove ? (fr == rank_to_idx('5')) : (fr == rank_to_idx('4')))); 
        }
    case 'N': return dr * dr + df * df == 5;
    case 'B': return (std::abs(dr) == std::abs(df)) && !isBlocked(fr,ff,tr,tf);
    case 'R': return (dr == 0 || df == 0) && !isBlocked(fr,ff,tr,tf);
    case 'Q': return (dr == 0 || df == 0 || std::abs(dr) == std::abs(df)) && !isBlocked(fr,ff,tr,tf);
    case 'K': return true; //only 1 king so assuming pgn is correct, its a legal move.
    }
    return false;
}

bool Board::isHardPinned(char piece, int fr, int ff, int tr, int tf){
    if(std::toupper(piece) == 'K') return false;
    char boardTmp[8][8]; //just to use can_reach function, editing member array board instead of tmp, and then use tmp to reverse it. 
                         //for now it was easier, would probably change it later...
    int kingRank, kingFile;
    bool pinned = false;
    for (int r = 0; r < 8; r++){
        for (int f = 0; f < 8; f++){
            boardTmp[r][f] = board[r][f];
            if(boardTmp[r][f] == 'K' && whiteToMove){
                kingRank = r;
                kingFile = f;
            }
            else if(boardTmp[r][f] == 'k' && !whiteToMove){
                kingRank = r;
                kingFile = f;
            }
        }
    }
    board[fr][ff] = '.';
    board[tr][tf] = piece;

    if( std::toupper(piece) == 'P' && boardTmp[tr][tf] == '.' && isCapture ) // en passant
        board[fr][tf] = '.';
    
    for (int r = 0; r < 8; r++){
        for (int f = 0; f < 8; f++){
            char p = board[r][f];
            if( whiteToMove && (p == 'q' || p == 'b' || p == 'r') && can_reach(p,r,f,kingRank, kingFile) )
                pinned = true;
            else if( !whiteToMove && (p == 'Q' || p == 'B' || p == 'R') && can_reach(p,r,f,kingRank, kingFile) )
                pinned = true;
            if(pinned) break;
        }
        if(pinned) break;  
    }
    std::copy(&boardTmp[0][0], &boardTmp[0][0] + 8*8, &board[0][0]);
    return pinned;
}

bool Board::isBlocked(int fr, int ff, int tr, int tf) {

    int dr = tr - fr, df = tf - ff;
    bool blocked = false;
    char p;
    if(dr == 0)
    {
        for(int f = std::min(tf,ff)+1; f < std::max(tf,ff); f++ ){
            p = get(tr,f);
            if(p != '.') return true;
        }
    }
    else if(df == 0)
    {
        for(int r = std::min(tr,fr)+1; r < std::max(tr,fr); r++ ){
            p = get(r,tf);
            if(p != '.') return true;
        }
    }
    else
    {
        int rChange = ( dr > 0 ) ? 1 : -1; 
        int fChange = ( df > 0 ) ? 1 : -1;
        for(int i = 1; i < std::abs(dr); i++){
            p = get(fr + i*rChange, ff + i*fChange);
            if(p != '.') return true;
        }
    }
    return false;

}