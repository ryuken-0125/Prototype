// Attack.h を新規作成
#pragma once
#include <vector>

class Attack {
public:
    // 攻撃の形(1:横4, 2:斜め4, 3:ピラミッド, 4:逆ピラミッド)と、初期座標を受け取って生成
    Attack(int attackType, float startX, float startY, float blockRadius);

    // 毎フレームの落下処理（キー操作は受け付けない）
    void Update(float speed);

    // 情報を取得
    float GetX() const { return m_x; }
    float GetY() const { return m_y; }
    int GetBlockCount() const { return m_blockCount; }
    int GetType(int index) const { return m_types[index]; }
    float GetBlockX(int index) const { return m_x + m_offsetX[index]; }
    float GetBlockY(int index) const { return m_y + m_offsetY[index]; }

private:
    float m_x, m_y;
    int m_blockCount;
    std::vector<int> m_types;
    std::vector<float> m_offsetX;
    std::vector<float> m_offsetY;
};