

#include "Board.h"

#include <cmath>
#include <set>
#include <map>
#include <random>

Board::Board() : m_baseX(0.0f), m_baseY(0.0f) { Clear(); }

void Board::Init(float baseX, float baseY) {
    m_baseX = baseX;
    m_baseY = baseY;
    Clear();
}

void Board::Clear() {
    for (int r = 0; r < BOARD_HEIGHT; ++r) {
        for (int c = 0; c < BOARD_WIDTH; ++c) {
            m_grid[r][c] = -1;
            m_crackedTurns[r][c] = -1; // ひび割れリセット
        }
    }
}

float Board::GetX(int col, int row) const {
    float x = m_baseX + col * (BLOCK_RADIUS * 2.0f); //
    if (row % 2 != 0) { x += BLOCK_RADIUS; }
    return x;
}

float Board::GetY(int row) const {
    return m_baseY + row * (BLOCK_RADIUS * 1.73205f); //
}

// ★四角形ではなく「ボール同士の距離」で当たり判定を行います
bool Board::IsCollision(float x, float nextY) const {
	// 1. 床との当たり判定（ボールの中心が床に触れたら衝突とみなす）
    if (nextY <= m_baseY) return true;

    // 2. 他のボールとの当たり判定（三平方の定理で距離を測る）
    for (int r = 0; r < BOARD_HEIGHT; ++r) {
        for (int c = 0; c < BOARD_WIDTH; ++c) {
            if (m_grid[r][c] != -1) {
                float bx = GetX(c, r);
                float by = GetY(r);
                float dx = x - bx;
                float dy = nextY - by;
                // 距離の2乗が、(半径×2)の2乗より小さければ衝突
                if (dx * dx + dy * dy <= (BLOCK_RADIUS * 2.0f) * (BLOCK_RADIUS * 2.0f)) {
                    return true;
                }
            }
        }
    }
    return false; // ぶつからなかった
}

// ★衝突した位置から、一番近い「空きマス」を探して吸着させます
void Board::LockBlock(float x, float y, int type) {
    float minDistSq = 999999.0f;
    int bestR = 0, bestC = 0;

    for (int r = 0; r < BOARD_HEIGHT; ++r) {
        for (int c = 0; c < BOARD_WIDTH; ++c) {
            if (m_grid[r][c] == -1) { // 空きマスのみチェック
                float cx = GetX(c, r);
                float cy = GetY(r);
                float distSq = (cx - x) * (cx - x) + (cy - y) * (cy - y);
                if (distSq < minDistSq) {
                    minDistSq = distSq;
                    bestR = r;
                    bestC = c;
                }
            }
        }
    }
    // 見つけた一番近いマスにボールを固定
    m_grid[bestR][bestC] = type;
}

// --- 指定したマスの「周囲6方向」の座標を取得する ---
void Board::GetNeighbors(int c, int r, std::vector<std::pair<int, int>>& neighbors) const {
    neighbors.clear();
    neighbors.push_back({ c - 1, r }); // 左
    neighbors.push_back({ c + 1, r }); // 右

    if (r % 2 == 0) { // 偶数段目
        neighbors.push_back({ c - 1, r + 1 }); // 左上
        neighbors.push_back({ c,     r + 1 }); // 右上
        neighbors.push_back({ c - 1, r - 1 }); // 左下
        neighbors.push_back({ c,     r - 1 }); // 右下
    }
    else {          // 奇数段目
        neighbors.push_back({ c,     r + 1 }); // 左上
        neighbors.push_back({ c + 1, r + 1 }); // 右上
        neighbors.push_back({ c,     r - 1 }); // 左下
        neighbors.push_back({ c + 1, r - 1 }); // 右下
    }

    // 盤面外の座標を除外する
    auto it = neighbors.begin();
    while (it != neighbors.end()) {
        if (it->first < 0 || it->first >= BOARD_WIDTH || it->second < 0 || it->second >= BOARD_HEIGHT) {
            it = neighbors.erase(it);
        }
        else {
            ++it;
        }
    }
}

// 0=右, 1=右上, 2=左上, 3=左, 4=左下, 5=右下
bool Board::GetHexNeighbor(int c, int r, int dir, int& nc, int& nr) const {
    nc = c; nr = r;
    if (dir == 0) { nc += 1; }
    else if (dir == 3) { nc -= 1; }
    else {
        int odd = (r % 2 != 0) ? 1 : 0;
        if (dir == 1) { nc = c + odd; nr = r + 1; }
        else if (dir == 2) { nc = c - 1 + odd; nr = r + 1; }
        else if (dir == 4) { nc = c - 1 + odd; nr = r - 1; }
        else if (dir == 5) { nc = c + odd; nr = r - 1; }
    }
    return (nc >= 0 && nc < BOARD_WIDTH && nr >= 0 && nr < BOARD_HEIGHT);
}

