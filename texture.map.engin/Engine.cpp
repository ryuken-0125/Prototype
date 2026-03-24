#include "Engine.h"
#include "Block.h"
#include "Player.h"
#include "Attack.h"

#include <d3dcompiler.h>
#include <random> // 乱数生成用
#include <chrono>
#include <string>
#include <thread>
#include <windows.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "xinput.lib") //コントローラー用
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;
using Microsoft::WRL::ComPtr;

// ウィンドウプロシージャ（OSのメッセージ受け取り）
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
// コンストラクタでモデルの初期位置（原点）を設定
Engine::Engine() : m_hwnd(nullptr), m_width(800), m_height(600), m_angle(0.0f),
m_posX(0.0f), m_posY(0.0f), m_posZ(0.0f), m_frameScale(0.3f), m_blockScale(0.166f)
{ 

}

Engine::~Engine() {
    //ComPtrを使用しているため、明示的な解放処理は不要です（自動でクリーンアップされます）
  }



bool Engine::Init(HINSTANCE hInstance, int width, int height) {
    if (!InitWindow(hInstance, width, height)) return false;
    if (!InitDirectX()) return false;
    if (!InitScene()) return false;
    return true;
}

bool Engine::InitWindow(HINSTANCE hInstance, int width, int height) {
    m_width = width;
    m_height = height;
    const wchar_t CLASS_NAME[] = L"GameEngineClass";
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    RegisterClass(&wc);

    m_hwnd = CreateWindowEx(0, CLASS_NAME, L"sixball",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, m_width, m_height, NULL, NULL, hInstance, NULL);
    
    if (!m_hwnd) return false;
    ShowWindow(m_hwnd, SW_SHOW);
    return true;
}

bool Engine::InitDirectX() {
    DXGI_SWAP_CHAIN_DESC scd = { 0 };
    scd.BufferCount = 1;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferDesc.Width = m_width;
    scd.BufferDesc.Height = m_height;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = m_hwnd;
    scd.SampleDesc.Count = 1;
    scd.Windowed = TRUE;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0,
        D3D11_SDK_VERSION, &scd, &m_swapChain, &m_device, NULL, &m_context);
    if (FAILED(hr)) return false;

    ComPtr<ID3D11Texture2D> backBuffer;
    m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer);
    m_device->CreateRenderTargetView(backBuffer.Get(), NULL, &m_renderTargetView);

    //深度バッファの作成
    D3D11_TEXTURE2D_DESC depthDesc = {};
    depthDesc.Width = m_width;
    depthDesc.Height = m_height;
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    ComPtr<ID3D11Texture2D> depthBuffer;
    m_device->CreateTexture2D(&depthDesc, NULL, &depthBuffer);
    m_device->CreateDepthStencilView(depthBuffer.Get(), NULL, &m_depthStencilView);
    //
    //ラスタライザステート（ポリゴンの両面を描画する設定）
    D3D11_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.CullMode = D3D11_CULL_NONE; // NONEにすることで裏面も強制的に描画
    rasterDesc.FrontCounterClockwise = FALSE;
    m_device->CreateRasterizerState(&rasterDesc, &m_rasterizerState);
    m_context->RSSetState(m_rasterizerState.Get());

    //深度ステンシルステート（奥のものを隠すZバッファのルールを強制）
    D3D11_DEPTH_STENCIL_DESC dssDesc = {};
    dssDesc.DepthEnable = TRUE;
    dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dssDesc.DepthFunc = D3D11_COMPARISON_LESS;
    m_device->CreateDepthStencilState(&dssDesc, &m_depthStencilState);
    // Drawメソッド内で毎フレームセットするため、ここでのセットに加えてDraw内でも呼び出す


    D3D11_VIEWPORT viewport = { 0.0f, 0.0f, (float)m_width, (float)m_height, 0.0f, 1.0f };
    m_context->RSSetViewports(1, &viewport);
    return true;
}

