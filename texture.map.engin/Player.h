#pragma once
#include "Board.h"
#include "Block.h"

class Player {
public:
    Player();
    // 盤面の位置を設定して初期化
    void Init(float startX, float startY);

    // プレイヤーごとの更新（キーボードの割り当てや、CPUかどうかの判定を受け取る）
    void Update(int leftKey, int rightKey, int downKey, bool isCPU);

    void SpawnRandomBlock(bool isCPU);

    // 枠を描画するために、盤面の中央X座標を計算して返す
    float GetCenterX() const;

    Board m_board;
    Block m_fallingBlock;
    int m_waitTimer;
    float m_baseX, m_baseY;

private:
    bool m_isLeftPressed;
    bool m_isRightPressed;

    // CPU（AI）用の変数
    int m_cpuTargetCol;
};