
#pragma once

class Block {
public:
    Block();

    // ブロックを初期位置に出現させる
    void Spawn(int type, float startX, float startY);

    // ブロックを落下させる
    void Update(float fallSpeed);

    void SetInactive() { m_isActive = false; }

    // 情報の取得
    int GetType() const { return m_type; }
    float GetX() const { return m_x; }
    float GetY() const { return m_y; }
    bool IsActive() const { return m_isActive; }


    void SetX(float x) { m_x = x; }
    void SetY(float y) { m_y = y; }


private:
    int m_type;       // ブロックの種類 (0:R, 1:G, 2:B, 3:Y)
    float m_x, m_y;   // 現在の座標
    bool m_isActive;  // 画面上に存在しているかどうか
};