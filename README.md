# ChessMoveAnalyzer

**ChessMoveAnalyzer** is a lightweight(hopefully) tool that takes a PGN file, converts and feeds the moves in UCI format into a chess engine (e.g., Stockfish), and categorizes each move (best move, blunder, mistake, inaccuracy, etc.).  
It works similarly to Chess.com's Game Review feature, allowing you to quickly spot critical mistakes in a single run.

## Features
- Parse any valid **PGN** file  
- Run engine evaluations (currently just **Stockfish** but should be easy to add others)   
- TO DO: Classify each move (blunder, mistake, inaccuracy, good, best)  
- TO DO: Output a clean summary of move quality  

## Requirements
- [Stockfish](https://stockfishchess.org/download/) engine binary (place it in the project root)  
- C++17 compiler
