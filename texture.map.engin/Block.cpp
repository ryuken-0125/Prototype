#include "Block.h"

// コンストラクタ（初期化）
Block::Block() : m_type(0), m_x(0.0f), m_y(0.0f), m_isActive(false) {}

void Block::Spawn(int type, float startX, float startY) {
    m_type = type;
    m_x = startX;
    m_y = startY;
    m_isActive = true;
}

void Block::Update(float fallSpeed) {
    if (m_isActive) {
        // Y座標を減らして下へ移動させる
        m_y -= fallSpeed;
    }
}