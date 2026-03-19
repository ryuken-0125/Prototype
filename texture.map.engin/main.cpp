
#include "Engine.h"
#include <objbase.h> // COMを使用するために追加

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // 1. COMの初期化（画像を読み込むためのシステムを起動）
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) return -1;

    // エンジンの実体を作成
    Engine myEngine;

    // 初期化に成功したらループを開始する
    if (myEngine.Init(hInstance,1920, 1080)) {
        myEngine.Run();
    }

    // 2. プログラム終了時にCOMを安全に終了させる
    CoUninitialize();

    return 0;
}