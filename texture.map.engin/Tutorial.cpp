#include "Tutorial.h"

Tutorial::Tutorial() {}

void Tutorial::Init() {
    // 状態を完全リセット
    m_player.Reset();
    m_dummy.Reset();

    // プレイヤーとダミーを左右に配置
    m_player.Init(-5.4f, 0.0f);
    m_dummy.Init(0.6f, 0.0f);

    // 相手をダミーモード（自分からボールを出さないモード）に設定
    m_dummy.SetDummyMode(true);
}

void Tutorial::Update(int leftKey, int rightKey, int downKey, int rotateKey) {
    // 1. プレイヤーの更新
    auto attacks = m_player.Update(leftKey, rightKey, downKey, rotateKey, false);

    // 2. プレイヤーからの攻撃をダミーに送る（攻撃の仕方を教えるため）
    for (int a : attacks) {
        m_dummy.AddAttack(a);
    }

    // 3. ダミーの更新（キー入力は全て 0 で操作させない。攻撃を食らう処理だけ行う）
    m_dummy.Update(0, 0, 0, 0, false);

    // ※ステップ2以降で、ここに「特定の形を作ったか？」などの進行チェックを追加していきます
}