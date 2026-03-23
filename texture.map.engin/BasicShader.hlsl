
cbuffer cb : register(b0)
{
    matrix WVP;
    matrix World;
    float4 LightDir;
    float4 LightColor;
    float4 Color; // C++から送られてくる6色
}

// テクスチャは使用しませんが、C++側とのエラーを防ぐため宣言だけ残します
Texture2D tex : register(t0);
SamplerState smp : register(s0);

struct VS_IN
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
};

struct PS_IN
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
};

PS_IN VSMain(VS_IN input)
{
    PS_IN output;
    output.pos = mul(float4(input.pos, 1.0f), WVP);
    output.uv = input.uv;
    output.normal = normalize(mul(input.normal, (float3x3) World));
    return output;
}

float4 PSMain(PS_IN input) : SV_Target
{
    float3 normal = normalize(input.normal);
    float3 lightDir = normalize(LightDir.xyz);

    // 光の当たり具合を計算
    float diffuse = max(0.0f, dot(normal, -lightDir));
    float3 ambient = float3(0.3f, 0.3f, 0.3f);

    // ★修正: 画像(テクスチャ)の色を使わず、純粋に Color と 光 だけで色を作ります
    float3 finalColor = Color.rgb * (LightColor.rgb * diffuse + ambient);
    
    return float4(finalColor, Color.a);
}