#pragma once
#include <windows.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <string>
#include <vector>



struct Vertex {
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT2 uv;
    DirectX::XMFLOAT3 normal; //面の向き（法線ベクトル）
};

class Mesh {
public:
    Mesh();
    ~Mesh();

    // OBJファイルを読み込み、GPUバッファを作成する
    bool LoadOBJ(ID3D11Device* device, const std::string& filename);

    // 描画する
    void Draw(ID3D11DeviceContext* context);

private:
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;
    UINT m_vertexCount;
};