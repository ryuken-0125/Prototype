// Board.h



#pragma once
#include <vector>
#include <utility>

const int BOARD_WIDTH = 9;   // 横のマス目の数
const int BOARD_HEIGHT = 11; // 縦のマス目の数
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
   // bool CheckAndErase();
    bool ApplyGravity();
    void LockBlock(float x, float y, int type);
    int GetBlockType(int col, int row) const;

    std::vector<int> CheckAndErase();

private:
    float m_baseX; //この盤面の左下のX座標
    float m_baseY; //この盤面の左下のY座標
    int m_grid[BOARD_HEIGHT][BOARD_WIDTH];

    bool GetHexNeighbor(int c, int r, int dir, int& nc, int& nr) const;

    void GetNeighbors(int c, int r, std::vector<std::pair<int, int>>& neighbors) const;
    void DFS(int c, int r, int type, std::vector<std::vector<bool>>& visited, std::vector<std::pair<int, int>>& connected) const;
};