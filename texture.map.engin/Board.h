// Board.h


/*
#pragma once

#include <vector>
#include <utility>

const int BOARD_WIDTH = 9;  // 横のマス目の数
const int BOARD_HEIGHT = 10; // 縦のマス目の数
const float BLOCK_RADIUS = 0.3f; // ブロックの半径（モデルのスケールに合わせて調整）
const float BASE_X = -2.4f;      // 0列目のX座標（左端）
const float BASE_Y = 0.3f;      // 0行目のY座標（一番下）

class Board {
public:
    Board();
    void Clear();

    // 座標計算ヘルパー
    int GetColFromX(float x) const;
    int GetRowFromY(float y) const;
    float GetXFromCol(int col) const;
    float GetYFromRow(int row) const;
    
    // 互い違い構造のX, Y座標を計算する
    float GetX(int col, int row) const;
    float GetY(int row) const;

    // 落下先が他のボールや床にぶつかるか（距離で判定）
    bool IsCollision(float x, float nextY) const;

    // DFSを使って隣接する同色ボールを数え、一定数以上なら消去する
    bool CheckAndErase();
    // 宙に浮いているボールを下の空きマスへ滑り落とす
    bool ApplyGravity();

    // ブロックを盤面に固定して記憶する
    void LockBlock(float x, float y, int type);

    // そのマスにあるブロックの種類を取得（-1なら空きマス）
    int GetBlockType(int col, int row) const;

private:
    int m_grid[BOARD_HEIGHT][BOARD_WIDTH]; // 盤面の状態を記憶する2次元配列
    // 6方向の隣接マスの座標を取得する
    void GetNeighbors(int c, int r, std::vector<std::pair<int, int>>& neighbors) const;
    // 深さ優先探索（DFS）の本体
    void DFS(int c, int r, int type, std::vector<std::vector<bool>>& visited, std::vector<std::pair<int, int>>& connected) const;
};

*/



#pragma once
#include <vector>
#include <utility>

const int BOARD_WIDTH = 9;   // 横のマス目の数
const int BOARD_HEIGHT = 10; // 縦のマス目の数
const float BLOCK_RADIUS = 0.3f; // ブロックの半径

class Board {
public:
    Board();
    //初期化時に盤面の位置を自由に決められるようにする
    void Init(float baseX, float baseY);
    void Clear();

    float GetX(int col, int row) const;
    float GetY(int row) const;
    bool IsCollision(float x, float nextY) const;
    bool CheckAndErase();
    bool ApplyGravity();
    void LockBlock(float x, float y, int type);
    int GetBlockType(int col, int row) const;

private:
    float m_baseX; //この盤面の左下のX座標
    float m_baseY; //この盤面の左下のY座標
    int m_grid[BOARD_HEIGHT][BOARD_WIDTH];

    void GetNeighbors(int c, int r, std::vector<std::pair<int, int>>& neighbors) const;
    void DFS(int c, int r, int type, std::vector<std::vector<bool>>& visited, std::vector<std::pair<int, int>>& connected) const;
};