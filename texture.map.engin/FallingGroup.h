// FallingGroup.h
#pragma once
#include <vector>

class FallingGroup {
public:
    FallingGroup();

    // countの数だけボールを生成する
    void Spawn(int count, const std::vector<int>& types, float startX, float startY, float blockRadius);

    // グループ全体の座標
    float GetX() const { return m_x; }
    float GetY() const { return m_y; }
    void SetX(float x) { m_x = x; }
    void SetY(float y) { m_y = y; }

    // 個別のボールの情報
    int GetBlockCount() const { return m_blockCount; }
    int GetType(int index) const { return m_types[index]; }
    float GetBlockX(int index) const; // index番目のボールの実際のX座標
    float GetBlockY(int index) const; // index番目のボールの実際のY座標

    bool IsActive() const { return m_isActive; }
    void SetInactive() { m_isActive = false; }

    // 時計回りに回転
    void RotateClockwise();

    // FallingGroup.h に追加
    // 自由に座標を指定して生成する関数
    void SpawnCustom(int count, const std::vector<int>& types, const std::vector<float>& offX, const std::vector<float>& offY, float startX, float startY);

private:
    bool m_isActive;
    float m_x, m_y; // グループの「中心(下部)」の座標
    int m_blockCount; // ボールの個数

    std::vector<int> m_types;    // 各ボールの色
    std::vector<float> m_offsetX; // 中心からのXズレ
    std::vector<float> m_offsetY; // 中心からのYズレ
};