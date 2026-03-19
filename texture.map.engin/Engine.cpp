#include "Engine.h"
#include "Block.h"


#include <d3dcompiler.h>
#include <random> // 乱数生成用


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
m_posX(0.0f), m_posY(0.0f), m_posZ(0.0f), m_scale(0.3f)
{ 

}

Engine::~Engine() {
    // ComPtrを使用しているため、明示的な解放処理は不要です（自動でクリーンアップされます）
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
    if (!m_frame.LoadOBJ(m_device.Get(), "C:/DX11/sixball/asset/model/frame.obj")) {
        MessageBoxA(m_hwnd, "枠のOBJファイルの読み込みに失敗しました。", "Error", MB_OK);
        return false;
    }

    //4つのブロックの読み込み（ファイルパスをご自身の環境に合わせて修正してください）
    m_blocks[0].LoadOBJ(m_device.Get(), "C:/DX11/sixball/asset/model/Block_R.obj");
    m_blocks[1].LoadOBJ(m_device.Get(), "C:/DX11/sixball/asset/model/Block_G.obj");
    m_blocks[2].LoadOBJ(m_device.Get(), "C:/DX11/sixball/asset/model/Block_B.obj");
    m_blocks[3].LoadOBJ(m_device.Get(), "C:/DX11/sixball/asset/model/Block_Y.obj");

    // 定数バッファの作成（重複していた cbd は1つだけにしました）
    D3D11_BUFFER_DESC cbd = { 0 };
    cbd.Usage = D3D11_USAGE_DEFAULT;
    cbd.ByteWidth = sizeof(ConstantBuffer);
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    m_device->CreateBuffer(&cbd, NULL, &m_constantBuffer);

    // テクスチャとサンプラーの読み込み
    HRESULT hrTex = CreateWICTextureFromFile(m_device.Get(), m_context.Get(), L"C:/DX11/sixball/asset/Texture/rainbow.jpg", nullptr, &m_texture);
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

   

    m_player1.Init(-5.4f, 0.0f); // 変更：-4.0f から -5.4f へ
    m_player2.Init(0.6f, 0.0f); // 変更： 2.0f から  0.6f へ

    return true;
}



//void Engine::Update() {
//    if (!m_fallingBlock.IsActive()) return;
//
//    // --- キーボード入力による左右移動（1回押し判定） ---
//    static bool isLeftPressed = false;
//    static bool isRightPressed = false;
//
//    // 左キー
//    if (GetAsyncKeyState('A') & 0x8000) {
//        if (!isLeftPressed) {
//            // 左にボール0.5個分（直径）移動させる
//            m_fallingBlock.SetX(m_fallingBlock.GetX() - BLOCK_RADIUS * 1.0f);
//            isLeftPressed = true;
//        }
//    }
//    else { isLeftPressed = false; }
//
//    // 右キー
//    if (GetAsyncKeyState('D') & 0x8000) {
//        if (!isRightPressed) {
//            // 右にボール0.5個分（直径）移動させる
//            m_fallingBlock.SetX(m_fallingBlock.GetX() + BLOCK_RADIUS * 1.0f);
//            isRightPressed = true;
//        }
//    }
//    else { isRightPressed = false; }
//
//    // --- はみ出し防止（壁ドン） ---
//    if (m_fallingBlock.GetX() < BASE_X) {
//        m_fallingBlock.SetX(BASE_X);
//    }
//    else if (m_fallingBlock.GetX() > BASE_X + (BOARD_WIDTH - 1) * (BLOCK_RADIUS * 2.0f)) {
//        m_fallingBlock.SetX(BASE_X + (BOARD_WIDTH - 1) * (BLOCK_RADIUS * 2.0f));
//    }
//
//    // --- 落下処理と高速落下 ---
//    float speed = 0.005f;
//    // 下キー長押しで高速落下
//    if (GetAsyncKeyState('S') & 0x8000) {
//        speed = 0.05f;
//    }
//
//    float nextY = m_fallingBlock.GetY() - speed;
//
//    // 衝突判定
//    if (!m_board.IsCollision(m_fallingBlock.GetX(), nextY)) {
//        // 空中ならそのまま落とす
//        m_fallingBlock.SetY(nextY);
//    }
//    else {
//        // ぶつかったらその位置で一番近いマスにロックする
//        m_board.LockBlock(m_fallingBlock.GetX(), m_fallingBlock.GetY(), m_fallingBlock.GetType());
//
//        // 次のボールを降らせる
//        SpawnRandomBlock();
//    }
//}

