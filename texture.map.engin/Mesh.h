#pragma once

#include <windows.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <string>
#include <vector>

#define NOMINMAX

struct Vertex
{
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT2 uv;
    DirectX::XMFLOAT3 normal; // 面の向き（法線ベクトル）
};

// Assimpの構造体を事前宣言（ヘッダーを軽くするため）
struct aiMesh;
struct aiScene;

class Mesh
{
public:
    Mesh();
    ~Mesh();

    //名前を LoadOBJ から LoadModel に変更（FBX等何でも読めます）
    bool LoadModel(ID3D11Device* device, const std::string& filename);

    //描画する
    void Draw(ID3D11DeviceContext* context);

private:
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;
    UINT m_vertexCount;

    //Assimp用の内部読み込み関数
    bool ProcessMesh(ID3D11Device* device, const aiMesh* mesh, const aiScene* scene);
};