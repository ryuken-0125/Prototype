// Tutorial.cpp の中身をまるごと以下に差し替えます

#include "Tutorial.h"

// コンストラクタで初期化
Tutorial::Tutorial() : m_phase(0), m_clearTimer(0) {}

void Tutorial::Init() {
    m_player.Reset();
    m_dummy.Reset();
    m_player.Init(-5.4f, 0.0f);
    m_dummy.Init(0.6f, 0.0f);
    m_dummy.SetDummyMode(true);

    m_phase = 0;
    m_clearTimer = 180; // 60fps環境なら約3秒間、クリア後に待つ
}

void Tutorial::Update(int leftKey, int rightKey, int downKey, int rotateKey) {
    auto attacks = m_player.Update(leftKey, rightKey, downKey, rotateKey, false);

    // クリア後（フェーズ3以降）はタイマーを減らすだけ
    if (m_phase >= 3) {
        if (m_clearTimer > 0) m_clearTimer--;
    }
    else {
        // まだクリアしていない場合、プレイヤーの攻撃をチェック
        if (!attacks.empty()) {
            bool phaseCleared = false;

            for (int a : attacks) {
                // 条件判定: attackType は 1:横4, 2:斜め4, 3:ピラミッド, 4:逆ピラミッド
                if (m_phase == 0 && a == 1) {
                    phaseCleared = true; // 横4つ達成！
                }
                else if (m_phase == 1 && a == 2) {
                    phaseCleared = true; // 斜め4つ達成！
                }
                else if (m_phase == 2 && (a == 3 || a == 4)) {
                    phaseCleared = true; // ピラミッド達成！
                }

                // 攻撃の仕方を教えるため、作られた攻撃はダミーに送る
                m_dummy.AddAttack(a);
            }

            // 正しい形を作って条件を満たしたら、次のステップへ
            if (phaseCleared) {
                m_phase++;
            }
        }
    }

    // ダミーの更新
    m_dummy.Update(0, 0, 0, 0, false);
}

// 画面に出力する文字を決める関数
std::wstring Tutorial::GetTutorialMessage() const {
    if (m_phase == 0) return L"STEP 1: よこ4つ を つくって けそう！";
    if (m_phase == 1) return L"STEP 2: ななめ4つ を つくって けそう！";
    if (m_phase == 2) return L"STEP 3: ピラミッド を つくって けそう！";
    if (m_phase >= 3) return L"チュートリアル クリア！";
    return L"";
}