/*
void Engine::Update() {
    // 1. もし待機中なら、カウントを減らして今回は何もしない（アニメーション効果）
    if (m_waitTimer > 0) {
        m_waitTimer--;
        return;
    }

    // 2. 宙に浮いているボールがあれば、1段だけ滑り落とす
    if (m_board.ApplyGravity()) {
        m_waitTimer = 8; // 8フレーム待つ（この数字で滑り落ちる速度が変わります）
        return;          // 落ちきっていないのでここでストップ
    }

    // 3. 落下がすべて終わったら、ボールが6個繋がっているか(DFS)チェック
    if (m_board.CheckAndErase()) {
        m_waitTimer = 20; // 消えた後、少し余韻を残すために待機
        return;           // 消えたことで浮いたボールがあるかもしれないので、次のフレームで重力判定に戻る
    }

    // 4. 重力処理も消去処理もない（盤面が安定している）場合、プレイヤーの落下操作
    if (m_fallingBlock.IsActive()) {

        // --- 左右移動 (前回と同じ) ---
        static bool isLeftPressed = false;
        static bool isRightPressed = false;

        if (GetAsyncKeyState('A') & 0x8000) {
			if (!isLeftPressed) { m_fallingBlock.SetX(m_fallingBlock.GetX() - BLOCK_RADIUS * 1.0f); isLeftPressed = true; }//1.0fはボール0.5個分（直径）移動させるための係数です。
        }
        else { isLeftPressed = false; }

        if (GetAsyncKeyState('D') & 0x8000) {
            if (!isRightPressed) { m_fallingBlock.SetX(m_fallingBlock.GetX() + BLOCK_RADIUS * 1.0f); isRightPressed = true; }
        }
        else { isRightPressed = false; }

        if (m_fallingBlock.GetX() < BASE_X) m_fallingBlock.SetX(BASE_X);
        if (m_fallingBlock.GetX() > BASE_X + (BOARD_WIDTH - 1) * (BLOCK_RADIUS * 2.0f)) m_fallingBlock.SetX(BASE_X + (BOARD_WIDTH - 1) * (BLOCK_RADIUS * 2.0f));

        // --- 落下処理 ---
        float speed = 0.01f;
        if (GetAsyncKeyState('S') & 0x8000) speed = 0.05f;
        float nextY = m_fallingBlock.GetY() - speed;

        if (!m_board.IsCollision(m_fallingBlock.GetX(), nextY)) {
            m_fallingBlock.SetY(nextY); // 空中
        }
        else {
            // 着地したら盤面に固定
            m_board.LockBlock(m_fallingBlock.GetX(), m_fallingBlock.GetY(), m_fallingBlock.GetType());
            m_fallingBlock.SetInactive(); // ★重要：現在落下中のブロックを「無し」にする

            // 着地直後にすぐ CheckAndErase や ApplyGravity が動くように、タイマーを少しセットする
            m_waitTimer = 5;
        }
    }
    else {
        // 全ての連鎖が終わり、現在落下中のボールもないなら、新しいボールを上から出す
        SpawnRandomBlock();
    }
}

*/

void Engine::Update() 
{
    // プレイヤー1の更新 (A, D, Sキーで操作、CPUではない=false)
    m_player1.Update('A', 'D', 'S', false);

    // プレイヤー2の更新 (矢印キーで操作、CPUモード=true)
    // ★ ここを false にすれば「友達と2人プレイ」、true にすれば「CPU対戦」になります！
    m_player2.Update(VK_LEFT, VK_RIGHT, VK_DOWN, true);
}


/*
void Engine::SpawnRandomBlock() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> typeDist(0, 3);
    std::uniform_int_distribution<int> colDist(0, BOARD_WIDTH - 1);

    int randomType = typeDist(gen);
    int randomCol = colDist(gen); // 0〜5 のランダムな列

    // 選ばれたマスの列のX座標と、画面の上のほうのY座標を取得
    float startX = m_board.GetX(randomCol, BOARD_HEIGHT - 1);
    float startY = m_board.GetY(BOARD_HEIGHT - 1) + 2.0f; // 少し上から降らせる

    m_fallingBlock.Spawn(randomType, startX, startY);
}
*/

