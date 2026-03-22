// Tutorial.h の中身を以下のように修正します

#pragma once
#include "Player.h"
#include <string> // ★追加: 文字列を扱うため

class Tutorial {
public:
    Tutorial();
    void Init();
    void Update(int leftKey, int rightKey, int downKey, int rotateKey);

    // ★追加: 現在の進行状態の取得
    int GetPhase() const { return m_phase; }

    // ★追加: 画面に表示するメッセージの取得
    std::wstring GetTutorialMessage() const;

    // ★追加: チュートリアルが完全に終わったかどうかの判定
    bool IsCompleted() const { return m_phase >= 3 && m_clearTimer <= 0; }

    Player m_player;
    Player m_dummy;

private:
    int m_phase;      // ★追加: 現在のステップ (0:横, 1:斜め, 2:ピラミッド, 3:クリア)
    int m_clearTimer; // ★追加: クリア後のタイトル遷移までの待ち時間
};