bool Engine::InitScene() {
    ComPtr<ID3DBlob> vsBlob, psBlob, errorBlob;
    HRESULT hr;

    hr = D3DCompileFromFile(L"BasicShader.hlsl", NULL, NULL, "VSMain", "vs_5_0", 0, 0, &vsBlob, &errorBlob);
    if (FAILED(hr)) { if (errorBlob) MessageBoxA(m_hwnd, (char*)errorBlob->GetBufferPointer(), "VS Error", MB_OK); return false; }

    hr = D3DCompileFromFile(L"BasicShader.hlsl", NULL, NULL, "PSMain", "ps_5_0", 0, 0, &psBlob, &errorBlob);
    if (FAILED(hr)) { if (errorBlob) MessageBoxA(m_hwnd, (char*)errorBlob->GetBufferPointer(), "PS Error", MB_OK); return false; }

    m_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), NULL, &m_vertexShader);
    m_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), NULL, &m_pixelShader);

    // Enginepp.cpp の InitScene() 内の layout 配列を修正
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 }, //20バイト目から法線データを読み込む
    };
    //第2引数の要素数を 2 から 3 に変更する
    m_device->CreateInputLayout(layout, 3, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_inputLayout);

    //四角形を作るための Vertex vertices[] = { ... } と、頂点バッファの作成コードをここから削除しました

    // 3Dモデル(.obj)の読み込み
// ★変更: 枠のモデル読み込み（ファイルパスは現在成功している枠のものにしてください）
    if (!m_frame.LoadModel(m_device.Get(),  "asset/model/frame.obj"))
    {
        MessageBoxA(m_hwnd, "枠のOBJファイルの読み込みに失敗しました。", "Error", MB_OK);
        return false;
    }

    //4つのブロックの読み込み（ファイルパスをご自身の環境に合わせて修正してください）
    m_blocks[0].LoadModel(m_device.Get(), "asset/model/aka.fbx");
    m_blocks[1].LoadModel(m_device.Get(), "asset/model/ao.fbx");
    m_blocks[2].LoadModel(m_device.Get(), "asset/model/siro.fbx");
    m_blocks[3].LoadModel(m_device.Get(), "asset/model/pink.fbx");
    m_blocks[4].LoadModel(m_device.Get(), "asset/model/murasaki.fbx");
    m_blocks[5].LoadModel(m_device.Get(), "asset/model/mizuiro.fbx");  

            
    m_blocks[0].LoadTexture(m_device.Get(), L"asset/Texture/rainbow.jpg");
    m_blocks[1].LoadTexture(m_device.Get(), L"asset/Texture/rainbow.jpg");
    m_blocks[2].LoadTexture(m_device.Get(), L"asset/Texture/rainbow.jpg");
    m_blocks[3].LoadTexture(m_device.Get(), L"asset/Texture/rainbow.jpg");
    m_blocks[4].LoadTexture(m_device.Get(), L"asset/Texture/rainbow.jpg");
    m_blocks[5].LoadTexture(m_device.Get(), L"asset/Texture/rainbow.jpg");


    // 定数バッファの作成（重複していた cbd は1つだけにしました）
    D3D11_BUFFER_DESC cbd = { 0 };
    cbd.Usage = D3D11_USAGE_DEFAULT;    
    cbd.ByteWidth = sizeof(ConstantBuffer);
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    m_device->CreateBuffer(&cbd, NULL, &m_constantBuffer);

    // テクスチャとサンプラーの読み込み
    HRESULT hrTex = CreateWICTextureFromFile(m_device.Get(), m_context.Get(), L"asset/Texture/rainbow.jpg", nullptr, &m_texture);
    if (FAILED(hrTex))
    {
        // 読み込みに失敗した場合は警告を出す
        MessageBoxA(m_hwnd, "テクスチャ(test.png)の読み込みに失敗しました。パスを確認してください。", "Error", MB_OK);
        return false;
    }


    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    m_device->CreateSamplerState(&sampDesc, &m_samplerState);

  

        //追加: 2D描画ツールの初期化
    m_spriteBatch = std::make_unique<DirectX::SpriteBatch>(m_context.Get());

    //追加: タイトル画面のUI画像を読み込む
    // （パスはご自身が用意した画像ファイルの場所に書き換えてください）
    CreateWICTextureFromFile(m_device.Get(), m_context.Get(), L"asset/Texture/title.png", nullptr, &m_texTitleBg);
    CreateWICTextureFromFile(m_device.Get(), m_context.Get(), L"asset/Texture/soloUI.png", nullptr, &m_texBtnCpu);
    CreateWICTextureFromFile(m_device.Get(), m_context.Get(), L"asset/Texture/battleUI.png", nullptr, &m_texBtn2P);
    CreateWICTextureFromFile(m_device.Get(), m_context.Get(), L"asset/Texture/tutorial.png", nullptr, &m_texBtnTutorial);
    CreateWICTextureFromFile(m_device.Get(), m_context.Get(), L"asset/Texture/back.png", nullptr, &m_texBtnBack);
 
    try {
        m_font = std::make_unique<DirectX::SpriteFont>(m_device.Get(), L"asset/Font/tutorial_font.spritefont");
    }
    catch (...) {
        // ファイルがない場合は無視する（文字は出ないがゲームは動く）
    }

    m_player1.Init(-5.4f, 0.0f);
    m_player2.Init(0.6f, 0.0f);

    // チュートリアル自体の初期化もしておく
    m_tutorial.Init();

    //追加: シーンの初期状態を設定
    m_currentScene = Scene::TITLE;
    m_menuCursor = 0; // 最初は上のボタンを選択状態にする

    return true;
}

