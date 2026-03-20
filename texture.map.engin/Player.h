#pragma once

#include "Board.h"
#include "Block.h"
#include "FallingGroup.h"
#include "Attack.h"


class Player {
public:
    Player();
    // 盤面の位置を設定して初期化
    void Init(float startX, float startY);

    // プレイヤーごとの更新（キーボードの割り当てや、CPUかどうかの判定を受け取る）
    //void Update(int leftKey, int rightKey, int downKey, int rotateKey, bool isCPU);
    std::vector<int> Update(int leftKey, int rightKey, int downKey, int rotateKey, bool isCPU);

    void SpawnRandomBlock(bool isCPU);

    // 枠を描画するために、盤面の中央X座標を計算して返す
    float GetCenterX() const;

    void AddAttack(int attackType) { m_attackQueue.push_back(attackType); }
    //一番上のラインを越えたか（負けたか）を判定する
    bool IsGameOver() const;

    // 次の対戦のために盤面や落下ブロックをすべて初期化する
    void Reset();

    Board m_board;
    Block m_fallingBlock;
    int m_waitTimer;
    float m_baseX, m_baseY;

    FallingGroup m_fallingGroup;
    bool m_isRotatePressed; // 回転キーの1回押し判定用

    const std::vector<Attack>& GetActiveAttacks() const { return m_activeAttacks; }

private:
    bool m_isLeftPressed;
    bool m_isRightPressed;
    bool m_isGameOver;

    //FallingGroup m_fallingGroup;
    //bool m_isRotatePressed; // 回転キーの1回押し判定用

    const int FALLING_BLOCK_COUNT = 2; // ここを 3 にすれば3個になる

    // ここを 4 にすれば、1回ストレートを作るだけで相手に4回連続でストレートが降ります。
    const int ATTACK_MULTIPLIER = 3;

    // CPU（AI）用の変数
    int m_cpuTargetCol;


    //「どの列に落とすか」を考えて決定する関数
    void DecideCPUTarget(int targetType);

    std::vector<int> m_attackQueue;

    std::vector<Attack> m_activeAttacks;
};