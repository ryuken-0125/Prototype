// Attack.cpp を新規作成
#include "Attack.h"
#include <random>

Attack::Attack(int attackType, float startX, float startY, float blockRadius)
    : m_x(startX), m_y(startY) {

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> typeDist(0, 3); // 0〜3のランダムな色

    float R = blockRadius;
    float H = blockRadius * 1.73205f; // 高さのズレ（ルート3）

    // Attack.cpp の Attack コンストラクタ内を修正します

    // 形に応じたボールの個数と相対座標の設定
    if (attackType == 1) { // 横4つ
        m_blockCount = 4;
        m_offsetX = { 0, 2 * R, 4 * R, 6 * R };
        m_offsetY = { 0, 0, 0, 0 };
    }
    else if (attackType == 2) { // 斜め4つ
        m_blockCount = 4;
        m_offsetX = { 0, R, 2 * R, 3 * R };
        m_offsetY = { 0, H, 2 * H, 3 * H };
    }
    else if (attackType == 3) { //ピラミッド (下3, 中2, 上1)
        m_blockCount = 6;
        m_offsetX = { 0, 2 * R, 4 * R, R, 3 * R, 2 * R };
        m_offsetY = { 0, 0,   0,   H, H,   2 * H };
    }
    else if (attackType == 4) { //逆ピラミッド (上3, 中2, 下1)
        m_blockCount = 6;
        m_offsetX = { 0, 2 * R, 4 * R, R,  3 * R, 2 * R };
        m_offsetY = { 0, 0,   0,  -H, -H,  -2 * H };
    }
    else {
        m_blockCount = 1;
        m_offsetX = { 0 }; m_offsetY = { 0 };
    }
    // 形が決まったら、それぞれのボールの色をランダムに決定
    for (int i = 0; i < m_blockCount; ++i) {
        m_types.push_back(typeDist(gen));
    }
}

void Attack::Update(float speed) {
    // 操作不可能なので、ただひたすら下に落ちるだけ
    m_y -= speed;
}