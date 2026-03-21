#pragma once
#include "Player.h"

class Tutorial {
public:
    Tutorial();
    void Init();
    void Update(int leftKey, int rightKey, int downKey, int rotateKey);

    // 描画のために Engine からアクセスできるように public にしておきます
    Player m_player;
    Player m_dummy;
};