/*
void Engine::Draw() {
    float clearColor[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
    m_context->ClearRenderTargetView(m_renderTargetView.Get(), clearColor);

    //深度バッファを毎フレームクリアする（1.0f が一番奥という意味）
    m_context->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

    //第3引数に m_depthStencilView を渡す
    m_context->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());

    // 行列計算（重複していた cb は1つだけにしました）
    XMMATRIX mScale = XMMatrixScaling(m_scale, m_scale, m_scale);

    XMMATRIX mRot = XMMatrixRotationY(m_angle);
    XMMATRIX mTrans = XMMatrixTranslation(m_posX, m_posY, m_posZ);

    XMMATRIX mWorld = mScale * mRot * mTrans;
    // カメラ位置
    XMMATRIX mView = XMMatrixLookAtLH(XMVectorSet(0.0f, 3.0f, -15.0f, 0.0f), XMVectorSet(0.0f, 3.0f, 0.0f, 0.0f), XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
    XMMATRIX mProjection = XMMatrixPerspectiveFovLH(XM_PIDIV4, (float)m_width / (float)m_height, 0.01f, 100.0f);

    // Enginepp.cpp の Draw() 内の ConstantBuffer 更新部分を修正
    ConstantBuffer cb = {}; // 初期化して安全にする
    cb.mWorldViewProj = XMMatrixTranspose(mWorld * mView * mProjection);
    cb.mWorld = XMMatrixTranspose(mWorld); //モデルの回転を法線にも適用するため

    //光の設定（例として、斜め上・奥から手前へ向かって照らす白い光）
    cb.vLightDir = XMFLOAT4(1.0f, -1.0f, 1.0f, 0.0f);
    cb.vLightColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

    XMFLOAT4 blockColors[4] = 
    {
            XMFLOAT4(1.0f, 0.2f, 0.2f, 1.0f), // 赤
            XMFLOAT4(0.2f, 1.0f, 0.2f, 1.0f), // 緑
            XMFLOAT4(0.2f, 0.4f, 1.0f, 1.0f), // 青
            XMFLOAT4(1.0f, 1.0f, 0.2f, 1.0f)  // 黄
    };

    m_context->UpdateSubresource(m_constantBuffer.Get(), 0, NULL, &cb, 0, 0);

    // シェーダーとテクスチャのセット
    // シェーダーとテクスチャのセット
    m_context->IASetInputLayout(m_inputLayout.Get());
    m_context->VSSetShader(m_vertexShader.Get(), NULL, 0);
    m_context->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

    m_context->PSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

    m_context->PSSetShaderResources(0, 1, m_texture.GetAddressOf());
    m_context->PSSetSamplers(0, 1, m_samplerState.GetAddressOf());
    m_context->PSSetShader(m_pixelShader.Get(), NULL, 0);

    // --- ① 枠の描画 ---
    XMMATRIX mScaleFrame = XMMatrixScaling(m_scale, m_scale, m_scale);
    XMMATRIX mRotFrame = XMMatrixRotationY(m_angle); // 枠も回す場合
    XMMATRIX mTransFrame = XMMatrixTranslation(m_posX, m_posY, m_posZ);
    XMMATRIX mWorldFrame = mScaleFrame * mRotFrame * mTransFrame;

    cb.vColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

    cb.mWorldViewProj = XMMatrixTranspose(mWorldFrame * mView * mProjection);
    cb.mWorld = XMMatrixTranspose(mWorldFrame);
    m_context->UpdateSubresource(m_constantBuffer.Get(), 0, NULL, &cb, 0, 0); // ここでGPUへ送信
    m_frame.Draw(m_context.Get());


// --- ② 落下中ブロックの描画 ---

    if (m_fallingBlock.IsActive()) {
        int type = m_fallingBlock.GetType();

        // Blockクラスが持っているX, Y座標を使う
        XMMATRIX mScaleBlock = XMMatrixScaling(m_scale, m_scale, m_scale);
        XMMATRIX mRotBlock = XMMatrixRotationY(m_angle); // 回さないなら消してもOK
        XMMATRIX mTransBlock = XMMatrixTranslation(m_fallingBlock.GetX(), m_fallingBlock.GetY(), 0.0f);
        XMMATRIX mWorldBlock = mScaleBlock * mRotBlock * mTransBlock;

        cb.vColor = blockColors[type];

        cb.mWorldViewProj = XMMatrixTranspose(mWorldBlock * mView * mProjection);
        cb.mWorld = XMMatrixTranspose(mWorldBlock);
        m_context->UpdateSubresource(m_constantBuffer.Get(), 0, NULL, &cb, 0, 0);

        // 種類(type)に応じたモデルを描画
        m_blocks[type].Draw(m_context.Get());
    }

    // --- ③ 盤面に固定されたブロック群の描画 ---
    for (int r = 0; r < BOARD_HEIGHT; ++r) {
        for (int c = 0; c < BOARD_WIDTH; ++c) {
            int blockType = m_board.GetBlockType(c, r);
            if (blockType != -1) {
                // ★ GetXFromColなどを 新しい GetX, GetY に書き換えます
                float posX = m_board.GetX(c, r);
                float posY = m_board.GetY(r);

                XMMATRIX mScaleBlock = XMMatrixScaling(m_scale, m_scale, m_scale);
                XMMATRIX mRotBlock = XMMatrixRotationY(m_angle);
                XMMATRIX mTransBlock = XMMatrixTranslation(posX, posY, 0.0f);
                XMMATRIX mWorldBlock = mScaleBlock * mRotBlock * mTransBlock;

                cb.vColor = blockColors[blockType];

                cb.mWorldViewProj = XMMatrixTranspose(mWorldBlock * mView * mProjection);
                cb.mWorld = XMMatrixTranspose(mWorldBlock);
                m_context->UpdateSubresource(m_constantBuffer.Get(), 0, NULL, &cb, 0, 0);

                m_blocks[blockType].Draw(m_context.Get());
            }
        }
    }

    m_swapChain->Present(1, 0);
}
*/

