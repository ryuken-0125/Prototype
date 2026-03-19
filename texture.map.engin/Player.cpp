#include "Player.h"
#include <random>
#include <windows.h>

Player::Player() : m_waitTimer(0), m_baseX(0.0f), m_baseY(0.0f), m_isLeftPressed(false), m_isRightPressed(false), m_cpuTargetCol(0) {}

void Player::Init(float startX, float startY) {
    m_baseX = startX;
    m_baseY = startY;
    m_board.Init(startX, startY);
    m_waitTimer = 0;
}

float Player::GetCenterX() const {
    // 盤面の横幅の半分を足して、だいたいの中央座標を返す
    return m_baseX + ((BOARD_WIDTH - 1) * BLOCK_RADIUS);
}

void Player::SpawnRandomBlock(bool isCPU) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> typeDist(0, 3);
    std::uniform_int_distribution<int> colDist(0, BOARD_WIDTH - 1);

    int randomType = typeDist(gen);
    int randomCol = colDist(gen);

    // 出現位置
    float startX = m_board.GetX(randomCol, BOARD_HEIGHT - 1);
    float startY = m_board.GetY(BOARD_HEIGHT - 1) + 2.0f;

    m_fallingBlock.Spawn(randomType, startX, startY);

    // ★CPUの場合、「どの列に落とすか」を出現時に決めて記憶しておく（AIの思考）
    if (isCPU) {

        DecideCPUTarget(randomType);
    }
}

void Player::Update(int leftKey, int rightKey, int downKey, bool isCPU) {
    if (m_waitTimer > 0) {
        m_waitTimer--;
        return;
    }
    if (m_board.ApplyGravity()) {
        m_waitTimer = 8;
        return;
    }
    if (m_board.CheckAndErase()) {
        m_waitTimer = 20;
        return;
    }

    if (m_fallingBlock.IsActive()) {
        float speed = 0.01f;

        // ★人間か、CPUかで操作を分ける
        if (!isCPU) {
            // 人間のキーボード操作
            if (GetAsyncKeyState(leftKey) & 0x8000) {
                if (!m_isLeftPressed) { m_fallingBlock.SetX(m_fallingBlock.GetX() - BLOCK_RADIUS * 1.0f); m_isLeftPressed = true; }
            }
            else { m_isLeftPressed = false; }

            if (GetAsyncKeyState(rightKey) & 0x8000) {
                if (!m_isRightPressed) { m_fallingBlock.SetX(m_fallingBlock.GetX() + BLOCK_RADIUS * 1.0f); m_isRightPressed = true; }
            }
            else { m_isRightPressed = false; }

            if (GetAsyncKeyState(downKey) & 0x8000) speed = 0.05f;

        }
        else {
            // ★CPU（AI）の自動操作
            float targetX = m_board.GetX(m_cpuTargetCol, BOARD_HEIGHT - 1);
            // ターゲットのX座標に向けて自動で移動させる
            if (m_fallingBlock.GetX() < targetX - 0.1f) {
                m_fallingBlock.SetX(m_fallingBlock.GetX() + BLOCK_RADIUS * 1.0f);
            }
            else if (m_fallingBlock.GetX() > targetX + 0.1f) {
                m_fallingBlock.SetX(m_fallingBlock.GetX() - BLOCK_RADIUS * 1.0f);
            }
            else {
                // 目的の列に着いたら、高速落下する（下キー長押しと同じ）
                speed = 0.05f;
            }
        }

        // はみ出し防止と落下処理
        if (m_fallingBlock.GetX() < m_baseX) m_fallingBlock.SetX(m_baseX);
        if (m_fallingBlock.GetX() > m_baseX + (BOARD_WIDTH - 1) * (BLOCK_RADIUS * 2.0f)) m_fallingBlock.SetX(m_baseX + (BOARD_WIDTH - 1) * (BLOCK_RADIUS * 1.0f));

        float nextY = m_fallingBlock.GetY() - speed;

        if (!m_board.IsCollision(m_fallingBlock.GetX(), nextY)) {
            m_fallingBlock.SetY(nextY);
        }
        else {
            m_board.LockBlock(m_fallingBlock.GetX(), m_fallingBlock.GetY(), m_fallingBlock.GetType());
            m_fallingBlock.SetInactive();
            m_waitTimer = 5;
        }
    }
    else {
        SpawnRandomBlock(isCPU);
    }
}


void Player::DecideCPUTarget(int targetType) {
    int bestCol = 0;
    int maxScore = -9999; // 最初はあり得ない低い点数にしておく

    // 0列目から8列目まで、すべての列をシミュレーション（採点）する
    for (int c = 0; c < BOARD_WIDTH; ++c) {
        int score = 0;

        // この列で「一番上にあるブロック」が何段目(row)にあるかを探す
        int topRow = -1;
        for (int r = BOARD_HEIGHT - 1; r >= 0; --r) {
            if (m_board.GetBlockType(c, r) != -1) {
                topRow = r;
                break; // 見つけたらループを抜ける
            }
        }

        // --- ここから AI の「採点（評価）」 ---

        // 1. 高さのペナルティ（ゲームオーバーを避けるため、高い場所には落としたくない）
        if (topRow >= BOARD_HEIGHT - 2) {
            score -= 1000; // 限界ギリギリの列には絶対落とさないように大減点
        }
        else {
            score -= topRow; // 高ければ高いほど少しマイナスにする（低い谷間を好むようになる）
        }

        // 2. 色のボーナス（落とした時に、周りに同じ色があるか？）
        // 着地するであろう場所(topRowの1つ上)の周りをチェックします

        // 真下のブロックが同じ色か？
        if (topRow >= 0 && m_board.GetBlockType(c, topRow) == targetType) {
            score += 50; // 同じ色の上に乗るなら超高得点！
        }

        // 左側のブロックが同じ色か？
        if (c > 0) {
            if (topRow >= 0 && m_board.GetBlockType(c - 1, topRow) == targetType) score += 20;
            if (topRow + 1 < BOARD_HEIGHT && m_board.GetBlockType(c - 1, topRow + 1) == targetType) score += 20;
        }

        // 右側のブロックが同じ色か？
        if (c < BOARD_WIDTH - 1) {
            if (topRow >= 0 && m_board.GetBlockType(c + 1, topRow) == targetType) score += 20;
            if (topRow + 1 < BOARD_HEIGHT && m_board.GetBlockType(c + 1, topRow + 1) == targetType) score += 20;
        }

        // --- 採点終了 ---

        // この列の点数が、今までの最高得点を超えていたら、そこをターゲットの候補にする
        if (score > maxScore) {
            maxScore = score;
            bestCol = c;
        }
    }

    // 全ての列をチェックし終わった後、一番点数の高かった列を最終決定とする
    m_cpuTargetCol = bestCol;
}