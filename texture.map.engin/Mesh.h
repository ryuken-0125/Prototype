// Mesh.h を以下のように修正します

#pragma once

#define NOMINMAX 
#include <windows.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <string>
#include <vector>

//テクスチャ読み込み用のヘッダー
#include <WICTextureLoader.h> 

struct Vertex {
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT2 uv;
    DirectX::XMFLOAT3 normal;
};

class Mesh {
public:
    Mesh();
    ~Mesh();

    bool LoadModel(ID3D11Device* device, const std::string& filename);

    //画像を読み込むための関数
    bool LoadTexture(ID3D11Device* device, const std::wstring& filename);

    void Draw(ID3D11DeviceContext* context);

private:
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;
    UINT m_vertexCount;

    //読み込んだ画像データを保存する変数
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_texture;
};