#include "Player.h"
#include <random>
#include <windows.h>


Player::Player() : m_waitTimer(0), m_baseX(0.0f), m_baseY(0.0f),
m_isLeftPressed(false), m_isRightPressed(false), m_isRotatePressed(false), m_cpuTargetCol(0) {
}

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

void Player::SpawnRandomBlock(bool isCPU) 
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> typeDist(0, 3);
    std::uniform_int_distribution<int> colDist(0, BOARD_WIDTH - 1);

    // ★設定された個数分だけ、ランダムな色を用意する
    std::vector<int> types;
    for (int i = 0; i < FALLING_BLOCK_COUNT; ++i) {
        types.push_back(typeDist(gen));
    }

    int randomCol = colDist(gen);
    float startX = m_board.GetX(randomCol, BOARD_HEIGHT - 1);
    float startY = m_board.GetY(BOARD_HEIGHT - 1) + 2.0f;

    m_fallingGroup.Spawn(FALLING_BLOCK_COUNT, types, startX, startY, BLOCK_RADIUS);

    if (isCPU) {
        // AIは一番下のボールの色(types[0])を基準に考える
        DecideCPUTarget(types[0]);
    }
}

// Player.cpp の Update() をまるごと差し替え

std::vector<int> Player::Update(int leftKey, int rightKey, int downKey, int rotateKey, bool isCPU) {
    std::vector<int> generatedAttacks;

    if (m_waitTimer > 0) { m_waitTimer--; return generatedAttacks; }
    if (m_board.ApplyGravity()) { m_waitTimer = 8; return generatedAttacks; }

    // 消去判定と攻撃の増幅（前回実装した部分）
    auto rawAttacks = m_board.CheckAndErase();
    if (!rawAttacks.empty()) {
        for (int a : rawAttacks) {
            for (int i = 0; i < ATTACK_MULTIPLIER; ++i) {
                generatedAttacks.push_back(a);
            }
        }
        m_waitTimer = 20;
        return generatedAttacks;
    }

    // ==========================================================
    // 1. 画面に降っている攻撃ブロック群の落下処理
    if (!m_activeAttacks.empty()) {
        auto it = m_activeAttacks.begin();
        float fallSpeed = 0.04f;
        bool lockedAny = false; // 今回のフレームで着地したものがあったか

        // 落下中の全ての攻撃ブロックを同時に動かす
        while (it != m_activeAttacks.end()) {
            bool canFall = true;
            // 落下先に障害物がないかチェック
            for (int i = 0; i < it->GetBlockCount(); ++i) {
                if (m_board.IsCollision(it->GetBlockX(i), it->GetBlockY(i) - fallSpeed)) {
                    canFall = false; break;
                }
            }

            if (canFall) {
                it->Update(fallSpeed); // 空中なら落とす
                ++it;
            }
            else {
                // 着地した攻撃ブロックだけを盤面にロックする
                for (int i = 0; i < it->GetBlockCount(); ++i) {
                    m_board.LockBlock(it->GetBlockX(i), it->GetBlockY(i), it->GetType(i));
                }
                it = m_activeAttacks.erase(it); // ロック完了したものはリストから消す
                lockedAny = true;
            }
        }

        // どれか1つでも着地したら少しウェイトをかけて連鎖判定を待つ
        if (lockedAny) {
            m_waitTimer = 5;
        }

        // 攻撃が降っている間はプレイヤーは操作不可！
        return generatedAttacks;
    }
    // ==========================================================

    // ==========================================================
    // 2. プレイヤーのブロックがなく、かつ攻撃キューにストックがあれば一気に出現させる
    if (!m_fallingGroup.IsActive()) {
        if (!m_attackQueue.empty()) {

            // ★いつでも変更可能: 連続して降ってくる攻撃の「縦の間隔」
            // 数字を大きくすると間隔が広がり、小さくすると塊になってドサッと降ります。
            float attackGapY = BLOCK_RADIUS * 7.0f;

            // 1個目の攻撃の初期高度
            float currentStartY = m_board.GetY(BOARD_HEIGHT + 2);

            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<int> colDist(1, BOARD_WIDTH - 4);

            // キュー（待ち行列）に入っている攻撃を「すべて」一気に出現させる！
            while (!m_attackQueue.empty()) {
                int attackType = m_attackQueue.front();
                m_attackQueue.erase(m_attackQueue.begin());

                float startX = m_board.GetX(colDist(gen), BOARD_HEIGHT - 1);

                // 攻撃ブロックをリストに追加（Y座標はcurrentStartYを使う）
                m_activeAttacks.push_back(Attack(attackType, startX, currentStartY, BLOCK_RADIUS));

                // 次の攻撃は、前の攻撃よりもさらに「上空」に配置してズラす
                currentStartY += attackGapY;
            }

            return generatedAttacks; // 出現させただけでこのフレームは終了
        }
        else {
            // 攻撃が何もない平和な時だけ、通常のブロックを生成してあげる
            SpawnRandomBlock(isCPU);
        }
    }
     // ==========================================================

    // 3. プレイヤーの操作と落下処理（以前のまま）
    if (m_fallingGroup.IsActive()) {
        float speed = 0.01f;

        if (!isCPU) {
            if (GetAsyncKeyState(leftKey) & 0x8000) {
                if (!m_isLeftPressed) { m_fallingGroup.SetX(m_fallingGroup.GetX() - BLOCK_RADIUS * 2.0f); m_isLeftPressed = true; }
            }
            else { m_isLeftPressed = false; }

            if (GetAsyncKeyState(rightKey) & 0x8000) {
                if (!m_isRightPressed) { m_fallingGroup.SetX(m_fallingGroup.GetX() + BLOCK_RADIUS * 2.0f); m_isRightPressed = true; }
            }
            else { m_isRightPressed = false; }

            if (GetAsyncKeyState(downKey) & 0x8000) speed = 0.05f;

            if (GetAsyncKeyState(rotateKey) & 0x8000) {
                if (!m_isRotatePressed) {
                    m_fallingGroup.RotateClockwise();
                    for (int i = 0; i < m_fallingGroup.GetBlockCount(); ++i) {
                        float bx = m_fallingGroup.GetBlockX(i);
                        if (bx < m_baseX) m_fallingGroup.SetX(m_fallingGroup.GetX() + BLOCK_RADIUS * 2.0f);
                        if (bx > m_baseX + (BOARD_WIDTH - 1) * (BLOCK_RADIUS * 2.0f)) m_fallingGroup.SetX(m_fallingGroup.GetX() - BLOCK_RADIUS * 2.0f);
                    }
                    m_isRotatePressed = true;
                }
            }
            else { m_isRotatePressed = false; }
        }
        else {
            float targetX = m_board.GetX(m_cpuTargetCol, BOARD_HEIGHT - 1);
            if (m_fallingGroup.GetX() < targetX - 0.1f) { m_fallingGroup.SetX(m_fallingGroup.GetX() + BLOCK_RADIUS * 2.0f); }
            else if (m_fallingGroup.GetX() > targetX + 0.1f) { m_fallingGroup.SetX(m_fallingGroup.GetX() - BLOCK_RADIUS * 2.0f); }
            else { speed = 0.05f; }
        }

        if (m_fallingGroup.GetX() < m_baseX) m_fallingGroup.SetX(m_baseX);
        if (m_fallingGroup.GetX() > m_baseX + (BOARD_WIDTH - 1) * (BLOCK_RADIUS * 2.0f)) m_fallingGroup.SetX(m_baseX + (BOARD_WIDTH - 1) * (BLOCK_RADIUS * 2.0f));

        bool canFall = true;
        for (int i = 0; i < m_fallingGroup.GetBlockCount(); ++i) {
            float bx = m_fallingGroup.GetBlockX(i);
            float nextBy = m_fallingGroup.GetBlockY(i) - speed;
            if (m_board.IsCollision(bx, nextBy)) { canFall = false; break; }
        }

        if (canFall) {
            m_fallingGroup.SetY(m_fallingGroup.GetY() - speed);
        }
        else {
            for (int i = 0; i < m_fallingGroup.GetBlockCount(); ++i) {
                m_board.LockBlock(m_fallingGroup.GetBlockX(i), m_fallingGroup.GetBlockY(i), m_fallingGroup.GetType(i));
            }
            m_fallingGroup.SetInactive();
            m_waitTimer = 5;
        }
    }

    return generatedAttacks;
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


