#include "Mesh.h"
#include <iostream>

#undef min
#undef max

//Assimpの機能を読み込む
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

using namespace DirectX;

Mesh::Mesh() : m_vertexCount(0) {}
Mesh::~Mesh() {}

// =========================================================
// 1. モデルファイルの読み込み（FBX, OBJなんでも対応）
// =========================================================
bool Mesh::LoadModel(ID3D11Device* device, const std::string& filename) {
    Assimp::Importer importer;

    // =========================================================
    //Assimpの読み込みフラグに「PreTransformVertices」を追加します
    const aiScene* scene = importer.ReadFile(filename,
        aiProcess_Triangulate |
        aiProcess_ConvertToLeftHanded |
        aiProcess_GenNormals |
        aiProcess_JoinIdenticalVertices |
        aiProcess_PreTransformVertices);  //回転やスケールを事前に全て適用（焼き付け）する！
    // =========================================================

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "モデル読み込みエラー: " << importer.GetErrorString() << std::endl;
        return false;
    }

    if (scene->mNumMeshes > 0) {
        return ProcessMesh(device, scene->mMeshes[0], scene);
    }

    return false;
}

// =========================================================
// 2. 読み込んだデータの抜き出しとGPUバッファ化
// =========================================================
bool Mesh::ProcessMesh(ID3D11Device* device, const aiMesh* mesh, const aiScene* scene) {
    std::vector<Vertex> vertices; // 一時保存用の頂点リスト

    // 1. 頂点データを一つずつ抜き出す
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;

        // 座標
        vertex.pos.x = mesh->mVertices[i].x;
        vertex.pos.y = mesh->mVertices[i].y;
        vertex.pos.z = mesh->mVertices[i].z;

        // 法線
        if (mesh->HasNormals()) {
            vertex.normal.x = mesh->mNormals[i].x;
            vertex.normal.y = mesh->mNormals[i].y;
            vertex.normal.z = mesh->mNormals[i].z;
        }
        else {
            vertex.normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
        }

        // UV座標 (モデルによっては複数持つことができるが、最初の[0]番目を使う)
        if (mesh->mTextureCoords[0]) {
            vertex.uv.x = mesh->mTextureCoords[0][i].x;
            vertex.uv.y = mesh->mTextureCoords[0][i].y;
        }
        else {
            vertex.uv = XMFLOAT2(0.0f, 0.0f);
        }

        vertices.push_back(vertex);
    }

    // 2. 面（ポリゴン）の情報を元に、頂点を描画する順番に並べ直す
    std::vector<Vertex> orderedVertices;
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        // aiProcess_Triangulate を指定しているので、必ず3角形（インデックス3つ）になっている
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            orderedVertices.push_back(vertices[face.mIndices[j]]);
        }
    }

    m_vertexCount = (UINT)orderedVertices.size();

    // 3. GPUに頂点バッファを作成
    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(Vertex) * m_vertexCount;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = orderedVertices.data();

    HRESULT hr = device->CreateBuffer(&bd, &initData, &m_vertexBuffer);
    return SUCCEEDED(hr);
}

// =========================================================
// 3. 描画
// =========================================================
void Mesh::Draw(ID3D11DeviceContext* context) {
    if (m_vertexCount == 0) return;
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->Draw(m_vertexCount, 0);
}