void Engine::Update() {
    if (m_currentScene == Scene::TITLE) {
        UpdateTitle();
    }
    else if (m_currentScene == Scene::GAME) {
        UpdateGame();
    }
}

void Engine::Draw() {
    float clearColor[4] = { 0.0f, 0.2f, 0.4f, 1.0f }; // 背景色
    m_context->ClearRenderTargetView(m_renderTargetView.Get(), clearColor);
    m_context->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
    m_context->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());

    if (m_currentScene == Scene::TITLE) {
        DrawTitle();
    }
    else if (m_currentScene == Scene::GAME) {
        DrawGame();
    }

    m_swapChain->Present(1, 0);
}

void Engine::DrawGame() {
    float clearColor[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
    m_context->ClearRenderTargetView(m_renderTargetView.Get(), clearColor);
    m_context->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
    m_context->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());

    // ★変更: カメラのZ座標を -10.0f から -18.0f に下げて、2つの盤面が画面に収まるように引きます
    XMMATRIX mView = XMMatrixLookAtLH(XMVectorSet(0.0f, 3.0f, -18.0f, 0.0f), XMVectorSet(0.0f, 3.0f, 0.0f, 0.0f), XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
    XMMATRIX mProjection = XMMatrixPerspectiveFovLH(XM_PIDIV4, (float)m_width / (float)m_height, 0.01f, 100.0f);

    ConstantBuffer cb = {};
    cb.vLightDir = XMFLOAT4(1.0f, -1.0f, 1.0f, 0.0f);
    cb.vLightColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

    XMFLOAT4 blockColors[6] =
    {
            XMFLOAT4(1.0f, 0.2f, 0.2f, 1.0f), // 0: 赤
            XMFLOAT4(0.2f, 0.4f, 1.0f, 1.0f), // 1: 青
            XMFLOAT4(0.9f, 0.9f, 0.9f, 1.0f), // 2: 白
            XMFLOAT4(0.8f, 0.2f, 0.8f, 1.0f), // 3: 紫 (混色)
            XMFLOAT4(0.3f, 0.8f, 1.0f, 1.0f), // 4: 水色 (混色)
            XMFLOAT4(1.0f, 0.6f, 0.8f, 1.0f)  // 5: ピンク (混色)
    };
    // シェーダーのセット
    m_context->IASetInputLayout(m_inputLayout.Get());
    m_context->VSSetShader(m_vertexShader.Get(), NULL, 0);
    m_context->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
    m_context->PSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
    m_context->PSSetShaderResources(0, 1, m_texture.GetAddressOf());
    m_context->PSSetSamplers(0, 1, m_samplerState.GetAddressOf());
    m_context->PSSetShader(m_pixelShader.Get(), NULL, 0);

    // ★追加: 2人のプレイヤーを順番に処理するための配列
    Player* players[2] = { &m_player1, &m_player2 };

    for (int p = 0; p < 2; ++p) {
        Player* current_player = players[p];

        // --- ① 枠の描画 ---
        // プレイヤーの基準座標(m_baseX)に合わせて枠も移動させます。
        // （元のプログラムでボールの左端が-2.4fの時、枠が0.0fにあったため、+2.4fずらして中央を合わせます）
        float framePosX = current_player->m_baseX + 2.4f;

        XMMATRIX mScaleFrame = XMMatrixScaling(m_frameScale, m_frameScale, m_frameScale);
        XMMATRIX mRotFrame = XMMatrixRotationY(m_angle);
        XMMATRIX mTransFrame = XMMatrixTranslation(framePosX, m_posY, m_posZ);
        XMMATRIX mWorldFrame = mScaleFrame * mRotFrame * mTransFrame;

        cb.vColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f); // 枠は元の色(白)
        cb.mWorldViewProj = XMMatrixTranspose(mWorldFrame * mView * mProjection);
        cb.mWorld = XMMatrixTranspose(mWorldFrame);
        m_context->UpdateSubresource(m_constantBuffer.Get(), 0, NULL, &cb, 0, 0);
        m_frame.Draw(m_context.Get());

        // --- ② 落下中ブロックの描画 ---
        if (current_player->m_fallingGroup.IsActive()) {
            // グループ内のボールの数だけループして描画する
            for (int i = 0; i < current_player->m_fallingGroup.GetBlockCount(); ++i) {
                int type = current_player->m_fallingGroup.GetType(i);

                XMMATRIX mScaleBlock = XMMatrixScaling(m_blockScale, m_blockScale, m_blockScale);
                XMMATRIX mRotBlock = XMMatrixRotationY(m_angle);
                // m_fallingBlock.GetX() ではなく、GetBlockX(i) を使う
                XMMATRIX mTransBlock = XMMatrixTranslation(current_player->m_fallingGroup.GetBlockX(i), current_player->m_fallingGroup.GetBlockY(i), 0.0f);
                XMMATRIX mWorldBlock = mScaleBlock * mRotBlock * mTransBlock;

                cb.vColor = blockColors[type];
                cb.mWorldViewProj = XMMatrixTranspose(mWorldBlock * mView * mProjection);
                cb.mWorld = XMMatrixTranspose(mWorldBlock);
                m_context->UpdateSubresource(m_constantBuffer.Get(), 0, NULL, &cb, 0, 0);

                m_blocks[type].Draw(m_context.Get());
            }
        }
        // ==========================================================
        //操作不可の攻撃ブロック(Attack)群の描画
        for (const auto& attack : current_player->GetActiveAttacks()) {
            for (int i = 0; i < attack.GetBlockCount(); ++i) {
                int type = attack.GetType(i);

                XMMATRIX mScaleBlock = XMMatrixScaling(m_blockScale, m_blockScale, m_blockScale);
                XMMATRIX mRotBlock = XMMatrixRotationY(m_angle);
                XMMATRIX mTransBlock = XMMatrixTranslation(attack.GetBlockX(i), attack.GetBlockY(i), 0.0f);
                XMMATRIX mWorldBlock = mScaleBlock * mRotBlock * mTransBlock;

                cb.vColor = blockColors[type];
                cb.mWorldViewProj = XMMatrixTranspose(mWorldBlock * mView * mProjection);
                cb.mWorld = XMMatrixTranspose(mWorldBlock);
                m_context->UpdateSubresource(m_constantBuffer.Get(), 0, NULL, &cb, 0, 0);

                m_blocks[type].Draw(m_context.Get());
            }
        }
       
        // --- 固定されたブロックの描画 ---
        for (int r = 0; r < BOARD_HEIGHT; ++r) {
            for (int c = 0; c < BOARD_WIDTH; ++c) {
                int blockType = current_player->m_board.GetBlockType(c, r);
                if (blockType != -1) {

                    // ★変更: m_blockScale ではなく m_blockScale を使うようにします！
                    float drawScale = m_blockScale;

                    int crackedTurns = current_player->m_board.GetCrackedTurns(c, r);
                    if (crackedTurns >= 0) {
                        float pulseSpeed = 0.01f + (crackedTurns * 0.005f);
                        drawScale *= (1.0f + 0.1f * sin(GetTickCount() * pulseSpeed));
                    }

                    XMMATRIX mWorldBlock = XMMatrixScaling(drawScale, drawScale, drawScale) * XMMatrixRotationY(m_angle) * XMMatrixTranslation(current_player->m_board.GetX(c, r), current_player->m_board.GetY(r), 0.0f);

                    cb.vColor = blockColors[blockType];
                    cb.mWorldViewProj = XMMatrixTranspose(mWorldBlock * mView * mProjection);
                    cb.mWorld = XMMatrixTranspose(mWorldBlock);
                    m_context->UpdateSubresource(m_constantBuffer.Get(), 0, NULL, &cb, 0, 0);
                    m_blocks[blockType].Draw(m_context.Get());
                }
            }
        }
    } // forループの終わり（ここで次のプレイヤーの描画へ）

    m_swapChain->Present(1, 0);
}

