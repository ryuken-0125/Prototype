#include "Board.h"
#include <cmath>
#include <set>

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
        }
    }
}

float Board::GetX(int col, int row) const {
    float x = m_baseX + col * (BLOCK_RADIUS * 2.0f); // ★変更
    if (row % 2 != 0) { x += BLOCK_RADIUS; }
    return x;
}

float Board::GetY(int row) const {
    return m_baseY + row * (BLOCK_RADIUS * 1.73205f); // ★変更
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

int Board::GetBlockType(int col, int row) const {
    if (col >= 0 && col < BOARD_WIDTH && row >= 0 && row < BOARD_HEIGHT) {
        return m_grid[row][col];
    }
    return -1;
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

/*
bool Board::CheckAndErase() {
    std::vector<std::vector<bool>> visited(BOARD_HEIGHT, std::vector<bool>(BOARD_WIDTH, false));
    bool erasedAny = false;

    // シックスボールパズルなので「6個」繋がったら消す（テスト時は 3 などに減らしてもOK）
    const int ERASE_COUNT = 6;

    for (int r = 0; r < BOARD_HEIGHT; ++r) {
        for (int c = 0; c < BOARD_WIDTH; ++c) {
            if (m_grid[r][c] != -1 && !visited[r][c]) {
                std::vector<std::pair<int, int>> connected;
                DFS(c, r, m_grid[r][c], visited, connected); // ここでDFS発動

                // 条件を満たしたら盤面から消す
                if (connected.size() >= ERASE_COUNT) {
                    for (auto& p : connected) {
                        m_grid[p.second][p.first] = -1;
                    }
                    erasedAny = true;
                }
            }
        }
    }
    return erasedAny;
}
*/

// --- 3. 盤面全体を調べて消去する ---
std::vector<int> Board::CheckAndErase() {
    std::vector<int> attacks;
    std::set<std::pair<int, int>> toErase;
    std::vector<std::vector<bool>> visited(BOARD_HEIGHT, std::vector<bool>(BOARD_WIDTH, false));

    const int ERASE_COUNT = 6; // 6個以上で消える

    for (int r = 0; r < BOARD_HEIGHT; ++r) {
        for (int c = 0; c < BOARD_WIDTH; ++c) {
            // まだ調べていないブロックがあればDFSで繋がっている数を数える
            if (m_grid[r][c] != -1 && !visited[r][c]) {
                std::vector<std::pair<int, int>> connected;
                DFS(c, r, m_grid[r][c], visited, connected);

                // 6個以上繋がっていた場合のみ、消去＆攻撃判定を行う
                if (connected.size() >= ERASE_COUNT) {
                    int color = m_grid[r][c];
                    bool attackAdded = false;

                    // この「消えるグループ」の中に、特定の形が含まれているかチェック
                    for (auto& p : connected) {
                        int bc = p.first;
                        int br = p.second;
                        int nc1, nr1, nc2, nr2, nc3, nr3, nc4, nr4, nc5, nr5;

                        // ① 横4つのストレート
                        if (!attackAdded &&
                            GetHexNeighbor(bc, br, 0, nc1, nr1) && m_grid[nr1][nc1] == color &&
                            GetHexNeighbor(nc1, nr1, 0, nc2, nr2) && m_grid[nr2][nc2] == color &&
                            GetHexNeighbor(nc2, nr2, 0, nc3, nr3) && m_grid[nr3][nc3] == color) {
                            attacks.push_back(1);
                            attackAdded = true;
                        }
                        // ② 斜め4つのストレート (右上方向)
                        else if (!attackAdded &&
                            GetHexNeighbor(bc, br, 1, nc1, nr1) && m_grid[nr1][nc1] == color &&
                            GetHexNeighbor(nc1, nr1, 1, nc2, nr2) && m_grid[nr2][nc2] == color &&
                            GetHexNeighbor(nc2, nr2, 1, nc3, nr3) && m_grid[nr3][nc3] == color) {
                            attacks.push_back(2);
                            attackAdded = true;
                        }
                        // ③ ピラミッド (下3, 中2, 上1)
                        else if (!attackAdded &&
                            GetHexNeighbor(bc, br, 0, nc1, nr1) && m_grid[nr1][nc1] == color &&
                            GetHexNeighbor(nc1, nr1, 0, nc2, nr2) && m_grid[nr2][nc2] == color &&
                            GetHexNeighbor(bc, br, 1, nc3, nr3) && m_grid[nr3][nc3] == color &&
                            GetHexNeighbor(nc3, nr3, 0, nc4, nr4) && m_grid[nr4][nc4] == color &&
                            GetHexNeighbor(nc3, nr3, 1, nc5, nr5) && m_grid[nr5][nc5] == color) {
                            attacks.push_back(3);
                            attackAdded = true;
                        }
                        // ④ 逆ピラミッド (上3, 中2, 下1)
                        else if (!attackAdded &&
                            GetHexNeighbor(bc, br, 0, nc1, nr1) && m_grid[nr1][nc1] == color &&
                            GetHexNeighbor(nc1, nr1, 0, nc2, nr2) && m_grid[nr2][nc2] == color &&
                            GetHexNeighbor(bc, br, 5, nc3, nr3) && m_grid[nr3][nc3] == color &&
                            GetHexNeighbor(nc3, nr3, 0, nc4, nr4) && m_grid[nr4][nc4] == color &&
                            GetHexNeighbor(nc3, nr3, 5, nc5, nr5) && m_grid[nr5][nc5] == color) {
                            attacks.push_back(4);
                            attackAdded = true;
                        }
                    }

                    // グループ全員を消去予定リストに追加
                    for (auto& p : connected) {
                        toErase.insert(p);
                    }
                }
            }
        }
    }

    // まとめて消去する
    for (auto& p : toErase) {
        m_grid[p.second][p.first] = -1;
    }

    return attacks;
}

// --- 4. 物理演算（空白へ滑り落ちる） ---
bool Board::ApplyGravity() {
    bool movedAny = false;

    // 下の段から上の段へ向かって、浮いているボールがないかスキャンする
    for (int r = 1; r < BOARD_HEIGHT; ++r) {
        for (int c = 0; c < BOARD_WIDTH; ++c) {
            if (m_grid[r][c] != -1) {
                // すぐ下の「左下(bl)」と「右下(br)」の列インデックスを計算
                int bl_c = (r % 2 == 0) ? (c - 1) : c;
                int br_c = (r % 2 == 0) ? c : (c + 1);
                int b_r = r - 1; // 1つ下の段

                bool canMoveBL = (bl_c >= 0 && bl_c < BOARD_WIDTH && m_grid[b_r][bl_c] == -1);
                bool canMoveBR = (br_c >= 0 && br_c < BOARD_WIDTH && m_grid[b_r][br_c] == -1);

                // 真下がポッカリ空いている場合（2段下も空いているか）
                bool canMoveDown = (r >= 2 && m_grid[r - 2][c] == -1 && canMoveBL && canMoveBR);

                // 優先度：真下 ＞ 左右の滑り落ち
                if (canMoveDown) {
                    m_grid[r - 2][c] = m_grid[r][c]; m_grid[r][c] = -1; movedAny = true;
                }
                else if (canMoveBL && canMoveBR) {
                    // 両方空いていたらランダムに滑らせるのもアリですが、ここでは左下に滑らせます
                    m_grid[b_r][bl_c] = m_grid[r][c]; m_grid[r][c] = -1; movedAny = true;
                }
                else if (canMoveBL) {
                    m_grid[b_r][bl_c] = m_grid[r][c]; m_grid[r][c] = -1; movedAny = true;
                }
                else if (canMoveBR) {
                    m_grid[b_r][br_c] = m_grid[r][c]; m_grid[r][c] = -1; movedAny = true;
                }
            }
        }
    }
    // 1マスでも動いたボールがあれば true を返す
    return movedAny;
}