void Engine::Draw() {
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

    XMFLOAT4 blockColors[4] = {
        XMFLOAT4(1.0f, 0.2f, 0.2f, 1.0f), // 赤
        XMFLOAT4(0.2f, 1.0f, 0.2f, 1.0f), // 緑
        XMFLOAT4(0.2f, 0.4f, 1.0f, 1.0f), // 青
        XMFLOAT4(1.0f, 1.0f, 0.2f, 1.0f)  // 黄
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

        XMMATRIX mScaleFrame = XMMatrixScaling(m_scale, m_scale, m_scale);
        XMMATRIX mRotFrame = XMMatrixRotationY(m_angle);
        XMMATRIX mTransFrame = XMMatrixTranslation(framePosX, m_posY, m_posZ);
        XMMATRIX mWorldFrame = mScaleFrame * mRotFrame * mTransFrame;

        cb.vColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f); // 枠は元の色(白)
        cb.mWorldViewProj = XMMatrixTranspose(mWorldFrame * mView * mProjection);
        cb.mWorld = XMMatrixTranspose(mWorldFrame);
        m_context->UpdateSubresource(m_constantBuffer.Get(), 0, NULL, &cb, 0, 0);
        m_frame.Draw(m_context.Get());

        // --- ② 落下中ブロックの描画 ---
        if (current_player->m_fallingBlock.IsActive()) {
            int type = current_player->m_fallingBlock.GetType();

            XMMATRIX mScaleBlock = XMMatrixScaling(m_scale, m_scale, m_scale);
            XMMATRIX mRotBlock = XMMatrixRotationY(m_angle);
            XMMATRIX mTransBlock = XMMatrixTranslation(current_player->m_fallingBlock.GetX(), current_player->m_fallingBlock.GetY(), 0.0f);
            XMMATRIX mWorldBlock = mScaleBlock * mRotBlock * mTransBlock;

            cb.vColor = blockColors[type];
            cb.mWorldViewProj = XMMatrixTranspose(mWorldBlock * mView * mProjection);
            cb.mWorld = XMMatrixTranspose(mWorldBlock);
            m_context->UpdateSubresource(m_constantBuffer.Get(), 0, NULL, &cb, 0, 0);

            m_blocks[type].Draw(m_context.Get());
        }

        // --- ③ 盤面に固定されたブロック群の描画 ---
        for (int r = 0; r < BOARD_HEIGHT; ++r) {
            for (int c = 0; c < BOARD_WIDTH; ++c) {
                int blockType = current_player->m_board.GetBlockType(c, r);
                if (blockType != -1) {
                    float posX = current_player->m_board.GetX(c, r);
                    float posY = current_player->m_board.GetY(r);

                    XMMATRIX mScaleBlock = XMMatrixScaling(m_scale, m_scale, m_scale);
                    XMMATRIX mRotBlock = XMMatrixRotationY(m_angle);
                    XMMATRIX mTransBlock = XMMatrixTranslation(posX, posY, 0.0f);
                    XMMATRIX mWorldBlock = mScaleBlock * mRotBlock * mTransBlock;

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

void Engine::Run() {
    MSG msg = { 0 };
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            Update(); // データの更新
            Draw();   // 画面の描画
        }
    }
}