void Engine::UpdateGame() {
    // それぞれのプレイヤーを更新し、発生した「攻撃リスト」を受け取る
    auto attacks1 = m_player1.Update('A', 'D', 'S', 'R', false);
    auto attacks2 = m_player2.Update(VK_LEFT, VK_RIGHT, VK_DOWN, VK_UP, m_isCpuMatch);

    // 1が攻撃したら、2のキューに入れる
    for (int a : attacks1) m_player2.AddAttack(a);

    // 2が攻撃したら、1のキューに入れる
    for (int a : attacks2) m_player1.AddAttack(a);

    // ==========================================================
    // 負け判定とタイトル画面へのループ処理
    if (m_player1.IsGameOver() || m_player2.IsGameOver()) {
        // どちらか片方でも負けた場合（一番上まで積み上がった場合）
        // 
        // 3秒間停止する
        Sleep(3000);

        // 次の対戦に備えて、両プレイヤーの盤面や攻撃をすべて綺麗にリセットする
        m_player1.Reset();
        m_player2.Reset();

        // シーンを最初のタイトル画面に戻す
        m_currentScene = Scene::TITLE;
    }
    // ==========================================================

}

void Engine::Run() {
    MSG msg = { 0 };

    // --- FPS制御のための設定 ---
    const double targetFPS = 60.0;
    const double targetFrameTime = 1.0 / targetFPS; // 1フレームに割くべき時間（約0.00694秒）

    // 時間計測用の変数（今の時間を記録）
    auto prevTime = std::chrono::high_resolution_clock::now();
    auto fpsUpdateTime = prevTime;
    int frameCount = 0; // 1秒間に何回描画したかを数えるカウンター

    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            // 現在の時間を取得
            auto currentTime = std::chrono::high_resolution_clock::now();

            // 前回の描画から「何秒」経過したかを計算
            std::chrono::duration<double> elapsedTime = currentTime - prevTime;

            // 経過時間が目標フレーム時間（1/144秒）以上になったら、1フレーム分進める
            if (elapsedTime.count() >= targetFrameTime) {
                prevTime = currentTime; // 時間を更新

                Update(); // データの更新
                Draw();   // 画面の描画

                // --- FPSの計算とウィンドウタイトルへの表示処理 ---
                frameCount++;
                std::chrono::duration<double> fpsElapsed = currentTime - fpsUpdateTime;

                // もし1秒以上経過していたら、タイトルバーを更新する
                if (fpsElapsed.count() >= 1.0) {
                    // "sixball - FPS: 144" という文字列を作る
                    std::wstring title = L"sixball - FPS: " + std::to_wstring(frameCount);

                    // ウィンドウのタイトルを書き換える
                    SetWindowText(m_hwnd, title.c_str());

                    // カウンターと時間をリセットして、次の1秒の計測を始める
                    frameCount = 0;
                    fpsUpdateTime = currentTime;
                }
            }
            else {
                // まだ1/144秒経っていない場合は、PCの負荷を下げるためにCPUの処理を少し休ませる（譲る）
                std::this_thread::yield();
            }
        }
    }
}

