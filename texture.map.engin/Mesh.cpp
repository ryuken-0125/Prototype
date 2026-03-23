
#include "Mesh.h"
#include <iostream>

#undef min
#undef max

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

using namespace DirectX;

Mesh::Mesh() : m_vertexCount(0) {}
Mesh::~Mesh() {}

bool Mesh::LoadModel(ID3D11Device* device, const std::string& filename) {
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(filename,
        aiProcess_Triangulate |
        aiProcess_ConvertToLeftHanded |
        aiProcess_GenNormals |
        aiProcess_JoinIdenticalVertices |
        aiProcess_PreTransformVertices);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        //画面にエラー内容のポップアップを出すようにする
        std::string errorMsg = "モデル読み込みエラー:\n" + std::string(importer.GetErrorString());
        MessageBoxA(nullptr, errorMsg.c_str(), "FBXエラー", MB_OK | MB_ICONERROR);
        return false;
    }

    std::vector<Vertex> allVertices;

    // =========================================================
    // ★修正: FBX内の「すべての部品（メッシュ）」をループして合体させる
    for (unsigned int m = 0; m < scene->mNumMeshes; m++) {
        aiMesh* mesh = scene->mMeshes[m];
        std::vector<Vertex> tempVertices;

        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex;
            vertex.pos.x = mesh->mVertices[i].x;
            vertex.pos.y = mesh->mVertices[i].y;
            vertex.pos.z = mesh->mVertices[i].z;

            if (mesh->HasNormals()) {
                vertex.normal.x = mesh->mNormals[i].x;
                vertex.normal.y = mesh->mNormals[i].y;
                vertex.normal.z = mesh->mNormals[i].z;
            }
            else {
                vertex.normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
            }

            if (mesh->HasTextureCoords(0)) {
                vertex.uv.x = mesh->mTextureCoords[0][i].x;
                vertex.uv.y = mesh->mTextureCoords[0][i].y;
            }
            else {
                vertex.uv = XMFLOAT2(0.0f, 0.0f);
            }
            tempVertices.push_back(vertex);
        }

        // 面のインデックスから頂点を順番に登録
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++) {
                allVertices.push_back(tempVertices[face.mIndices[j]]);
            }
        }
    }
    // =========================================================

    m_vertexCount = (UINT)allVertices.size();
    if (m_vertexCount == 0) return false;

    // GPUバッファ作成
    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(Vertex) * m_vertexCount;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = allVertices.data();

    HRESULT hr = device->CreateBuffer(&bd, &initData, &m_vertexBuffer);
    return SUCCEEDED(hr);
}

void Mesh::Draw(ID3D11DeviceContext* context) {
    if (m_vertexCount == 0) return;
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->Draw(m_vertexCount, 0);
}

// テクスチャ画像の読み込み
bool Mesh::LoadTexture(ID3D11Device* device, const std::wstring& filename) {
    HRESULT hr = DirectX::CreateWICTextureFromFile(device, nullptr, filename.c_str(), nullptr, m_texture.GetAddressOf());
    if (FAILED(hr)) {
        // 画像が見つからなかった場合は警告を出す
        std::wstring errorMsg = L"画像が見つかりません:\n" + filename;
        MessageBoxW(nullptr, errorMsg.c_str(), L"テクスチャエラー", MB_OK | MB_ICONERROR);
        return false;
    }
    return true;
}

/*

// さらに、既存の Draw メソッドを以下のように書き換えます（1行追加するだけです）
void Mesh::Draw(ID3D11DeviceContext* context) {
    if (m_vertexCount == 0) return;

    // ★追加: もし画像が読み込まれていたら、GPUにセットして貼り付ける
    if (m_texture) {
        context->PSSetShaderResources(0, 1, m_texture.GetAddressOf());
    }

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->Draw(m_vertexCount, 0);
}

*/