// --- 深さ優先探索（DFS）による再帰的な連結チェック ---
void Board::DFS(int c, int r, int type, std::vector<std::vector<bool>>& visited, std::vector<std::pair<int, int>>& connected) const {
    visited[r][c] = true;         // このマスを「探索済み」にする
    connected.push_back({ c, r });  // 繋がっているリストに追加

    std::vector<std::pair<int, int>> neighbors;
    GetNeighbors(c, r, neighbors);

    for (auto& n : neighbors) {
        int nc = n.first;
        int nr = n.second;
        // まだ探索しておらず、かつ「同じ色」ならさらに奥へ探索（DFS）
        if (!visited[nr][nc] && m_grid[nr][nc] == type) {
            DFS(nc, nr, type, visited, connected);
        }
    }
}

// --- 盤面全体を調べて消去する ---
std::vector<int> Board::CheckAndErase() 
{
    std::vector<int> attacks;
    std::set<std::pair<int, int>> newlyCracked; // 時限爆弾になる卵（ピラミッド）
    std::set<std::pair<int, int>> toErase;      // 即座に消える卵（ただの6個連結）

    // =========================================================
    // 1. ピラミッドと逆ピラミッドの検出（前回のコードそのまま）
    // =========================================================
    for (int r = 0; r < BOARD_HEIGHT; ++r) {
        for (int c = 0; c < BOARD_WIDTH; ++c) {
            int cB2, rB2, cB3, rB3, cM1, rM1, cM2, rM2, cT1, rT1;

            // 通常ピラミッド形状のチェック
            if (GetHexNeighbor(c, r, 0, cB2, rB2) && GetHexNeighbor(cB2, rB2, 0, cB3, rB3) &&
                GetHexNeighbor(c, r, 1, cM1, rM1) && GetHexNeighbor(cM1, rM1, 0, cM2, rM2) &&
                GetHexNeighbor(cM1, rM1, 1, cT1, rT1)) {

                std::vector<std::pair<int, int>> shape = { {c,r}, {cB2,rB2}, {cB3,rB3}, {cM1,rM1}, {cM2,rM2}, {cT1,rT1} };
                std::map<int, std::vector<std::pair<int, int>>> colorMap;
                for (auto& p : shape) {
                    if (m_grid[p.second][p.first] != -1) colorMap[m_grid[p.second][p.first]].push_back(p);
                }

                for (auto& kv : colorMap) {
                    // ★条件：同じ色が「5個」ならリーチ状態（ひび割れ）にする！
                    if (kv.second.size() == 5) {
                        for (auto& p : kv.second) newlyCracked.insert(p);
                    }
                }
            }

            // 逆ピラミッド形状のチェック
            int cB2_r, rB2_r, cB3_r, rB3_r, cM1_r, rM1_r, cM2_r, rM2_r, cT1_r, rT1_r;
            if (GetHexNeighbor(c, r, 0, cB2_r, rB2_r) && GetHexNeighbor(cB2_r, rB2_r, 0, cB3_r, rB3_r) &&
                GetHexNeighbor(c, r, 5, cM1_r, rM1_r) && GetHexNeighbor(cM1_r, rM1_r, 0, cM2_r, rM2_r) &&
                GetHexNeighbor(cM1_r, rM1_r, 5, cT1_r, rT1_r)) {

                std::vector<std::pair<int, int>> shapeRev = { {c,r}, {cB2_r,rB2_r}, {cB3_r,rB3_r}, {cM1_r,rM1_r}, {cM2_r,rM2_r}, {cT1_r,rT1_r} };
                std::map<int, std::vector<std::pair<int, int>>> colorMapRev;
                for (auto& p : shapeRev) {
                    if (m_grid[p.second][p.first] != -1) colorMapRev[m_grid[p.second][p.first]].push_back(p);
                }
                for (auto& kv : colorMapRev) {
                    if (kv.second.size() == 5) {
                        for (auto& p : kv.second) newlyCracked.insert(p);
                    }
                }
            }
        }
    }

    // ② 単純な6個以上の連結の検出（DFS）
    std::vector<std::vector<bool>> visited(BOARD_HEIGHT, std::vector<bool>(BOARD_WIDTH, false));
    for (int r = 0; r < BOARD_HEIGHT; ++r) {
        for (int c = 0; c < BOARD_WIDTH; ++c) {
            if (m_grid[r][c] != -1 && !visited[r][c]) {
                std::vector<std::pair<int, int>> connected;
                DFS(c, r, m_grid[r][c], visited, connected);
                if (connected.size() >= 6) {
                    for (auto& p : connected) toErase.insert(p);
                }
            }
        }
    }

    // ③ 状態の更新
    // 6個連結で消えるものは、ひび割れリストから除外して即座に消去
    for (auto& p : toErase) {
        m_grid[p.second][p.first] = -1;
        m_crackedTurns[p.second][p.first] = -1;
        newlyCracked.erase(p);
    }

    // リーチの卵を「ひび割れ（ターン0）」状態にする
    for (auto& p : newlyCracked) {
        if (m_crackedTurns[p.second][p.first] == -1) {
            m_crackedTurns[p.second][p.first] = 0; // ここからドキドキが始まる
        }
    }

    return attacks;
}