void Engine::UpdateTitle() {
    // --- 1. コントローラー（XInput）の処理 ---
    XINPUT_STATE xState;
    ZeroMemory(&xState, sizeof(XINPUT_STATE));
    bool isDecided = false;

    // コントローラーが接続されているか確認
    if (XInputGetState(0, &xState) == ERROR_SUCCESS) {
        static WORD lastButtons = 0;
        WORD currentButtons = xState.Gamepad.wButtons; // ← ここで変数が定義されています

        // ★変更: 十字キーの上下でカーソル移動（0:CPU, 1:2P, 2:Tutorial）
        if ((currentButtons & XINPUT_GAMEPAD_DPAD_UP) && !(lastButtons & XINPUT_GAMEPAD_DPAD_UP)) {
            m_menuCursor--;
            if (m_menuCursor < 0) m_menuCursor = 0;
        }
        if ((currentButtons & XINPUT_GAMEPAD_DPAD_DOWN) && !(lastButtons & XINPUT_GAMEPAD_DPAD_DOWN)) {
            m_menuCursor++;
            if (m_menuCursor > 2) m_menuCursor = 2; // 2(チュートリアル)まで動けるように
        }

        // Aボタン（PS4の✕ボタンに相当）で決定
        if ((currentButtons & XINPUT_GAMEPAD_A) && !(lastButtons & XINPUT_GAMEPAD_A)) {
            isDecided = true;
        }
        lastButtons = currentButtons;
    } // ← この波括弧の中でだけ currentButtons と lastButtons が使えます

    // --- 2. マウスの処理 ---
    POINT cursorPos;
    GetCursorPos(&cursorPos);
    ScreenToClient(m_hwnd, &cursorPos); // ウィンドウ内の座標に変換

    // ボタンの当たり判定（画像の配置位置に合わせて調整してください）
    RECT rectCpu = { m_width / 2 - 150, m_height / 2 - 80, m_width / 2 + 150, m_height / 2 - 10 };
    RECT rect2P = { m_width / 2 - 150, m_height / 2 + 10, m_width / 2 + 150, m_height / 2 + 80 };
    RECT rectTut = { m_width / 2 - 150, m_height / 2 + 100, m_width / 2 + 150, m_height / 2 + 170 }; // ★追加: チュートリアルボタン

    // マウスがボタンの上に乗ったらカーソルを合わせる
    if (PtInRect(&rectCpu, cursorPos)) m_menuCursor = 0;
    if (PtInRect(&rect2P, cursorPos))  m_menuCursor = 1;
    if (PtInRect(&rectTut, cursorPos)) m_menuCursor = 2; //

    // 左クリック（1回押し）判定
    static bool isMousePressed = false;
    if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
        if (!isMousePressed && (PtInRect(&rectCpu, cursorPos) || PtInRect(&rect2P, cursorPos) || PtInRect(&rectTut, cursorPos))) {
            isDecided = true;
        }
        isMousePressed = true;
    }
    else {
        isMousePressed = false;
    }

    // --- 3. 決定された場合のシーン遷移 ---
    if (isDecided) {
        if (m_menuCursor == 0 || m_menuCursor == 1) {

            // ★対戦開始時に「必ず」初期位置と初期状態をセットし直す
            m_player1.Init(-5.4f, 0.0f);
            m_player2.Init(0.6f, 0.0f);

            m_isCpuMatch = (m_menuCursor == 0);
            m_currentScene = Scene::GAME;

        }
        else if (m_menuCursor == 2) {
            // チュートリアルが選ばれた場合
            m_tutorial.Init();
            m_currentScene = Scene::TUTORIAL;
        }
    }
}

