// FallingGroup.cpp
#include "FallingGroup.h"

FallingGroup::FallingGroup() : m_isActive(false), m_x(0.0f), m_y(0.0f), m_blockCount(0) {}

void FallingGroup::Spawn(int count, const std::vector<int>& types, float startX, float startY, float blockRadius) {
    m_blockCount = count;
    m_types = types;
    m_x = startX;
    m_y = startY;
    m_isActive = true;

    m_offsetX.clear();
    m_offsetY.clear();

    for (int i = 0; i < count; ++i) {
        // 0番目を一番下とし、1番目、2番目はその上に積み重なるようにズラす
        m_offsetX.push_back(0.0f);
        m_offsetY.push_back(i * blockRadius * 2.0f);
    }
}

float FallingGroup::GetBlockX(int index) const {
    return m_x + m_offsetX[index];
}

float FallingGroup::GetBlockY(int index) const {
    return m_y + m_offsetY[index];
}

void FallingGroup::RotateClockwise() {
    // 全てのボールのズレ(Offset)を計算し直して、時計回りに90度回転させる
    for (int i = 0; i < m_blockCount; ++i) {
        float tempX = m_offsetX[i];
        float tempY = m_offsetY[i];
        // 2D回転の公式 (x', y') = (y, -x)
        m_offsetX[i] = tempY;
        m_offsetY[i] = -tempX;
    }
}

// FallingGroup.cpp に追加
void FallingGroup::SpawnCustom(int count, const std::vector<int>& types, const std::vector<float>& offX, const std::vector<float>& offY, float startX, float startY) {
    m_blockCount = count;
    m_types = types;
    m_offsetX = offX;
    m_offsetY = offY;
    m_x = startX;
    m_y = startY;
    m_isActive = true;
}
