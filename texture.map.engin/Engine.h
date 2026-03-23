
#pragma once
#include <windows.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <WICTextureLoader.h> //画像読み込み用ヘッダー
#include <random> // 乱数生成用
#include <memory>
#include <SpriteBatch.h> // 2D画像描画用
#include <Xinput.h>      // コントローラー入力用
#include <SpriteFont.h>

#include "Mesh.h"
#include "Block.h"
#include "Board.h"
#include "Player.h"
#include "Tutorial.h"

enum class Scene {
    TITLE,  // タイトル画面
    GAME,    // ゲーム画面
    TUTORIAL
};

// 頂点と定数バッファの構造体
// 頂点は位置と色を持ち、定数バッファはワールドビュー射影行列を持つ。これらはシェーダーで使用されるデータ構造で、頂点バッファや定数バッファに転送される。

// 頂点構造体は、3D空間の位置と色を表すために使用され、定数バッファ構造体は、シェーダーで使用されるワールドビュー射影行列を格納するために使用される。
// これらの構造体は、DirectX 11の描画パイプラインで重要な役割を果たす。
// シェーダーでこれらのデータを受け取ることで、頂点の位置や色を計算し、最終的な描画結果を生成することができる。




// Engine.h の ConstantBuffer 構造体を修正
struct ConstantBuffer 
{
    DirectX::XMMATRIX mWorldViewProj; // WVP行列
    DirectX::XMMATRIX mWorld;         // ワールド行列
    DirectX::XMFLOAT4 vLightDir;      // 光の向き
    DirectX::XMFLOAT4 vLightColor;    // 光の色
    DirectX::XMFLOAT4 vColor;         //オブジェクト固有の色
};

// シンプルなDirectX 11エンジンのクラス定義。ウィンドウの作成、DirectXの初期化、シーンの準備、更新、描画を行う。


class Engine {
public:
    Engine();
    ~Engine();

    bool Init(HINSTANCE hInstance, int width, int height); // 初期化
    void Run();                                            // メインループ実行

private:
    bool InitWindow(HINSTANCE hInstance, int width, int height);
    bool InitDirectX();
    bool InitScene(); // シェーダーや頂点の準備
    void Update();    // 計算（回転など）
    void Draw();      // 描画

    // ブロックをランダムに生成する関数
    //void SpawnRandomBlock();

    // ウィンドウ関連
    HWND m_hwnd;
    int m_width;
    int m_height;

    Player m_player1;
    Player m_player2;

    //Block m_fallingBlock; // 現在落下中のブロック
    //Board m_board;

    //自分で頂点バッファを持たず、Meshクラスに任せる
    Mesh m_frame;       // 枠用のモデル
    Mesh m_blocks[6 ];   // 4種類のブロック（0:R, 1:G, 2:B, 3:Y）

    //  3Dモデルの位置座標
    float m_posX, m_posY, m_posZ;
    // 3Dモデルのスケール（大きさ）

    float m_frameScale;     // フレーム(枠)専用の大きさ
    float m_blockScale;     // Block(卵)専用の大きさ

    Scene m_currentScene; // 現在のシーン

    bool m_isCpuMatch;    // CPU対戦かどうか
    int m_menuCursor;     // コントローラー用のカーソル位置（0: CPU対戦, 1: 2人対戦）

    std::unique_ptr<DirectX::SpriteBatch> m_spriteBatch;
    std::unique_ptr<DirectX::SpriteFont> m_font;

    // DirectX関連のリソース（ComPtrで自動メモリ管理し、軽量化と安全性を両立）
    Microsoft::WRL::ComPtr<ID3D11Device> m_device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_context;
    Microsoft::WRL::ComPtr<IDXGISwapChain> m_swapChain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_renderTargetView;

    // 3D描画用のリソース
    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_constantBuffer;

    //テクスチャとサンプラーの変数
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_texture;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> m_samplerState;
        
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_rasterizerState;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_depthStencilState;    

    //深度バッファ（奥のものを隠すためのデータ）
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_depthStencilView;

    // タイトル画面用の画像（テクスチャ）
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_texTitleBg;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_texBtnCpu;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_texBtn2P;

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_texBtnTutorial; // タイトル画面用
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_texBtnBack;     // チュートリアル左下用

   // int m_waitTimer; // アニメーション待機用タイマー

    float m_angle; // 回転角度

    void UpdateTitle();
    void UpdateGame();
    void DrawTitle();
    void DrawGame();

    Tutorial m_tutorial;
    void UpdateTutorial();
    void DrawTutorial();

};