void Engine::DrawTitle() {
    // 2D描画モード開始
    m_spriteBatch->Begin();

    // 背景の描画 (画面全体)
    if (m_texTitleBg) {
        RECT bgRect = { 0, 0, m_width, m_height };
        m_spriteBatch->Draw(m_texTitleBg.Get(), bgRect, Colors::White);
    }

    // ボタンの色（選択されているボタンは白、選択されていない方は少し暗くする）
    XMVECTORF32 colorCpu = (m_menuCursor == 0) ? Colors::White : Colors::Gray;
    XMVECTORF32 color2P = (m_menuCursor == 1) ? Colors::White : Colors::Gray;
    XMVECTORF32 colorTut = (m_menuCursor == 2) ? Colors::White : Colors::Gray;

    if (m_texBtnTutorial) {
        RECT btnTutRect = { m_width / 2 - 150, m_height / 2 + 100, m_width / 2 + 150, m_height / 2 + 170 };
        m_spriteBatch->Draw(m_texBtnTutorial.Get(), btnTutRect, colorTut);
    }
    // ボタンの描画（位置はUpdateTitleの当たり判定RECTに合わせています）
    if (m_texBtnCpu) {
        RECT btnCpuRect = { m_width / 2 - 150, m_height / 2 - 50, m_width / 2 + 150, m_height / 2 + 20 };
        m_spriteBatch->Draw(m_texBtnCpu.Get(), btnCpuRect, colorCpu);
    }
    if (m_texBtn2P) {
        RECT btn2PRect = { m_width / 2 - 150, m_height / 2 + 50, m_width / 2 + 150, m_height / 2 + 120 };
        m_spriteBatch->Draw(m_texBtn2P.Get(), btn2PRect, color2P);
    }

    // 2D描画モード終了
    m_spriteBatch->End();
}

