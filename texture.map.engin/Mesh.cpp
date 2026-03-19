
#include "Mesh.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm> // （std::replaceを使うため）



using namespace DirectX;

Mesh::Mesh() : m_vertexCount(0) {}
Mesh::~Mesh() {}

// Mesh.cpp の LoadOBJ メソッド内を修正

bool Mesh::LoadOBJ(ID3D11Device* device, const std::string& filename) {
    std::vector<XMFLOAT3> temp_positions;
    std::vector<XMFLOAT2> temp_uvs;
    std::vector<XMFLOAT3> temp_normals; //法線データを一時保存する配列
    std::vector<Vertex> vertices;

    std::ifstream file(filename);
    if (!file.is_open()) return false;

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "v") {
            XMFLOAT3 pos;
            iss >> pos.x >> pos.y >> pos.z;
            pos.z *= -1.0f;
            temp_positions.push_back(pos);
        }
        else if (type == "vt") {
            XMFLOAT2 uv;
            iss >> uv.x >> uv.y;
            uv.y = 1.0f - uv.y;
            temp_uvs.push_back(uv);
        }
        else if (type == "vn") { //法線データの読み込み
            XMFLOAT3 normal;
            iss >> normal.x >> normal.y >> normal.z;
            normal.z *= -1.0f; // Z座標の反転に合わせて法線の向きも反転させる
            temp_normals.push_back(normal);
        }
        // Mesh.cpp の LoadOBJ 内

        else if (type == "f") {
            std::string vertexData;
            std::vector<Vertex> faceVertices;

            // 行の最後まですべての頂点を読み込む（四角形や多角形にも対応）
            while (iss >> vertexData) {
                int v = 0, vt = 0, vn = 0;
                std::stringstream viss(vertexData);
                std::string val;

                // スラッシュ(/)で区切って安全に読み込む（UVがない v//vn 形式でもエラーにならない）
                std::getline(viss, val, '/');
                if (!val.empty()) v = std::stoi(val);

                std::getline(viss, val, '/');
                if (!val.empty()) vt = std::stoi(val);

                std::getline(viss, val, '/');
                if (!val.empty()) vn = std::stoi(val);

                Vertex vertex;
                // 頂点座標 (必須)
                if (v > 0 && v <= temp_positions.size()) {
                    vertex.pos = temp_positions[v - 1];
                }

                // UV座標 (無い場合は仮に 0,0 を入れる)
                if (vt > 0 && vt <= temp_uvs.size()) {
                    vertex.uv = temp_uvs[vt - 1];
                }
                else {
                    vertex.uv = DirectX::XMFLOAT2(0.0f, 0.0f);
                }

                // 法線 (無い場合は仮の手前向きの法線を入れる)
                if (vn > 0 && vn <= temp_normals.size()) {
                    vertex.normal = temp_normals[vn - 1];
                }
                else {
                    vertex.normal = DirectX::XMFLOAT3(0.0f, 0.0f, -1.0f);
                }

                faceVertices.push_back(vertex);
            }

            // 読み込んだ頂点を三角形に分割して登録する（ポリゴン分割）
            // 3頂点なら1回、4頂点(四角形)なら2回ループします
            for (size_t i = 1; i < faceVertices.size() - 1; ++i) {
                // ★重要: 面の裏返りを直す処理
                // Maya(右手系) -> DirectX(左手系) への変換のため、
                // 頂点の登録順を 0番 → (i+1)番 → i番 と「逆回り」にします
                vertices.push_back(faceVertices[0]);
                vertices.push_back(faceVertices[i + 1]);
                vertices.push_back(faceVertices[i]);
            }
        }
    }


    m_vertexCount = (UINT)vertices.size();

    // GPUに頂点バッファを作成
    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(Vertex) * m_vertexCount;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = vertices.data();

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