// Board.h


#pragma once
#include <vector>
#include <set>
#include <utility>

const int BOARD_WIDTH = 9;   // 横のマス目の数
const int BOARD_HEIGHT = 11; // 縦のマス目の数
const float BLOCK_RADIUS = 0.3f; // ブロックの半径

class Board {
public:
    Board();
    void Init(float baseX, float baseY);
    void Clear();
    void LockBlock(float x, float y, int type);
    bool ApplyGravity();

    std::vector<int> CheckAndErase();

    float GetX(int col, int row) const;
    float GetY(int row) const;
    bool IsCollision(float x, float nextY) const;

    int GetBlockType(int col, int row) const { return m_grid[row][col]; }

    int GetCrackedTurns(int col, int row) const { return m_crackedTurns[row][col]; }

    bool AdvanceTurnAndBreak();

private:
    float m_baseX; //この盤面の左下のX座標
    float m_baseY; //この盤面の左下のY座標
    int m_grid[BOARD_HEIGHT][BOARD_WIDTH];

    // ひび割れターン数（-1:ひび無し, 0:発生直後, 1〜4:経過ターン）
    int m_crackedTurns[BOARD_HEIGHT][BOARD_WIDTH];

    int MixColor(int brokenColor, int targetColor) const;

    bool GetHexNeighbor(int c, int r, int dir, int& nc, int& nr) const;

    void GetNeighbors(int c, int r, std::vector<std::pair<int, int>>& neighbors) const;

    void DFS(int c, int r, int targetType, std::vector<std::vector<bool>>& visited, std::vector<std::pair<int, int>>& connected) const;

};







