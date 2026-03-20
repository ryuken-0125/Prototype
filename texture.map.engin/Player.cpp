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

    generatedAttacks = m_board.CheckAndErase();
    if (!generatedAttacks.empty()) { m_waitTimer = 20; return generatedAttacks; }

    // ==========================================================
    // 1. 画面に降っている攻撃ブロックがあれば、それを最優先で落とす
    if (!m_activeAttacks.empty()) {
        auto it = m_activeAttacks.begin();
        float fallSpeed = 0.04f;

        bool canFall = true;
        for (int i = 0; i < it->GetBlockCount(); ++i) {
            if (m_board.IsCollision(it->GetBlockX(i), it->GetBlockY(i) - fallSpeed)) {
                canFall = false; break;
            }
        }

        if (canFall) {
            it->Update(fallSpeed);
        }
        else {
            for (int i = 0; i < it->GetBlockCount(); ++i) {
                m_board.LockBlock(it->GetBlockX(i), it->GetBlockY(i), it->GetType(i));
            }
            m_activeAttacks.erase(it);
            m_waitTimer = 5;
        }

        // ★攻撃ブロックが降っている間は、ここで処理を終えるためプレイヤーは動けない！
        return generatedAttacks;
    }
    // ==========================================================

    // ==========================================================
    // 2. プレイヤーのブロックがなく、かつ攻撃キューにストックがあれば、攻撃を出現させる
    if (!m_fallingGroup.IsActive()) {
        if (!m_attackQueue.empty()) {
            int attackType = m_attackQueue.front();
            m_attackQueue.erase(m_attackQueue.begin());

            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<int> colDist(1, BOARD_WIDTH - 4);

            float startX = m_board.GetX(colDist(gen), BOARD_HEIGHT - 1);
            float startY = m_board.GetY(BOARD_HEIGHT + 2);

            m_activeAttacks.push_back(Attack(attackType, startX, startY, BLOCK_RADIUS));

            return generatedAttacks; // ★出現させただけでこのフレームは終了
        }
        else {
            // キューが空の時（平和な時）だけ、通常のブロックを生成してあげる
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

/*
std::vector<int> Player::Update(int leftKey, int rightKey, int downKey, int rotateKey, bool isCPU) {
    std::vector<int> generatedAttacks;

    if (m_waitTimer > 0) { m_waitTimer--; return generatedAttacks; }
    if (m_board.ApplyGravity()) { m_waitTimer = 8; return generatedAttacks; }

    generatedAttacks = m_board.CheckAndErase();
    if (!generatedAttacks.empty()) { m_waitTimer = 20; return generatedAttacks; }

    // ==========================================================
    // ★追加: 攻撃キューに溜まっているものがあれば、操作不可のAttackとして画面に出現させる
    if (!m_attackQueue.empty()) {
        int attackType = m_attackQueue.front();
        m_attackQueue.erase(m_attackQueue.begin()); // キューから取り出す

        // ランダムな列から降らせる
        std::random_device rd;
        std::mt19937 gen(rd());
        // はみ出さないように適度に列の範囲を絞る（攻撃の幅による）
        std::uniform_int_distribution<int> colDist(1, BOARD_WIDTH - 4);

        float startX = m_board.GetX(colDist(gen), BOARD_HEIGHT - 1);
        float startY = m_board.GetY(BOARD_HEIGHT + 2); // 画面の見えない少し上から降らせる

        // Attackオブジェクトを生成して、落下中リストに追加
        m_activeAttacks.push_back(Attack(attackType, startX, startY, BLOCK_RADIUS));
    }
    // ==========================================================

    // ==========================================================
    // ★追加: 落下中の「操作不可の攻撃ブロック」の処理
    auto it = m_activeAttacks.begin();
    while (it != m_activeAttacks.end()) {
        float fallSpeed = 0.04f; // お邪魔ブロックの落下速度（少し早め）

        // 落下先がぶつかるかチェック
        bool canFall = true;
        for (int i = 0; i < it->GetBlockCount(); ++i) {
            float bx = it->GetBlockX(i);
            float nextBy = it->GetBlockY(i) - fallSpeed;
            if (m_board.IsCollision(bx, nextBy)) { canFall = false; break; }
        }

        if (canFall) {
            it->Update(fallSpeed); // ぶつからなければそのまま落下
            ++it;
        }
        else {
            // ぶつかったら盤面にロックする
            for (int i = 0; i < it->GetBlockCount(); ++i) {
                m_board.LockBlock(it->GetBlockX(i), it->GetBlockY(i), it->GetType(i));
            }
            // ロック完了したらリストから削除
            it = m_activeAttacks.erase(it);

            m_waitTimer = 5; // 着地時の余韻と、直後の連鎖判定のためのウェイト
            return generatedAttacks; // 一旦ここで処理を区切る
        }
    }
    // ==========================================================

    if (m_fallingGroup.IsActive()) {
        float speed = 0.01f;

        if (!isCPU) {
            // 人間の操作
            if (GetAsyncKeyState(leftKey) & 0x8000) {
                if (!m_isLeftPressed) { m_fallingGroup.SetX(m_fallingGroup.GetX() - BLOCK_RADIUS * 2.0f); m_isLeftPressed = true; }
            }
            else { m_isLeftPressed = false; }

            if (GetAsyncKeyState(rightKey) & 0x8000) {
                if (!m_isRightPressed) { m_fallingGroup.SetX(m_fallingGroup.GetX() + BLOCK_RADIUS * 2.0f); m_isRightPressed = true; }
            }
            else { m_isRightPressed = false; }

            if (GetAsyncKeyState(downKey) & 0x8000) speed = 0.05f;

            // =================================================================
            // ★企画段階：回転機能
            // 回転機能をオフ（位置変更なし）にしたい場合は、
            // 以下の if (GetAsyncKeyState(rotateKey)... のブロックを 注釈で囲んでコメントアウトしてください。
            if (GetAsyncKeyState(rotateKey) & 0x8000) {
                if (!m_isRotatePressed) {
                    m_fallingGroup.RotateClockwise();

                    // 回転時に壁にめり込んだ場合、内側に押し戻す（壁蹴り処理）
                    for (int i = 0; i < m_fallingGroup.GetBlockCount(); ++i) {
                        float bx = m_fallingGroup.GetBlockX(i);
                        if (bx < m_baseX) m_fallingGroup.SetX(m_fallingGroup.GetX() + BLOCK_RADIUS * 2.0f);
                        if (bx > m_baseX + (BOARD_WIDTH - 1) * (BLOCK_RADIUS * 2.0f)) m_fallingGroup.SetX(m_fallingGroup.GetX() - BLOCK_RADIUS * 2.0f);
                    }
                    m_isRotatePressed = true;
                }
            }
            else { m_isRotatePressed = false; }
            // =================================================================

        }
        else {
            // CPUの操作（そのまま変更なし）
            float targetX = m_board.GetX(m_cpuTargetCol, BOARD_HEIGHT - 1);
            if (m_fallingGroup.GetX() < targetX - 0.1f) {
                m_fallingGroup.SetX(m_fallingGroup.GetX() + BLOCK_RADIUS * 2.0f);
            }
            else if (m_fallingGroup.GetX() > targetX + 0.1f) {
                m_fallingGroup.SetX(m_fallingGroup.GetX() - BLOCK_RADIUS * 2.0f);
            }
            else { speed = 0.05f; }
        }

        // はみ出し防止（壁ドン）
        if (m_fallingGroup.GetX() < m_baseX) m_fallingGroup.SetX(m_baseX);
        if (m_fallingGroup.GetX() > m_baseX + (BOARD_WIDTH - 1) * (BLOCK_RADIUS * 2.0f)) m_fallingGroup.SetX(m_baseX + (BOARD_WIDTH - 1) * (BLOCK_RADIUS * 2.0f));

        // ★修正: 落下判定。グループ内の「どれか1つでも」ぶつかったら止まる
        bool canFall = true;
        for (int i = 0; i < m_fallingGroup.GetBlockCount(); ++i) {
            float bx = m_fallingGroup.GetBlockX(i);
            float nextBy = m_fallingGroup.GetBlockY(i) - speed;
            if (m_board.IsCollision(bx, nextBy)) {
                canFall = false;
                break;
            }
        }

        if (canFall) {
            m_fallingGroup.SetY(m_fallingGroup.GetY() - speed);
        }
        else {
            // ★修正: ぶつかったら、グループ全員を今の位置でロックする
            for (int i = 0; i < m_fallingGroup.GetBlockCount(); ++i) {
                m_board.LockBlock(m_fallingGroup.GetBlockX(i), m_fallingGroup.GetBlockY(i), m_fallingGroup.GetType(i));
            }
            m_fallingGroup.SetInactive();
            m_waitTimer = 5;
            // （ロック後、空中に浮いたボールは次のフレームで ApplyGravity() によって自動的に下にスッポリ落ちます）
        }
    }
    else {
        SpawnRandomBlock(isCPU);
    }
}
*/

/*
void Player::Update(int leftKey, int rightKey, int downKey, int rotateKey, bool isCPU) {
    if (m_waitTimer > 0) { m_waitTimer--; return; }
    if (m_board.ApplyGravity()) { m_waitTimer = 8; return; }
    if (m_board.CheckAndErase()) { m_waitTimer = 20; return; }

    if (m_fallingGroup.IsActive()) {
        float speed = 0.01f;

        if (!isCPU) {
            // 人間の操作
            if (GetAsyncKeyState(leftKey) & 0x8000) {
                if (!m_isLeftPressed) { m_fallingGroup.SetX(m_fallingGroup.GetX() - BLOCK_RADIUS * 2.0f); m_isLeftPressed = true; }
            }
            else { m_isLeftPressed = false; }

            if (GetAsyncKeyState(rightKey) & 0x8000) {
                if (!m_isRightPressed) { m_fallingGroup.SetX(m_fallingGroup.GetX() + BLOCK_RADIUS * 2.0f); m_isRightPressed = true; }
            }
            else { m_isRightPressed = false; }

            if (GetAsyncKeyState(downKey) & 0x8000) speed = 0.05f;

            // =================================================================
            // ★企画段階：回転機能
            // 回転機能をオフ（位置変更なし）にしたい場合は、
            // 以下の if (GetAsyncKeyState(rotateKey)... のブロックを 注釈 で囲んでコメントアウトしてください。
            if (GetAsyncKeyState(rotateKey) & 0x8000) {
                if (!m_isRotatePressed) {
                    m_fallingGroup.RotateClockwise();

                    // 回転時に壁にめり込んだ場合、内側に押し戻す（壁蹴り処理）
                    for (int i = 0; i < m_fallingGroup.GetBlockCount(); ++i) {
                        float bx = m_fallingGroup.GetBlockX(i);
                        if (bx < m_baseX) m_fallingGroup.SetX(m_fallingGroup.GetX() + BLOCK_RADIUS * 2.0f);
                        if (bx > m_baseX + (BOARD_WIDTH - 1) * (BLOCK_RADIUS * 2.0f)) m_fallingGroup.SetX(m_fallingGroup.GetX() - BLOCK_RADIUS * 2.0f);
                    }
                    m_isRotatePressed = true;
                }
            }
            else { m_isRotatePressed = false; }
            // =================================================================

        }
        else {
            // CPUの操作（そのまま変更なし）
            float targetX = m_board.GetX(m_cpuTargetCol, BOARD_HEIGHT - 1);
            if (m_fallingGroup.GetX() < targetX - 0.1f) {
                m_fallingGroup.SetX(m_fallingGroup.GetX() + BLOCK_RADIUS * 2.0f);
            }
            else if (m_fallingGroup.GetX() > targetX + 0.1f) {
                m_fallingGroup.SetX(m_fallingGroup.GetX() - BLOCK_RADIUS * 2.0f);
            }
            else { speed = 0.05f; }
        }

        // はみ出し防止（壁ドン）
        if (m_fallingGroup.GetX() < m_baseX) m_fallingGroup.SetX(m_baseX);
        if (m_fallingGroup.GetX() > m_baseX + (BOARD_WIDTH - 1) * (BLOCK_RADIUS * 2.0f)) m_fallingGroup.SetX(m_baseX + (BOARD_WIDTH - 1) * (BLOCK_RADIUS * 2.0f));

        // ★修正: 落下判定。グループ内の「どれか1つでも」ぶつかったら止まる
        bool canFall = true;
        for (int i = 0; i < m_fallingGroup.GetBlockCount(); ++i) {
            float bx = m_fallingGroup.GetBlockX(i);
            float nextBy = m_fallingGroup.GetBlockY(i) - speed;
            if (m_board.IsCollision(bx, nextBy)) {
                canFall = false;
                break;
            }
        }

        if (canFall) {
            m_fallingGroup.SetY(m_fallingGroup.GetY() - speed);
        }
        else {
            // ★修正: ぶつかったら、グループ全員を今の位置でロックする
            for (int i = 0; i < m_fallingGroup.GetBlockCount(); ++i) {
                m_board.LockBlock(m_fallingGroup.GetBlockX(i), m_fallingGroup.GetBlockY(i), m_fallingGroup.GetType(i));
            }
            m_fallingGroup.SetInactive();
            m_waitTimer = 5;
            // （ロック後、空中に浮いたボールは次のフレームで ApplyGravity() によって自動的に下にスッポリ落ちます）
        }
    }
    else {
        SpawnRandomBlock(isCPU);
    }
}
*/

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


