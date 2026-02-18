Texture2D texDiffuse : register(t0);
SamplerState samplerDiffuse : register(s0);

cbuffer CONSTANT_BUFFER_SPRITE : register(b0)
{
    float2 g_ScreenRes;
    float2 g_Pos;
    float2 g_Scale;
    float2 g_TexSize;
    float2 g_TexSamplePos;
    float2 g_TexSampleSize;
    float g_Z;
    float g_Alpha;
    float g_Reserved0;
    float g_Reserved1;
};

struct VSInput
{
    float3 Pos : POSITION0;
    float4 color : COLOR0;
    float2 TexCoord : TEXCOORD0;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 TexCoord : TEXCOORD0;
};

PSInput VSMain(VSInput input)
{
    PSInput result = (PSInput) 0;

    float2 scale = (g_TexSize / g_ScreenRes) * g_Scale;
    float2 offset = (g_Pos / g_ScreenRes);
    float2 pos = input.Pos.xy * scale + offset;

    result.position = float4(pos * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), g_Z, 1.0f);

    float2 texScale = (g_TexSampleSize / g_TexSize);
    float2 texOffset = (g_TexSamplePos / g_TexSize);
    result.TexCoord = input.TexCoord * texScale + texOffset;

    result.color = input.color;
    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float4 texColor = texDiffuse.Sample(samplerDiffuse, input.TexCoord);
    return texColor * input.color;
}