void Engine::UpdateTutorial() {
    // チュートリアル本編の更新（キーボード操作を渡す）
    m_tutorial.Update('A', 'D', 'S', 'R');

    if (m_tutorial.IsCompleted()) {
        m_currentScene = Scene::TITLE;
        return;
    }   

    // 左下の「戻る」ボタンの判定
    POINT cursorPos;
    GetCursorPos(&cursorPos);
    ScreenToClient(m_hwnd, &cursorPos);

    // 左下の座標（画像サイズに合わせて調整してください）
    RECT rectBack = { 20, m_height - 80, 180, m_height - 20 };

    static bool isMousePressed = false;
    if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
        if (!isMousePressed && PtInRect(&rectBack, cursorPos)) {
            // 戻るボタンが押されたらタイトルに戻る
            m_currentScene = Scene::TITLE;
        }
        isMousePressed = true;
    }
    else {
        isMousePressed = false;
    }
}

void Engine::DrawTutorial() {
    // =========================================================
    // 【3D盤面の描画】 DrawGame() とほぼ同じですが、対象がチュートリアルになります
    // =========================================================
    XMMATRIX mView = XMMatrixLookAtLH(XMVectorSet(0.0f, 3.0f, -18.0f, 0.0f), XMVectorSet(0.0f, 3.0f, 0.0f, 0.0f), XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
    XMMATRIX mProjection = XMMatrixPerspectiveFovLH(XM_PIDIV4, (float)m_width / (float)m_height, 0.01f, 100.0f);

    ConstantBuffer cb = {};
    cb.vLightDir = XMFLOAT4(1.0f, -1.0f, 1.0f, 0.0f);
    cb.vLightColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

    XMFLOAT4 blockColors[6] =
    {
            XMFLOAT4(1.0f, 0.2f, 0.2f, 1.0f), // 0: 赤
            XMFLOAT4(0.2f, 0.4f, 1.0f, 1.0f), // 1: 青
            XMFLOAT4(0.9f, 0.9f, 0.9f, 1.0f), // 2: 白
            XMFLOAT4(0.8f, 0.2f, 0.8f, 1.0f), // 3: 紫 (混色)
            XMFLOAT4(0.3f, 0.8f, 1.0f, 1.0f), // 4: 水色 (混色)
            XMFLOAT4(1.0f, 0.6f, 0.8f, 1.0f)  // 5: ピンク (混色)
    };

    m_context->IASetInputLayout(m_inputLayout.Get());
    m_context->VSSetShader(m_vertexShader.Get(), NULL, 0);
    m_context->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
    m_context->PSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
    m_context->PSSetShaderResources(0, 1, m_texture.GetAddressOf());
    m_context->PSSetSamplers(0, 1, m_samplerState.GetAddressOf());
    m_context->PSSetShader(m_pixelShader.Get(), NULL, 0);

    // ★ 描画対象をチュートリアル用の2人にする
    Player* players[2] = { &m_tutorial.m_player, &m_tutorial.m_dummy };

    for (int p = 0; p < 2; ++p) {
        Player* current_player = players[p];
        // --- 枠の描画 ---
        float framePosX = current_player->m_baseX + 2.4f;
        XMMATRIX mWorldFrame = XMMatrixScaling(m_frameScale, m_frameScale, m_frameScale) * XMMatrixRotationY(m_angle) * XMMatrixTranslation(framePosX, m_posY, m_posZ);
        cb.vColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        cb.mWorldViewProj = XMMatrixTranspose(mWorldFrame * mView * mProjection);
        cb.mWorld = XMMatrixTranspose(mWorldFrame);
        m_context->UpdateSubresource(m_constantBuffer.Get(), 0, NULL, &cb, 0, 0);
        m_frame.Draw(m_context.Get());

        // --- 落下中ブロックの描画 ---
        if (current_player->m_fallingGroup.IsActive()) {
            for (int i = 0; i < current_player->m_fallingGroup.GetBlockCount(); ++i) {
                int type = current_player->m_fallingGroup.GetType(i);
                XMMATRIX mWorldBlock = XMMatrixScaling(m_blockScale, m_blockScale, m_blockScale) * XMMatrixRotationY(m_angle) * XMMatrixTranslation(current_player->m_fallingGroup.GetBlockX(i), current_player->m_fallingGroup.GetBlockY(i), 0.0f);
                cb.vColor = blockColors[type];
                cb.mWorldViewProj = XMMatrixTranspose(mWorldBlock * mView * mProjection);
                cb.mWorld = XMMatrixTranspose(mWorldBlock);
                m_context->UpdateSubresource(m_constantBuffer.Get(), 0, NULL, &cb, 0, 0);
                m_blocks[type].Draw(m_context.Get());
            }
        }

        // --- 操作不可の攻撃ブロックの描画 ---
        for (const auto& attack : current_player->GetActiveAttacks()) {
            for (int i = 0; i < attack.GetBlockCount(); ++i) {
                int type = attack.GetType(i);
                XMMATRIX mWorldBlock = XMMatrixScaling(m_blockScale, m_blockScale, m_blockScale) * XMMatrixRotationY(m_angle) * XMMatrixTranslation(attack.GetBlockX(i), attack.GetBlockY(i), 0.0f);
                cb.vColor = blockColors[type];
                cb.mWorldViewProj = XMMatrixTranspose(mWorldBlock * mView * mProjection);
                cb.mWorld = XMMatrixTranspose(mWorldBlock);
                m_context->UpdateSubresource(m_constantBuffer.Get(), 0, NULL, &cb, 0, 0);
                m_blocks[type].Draw(m_context.Get());
            }
        }

        // --- 固定されたブロックの描画 ---
        for (int r = 0; r < BOARD_HEIGHT; ++r) {
            for (int c = 0; c < BOARD_WIDTH; ++c) {
                int blockType = current_player->m_board.GetBlockType(c, r);
                if (blockType != -1) {

                    // ★変更: m_blockScale ではなく m_blockScale を使うようにします！
                    float drawScale = m_blockScale;

                    int crackedTurns = current_player->m_board.GetCrackedTurns(c, r);
                    if (crackedTurns >= 0) {
                        float pulseSpeed = 0.01f + (crackedTurns * 0.005f);
                        drawScale *= (1.0f + 0.1f * sin(GetTickCount() * pulseSpeed));
                    }

                    XMMATRIX mWorldBlock = XMMatrixScaling(drawScale, drawScale, drawScale) * XMMatrixRotationY(m_angle) * XMMatrixTranslation(current_player->m_board.GetX(c, r), current_player->m_board.GetY(r), 0.0f);

                    cb.vColor = blockColors[blockType];
                    cb.mWorldViewProj = XMMatrixTranspose(mWorldBlock * mView * mProjection);
                    cb.mWorld = XMMatrixTranspose(mWorldBlock);
                    m_context->UpdateSubresource(m_constantBuffer.Get(), 0, NULL, &cb, 0, 0);
                    m_blocks[blockType].Draw(m_context.Get());
                }
            }
        }
    }

    // =========================================================
        // チュートリアルのテキスト描画
    if (m_font) {
        // ★変更: 呼び出す関数の名前を変えます
        std::wstring msg = m_tutorial.GetTutorialMessage();
            
        if (!msg.empty()) {
            DirectX::XMVECTOR origin = m_font->MeasureString(msg.c_str()) / 2.0f;
            DirectX::XMVECTOR textPos = DirectX::XMVectorSet(m_width / 2.0f, 50.0f, 0.0f, 0.0f);

            m_font->DrawString(m_spriteBatch.get(), msg.c_str(), textPos, DirectX::Colors::Yellow, 0.0f, origin, 1.0f);
        }
    }
    // =========================================================

    // =========================================================
    // 【2D UIの描画（戻るボタンなど）】
    // =========================================================
    m_spriteBatch->Begin();
    if (m_texBtnBack) {
        RECT btnBackRect = { 20, m_height - 80, 180, m_height - 20 };
        // マウスが乗っていたら少し明るくする演出
        POINT cursorPos; GetCursorPos(&cursorPos); ScreenToClient(m_hwnd, &cursorPos);
        XMVECTORF32 btnColor = PtInRect(&btnBackRect, cursorPos) ? Colors::White : Colors::LightGray;

        m_spriteBatch->Draw(m_texBtnBack.Get(), btnBackRect, btnColor);
    }
    m_spriteBatch->End();
}