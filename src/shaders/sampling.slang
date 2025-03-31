struct SamplingFragmentIn {
    float2 uv;
};

struct SamplingVertexOut {
    float4 position : SV_Position;
    SamplingFragmentIn frag : SAMPLING_FRAGMENT_IN;
};

struct SamplingVertexIn {
    float4 position;
    float2 uv;
};

[Shader("vertex")]
SamplingVertexOut SamplingVertexMain(SamplingVertexIn vtx) {
    SamplingVertexOut out;
    out.position = vtx.position;
    out.frag.uv = vtx.uv;
    return out;
}

Texture2D uTexture : register(t0);
SamplerState uSamplerState : register(s0);

[Shader("fragment")]
float4 SamplingFragmentMain(SamplingFragmentIn frag: SAMPLING_FRAGMENT_IN) : SV_Target {
    return uTexture.Sample(uSamplerState, frag.uv);
}
