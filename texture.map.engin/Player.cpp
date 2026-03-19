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
        // 今は完全にランダムな列を目標にする（後でここを「同じ色の場所を探す」など賢くできます）
        m_cpuTargetCol = colDist(gen);
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
                if (!m_isLeftPressed) { m_fallingBlock.SetX(m_fallingBlock.GetX() - BLOCK_RADIUS * 2.0f); m_isLeftPressed = true; }
            }
            else { m_isLeftPressed = false; }

            if (GetAsyncKeyState(rightKey) & 0x8000) {
                if (!m_isRightPressed) { m_fallingBlock.SetX(m_fallingBlock.GetX() + BLOCK_RADIUS * 2.0f); m_isRightPressed = true; }
            }
            else { m_isRightPressed = false; }

            if (GetAsyncKeyState(downKey) & 0x8000) speed = 0.05f;

        }
        else {
            // ★CPU（AI）の自動操作
            float targetX = m_board.GetX(m_cpuTargetCol, BOARD_HEIGHT - 1);
            // ターゲットのX座標に向けて自動で移動させる
            if (m_fallingBlock.GetX() < targetX - 0.1f) {
                m_fallingBlock.SetX(m_fallingBlock.GetX() + BLOCK_RADIUS * 2.0f);
            }
            else if (m_fallingBlock.GetX() > targetX + 0.1f) {
                m_fallingBlock.SetX(m_fallingBlock.GetX() - BLOCK_RADIUS * 2.0f);
            }
            else {
                // 目的の列に着いたら、高速落下する（下キー長押しと同じ）
                speed = 0.05f;
            }
        }

        // はみ出し防止と落下処理
        if (m_fallingBlock.GetX() < m_baseX) m_fallingBlock.SetX(m_baseX);
        if (m_fallingBlock.GetX() > m_baseX + (BOARD_WIDTH - 1) * (BLOCK_RADIUS * 2.0f)) m_fallingBlock.SetX(m_baseX + (BOARD_WIDTH - 1) * (BLOCK_RADIUS * 2.0f));

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