// --- 4. 物理演算（空白へ滑り落ちる） ---
bool Board::ApplyGravity() {
    bool movedAny = false;

    // 下の段から上の段へ向かって、浮いているボールがないかスキャンする
    for (int r = 1; r < BOARD_HEIGHT; ++r) {
        for (int c = 0; c < BOARD_WIDTH; ++c) {
            if (m_grid[r][c] != -1)
            {
                // すぐ下の「左下(bl)」と「右下(br)」の列インデックスを計算
                int bl_c = (r % 2 == 0) ? (c - 1) : c;
                int br_c = (r % 2 == 0) ? c : (c + 1);
                int b_r = r - 1; // 1つ下の段

                bool canMoveBL = (bl_c >= 0 && bl_c < BOARD_WIDTH && m_grid[b_r][bl_c] == -1);
                bool canMoveBR = (br_c >= 0 && br_c < BOARD_WIDTH && m_grid[b_r][br_c] == -1);

                // 真下がポッカリ空いている場合（2段下も空いているか）
                bool canMoveDown = (r >= 2 && m_grid[r - 2][c] == -1 && canMoveBL && canMoveBR);

                // 優先度：真下 ＞ 左右の滑り落ち
                if (canMoveDown)
                {
                    m_grid[r - 2][c] = m_grid[r][c]; m_grid[r][c] = -1; movedAny = true;
                }
                else if (canMoveBL && canMoveBR) 
                {
                    // 両方空いていたらランダムに滑らせるのもアリですが、ここでは左下に滑らせます
                    m_grid[b_r][bl_c] = m_grid[r][c]; m_grid[r][c] = -1; movedAny = true;
                }
                else if (canMoveBL)
                {
                    m_grid[b_r][bl_c] = m_grid[r][c]; m_grid[r][c] = -1; movedAny = true;
                }
                else if (canMoveBR) 
                {
                    m_grid[b_r][br_c] = m_grid[r][c]; m_grid[r][c] = -1; movedAny = true;
                }
                else if (m_grid[r][c] != -1 && m_grid[r - 1][c] == -1) 
                {
                    m_grid[r - 1][c] = m_grid[r][c];
                    m_crackedTurns[r - 1][c] = m_crackedTurns[r][c]; // ひび割れも一緒に落ちる
                    m_grid[r][c] = -1;
                    m_crackedTurns[r][c] = -1;
                    movedAny = true;
                }


            }
        }
    }
    // 1マスでも動いたボールがあれば true を返す
    return movedAny;
}

//カラーミックス（0:赤, 1:青, 2:白, 3:紫, 4:水色, 5:ピンク）
int Board::MixColor(int broken, int target) const 
{
    if (target == -1) return -1;
    if ((broken == 0 && target == 1) || (broken == 1 && target == 0)) return 3; // 赤＋青＝紫
    if ((broken == 1 && target == 2) || (broken == 2 && target == 1)) return 4; // 青＋白＝水色
    if ((broken == 0 && target == 2) || (broken == 2 && target == 0)) return 5; // 赤＋白＝ピンク
    return target; // 上記以外（すでに混ざっている色など）は変化しない
}

// 6. ファイルの一番下に、ターン経過＆割れる処理を追加
struct BreakInfo { int c, r, color; };

bool Board::AdvanceTurnAndBreak() {
    std::vector<BreakInfo> toBreak;
    // 0〜4ターン目の割れる確率（1T:40%, 2T:70%, 3T:90%, 4T:100%）
    int breakChances[] = { 0, 40, 70, 90, 100 };

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(1, 100);

    for (int r = 0; r < BOARD_HEIGHT; ++r) {
        for (int c = 0; c < BOARD_WIDTH; ++c) {
            if (m_crackedTurns[r][c] >= 0) {
                m_crackedTurns[r][c]++; // ターンを進める
                int turns = m_crackedTurns[r][c];
                if (turns > 4) turns = 4; // 限界突破防止

                // 確率判定
                if (dist(gen) <= breakChances[turns]) {
                    toBreak.push_back({ c, r, m_grid[r][c] });
                }
            }
        }
    }

    if (toBreak.empty()) return false; // 何も割れなかった

    // 割れる卵を一旦すべて盤面から消す
    for (auto& info : toBreak) {
        m_grid[info.r][info.c] = -1;
        m_crackedTurns[info.r][info.c] = -1;
    }

    // 割れた卵の周囲6方向に色を混ぜる（絵の具システム）
    for (auto& info : toBreak) {
        for (int dir = 0; dir < 6; ++dir) {
            int nc, nr;
            if (GetHexNeighbor(info.c, info.r, dir, nc, nr)) {
                int targetColor = m_grid[nr][nc];
                if (targetColor != -1) {
                    m_grid[nr][nc] = MixColor(info.color, targetColor);
                }
            }
        }
    }
    return true; // 割れて色が混ざった！
}
