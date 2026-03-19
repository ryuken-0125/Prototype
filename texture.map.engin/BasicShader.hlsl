    
cbuffer cb : register(b0)
{
    matrix WVP;
    matrix World; //ワールド行列
    float4 LightDir; //光の向き
    float4 LightColor; //光の色
    float4 Color; //C++から送られてくる色
}

Texture2D tex : register(t0);
SamplerState smp : register(s0);

struct VS_IN
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL; // 追加
};

struct PS_IN
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL; // 追加
};

PS_IN VSMain(VS_IN input)
{
    PS_IN output;
    
    // 位置座標の変換
    output.pos = mul(float4(input.pos, 1.0f), WVP);
    output.uv = input.uv;
    
    // 法線ベクトルの変換 (モデルが回転したら、面の向きも一緒に回転させる)
    // 回転の要素だけを取り出すために (float3x3) にキャストして掛け算します
    output.normal = mul(input.normal, (float3x3) World);
    
    // 計算誤差を無くすため、ベクトルの長さを1に正規化
    output.normal = normalize(output.normal);

    return output;
}

float4 PSMain(PS_IN input) : SV_Target
{
    // 1. テクスチャの色を取得
    float4 texColor = tex.Sample(smp, input.uv);

    // 2. ピクセルでの法線と光の向きを正規化（長さを1に）
    float3 normal = normalize(input.normal);
    float3 lightDir = normalize(LightDir.xyz);

    // 3. 内積（dot）を使って光の当たり具合を計算
    // 光の向きと逆方向(-lightDir)が法線と一致するときが一番明るい
    float diffuse = dot(normal, -lightDir);
    
    // 4. 光が当たらない裏側はマイナスになるため、0で切り捨てる
    diffuse = max(0.0f, diffuse);

    // 5. 環境光（アンビエント）の設定
    // 影になっている部分が真っ黒にならないように、ベースとなる僅かな光を足します
    float3 ambient = float3(0.3f, 0.3f, 0.3f); // 0.3くらいのグレー

    // 6. 最終的な色の計算 = テクスチャ色 * (光の色 * 光の強さ + 環境光)
    float3 finalColor = texColor.rgb * (LightColor.rgb * diffuse + ambient);

    return float4(finalColor, texColor.a) * Color;
}