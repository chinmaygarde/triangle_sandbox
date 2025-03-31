
struct VertexIn {
  float4 position: POSITION;
  float4 color: COLOR;
};

struct FragmentIn {
  float4 color;
};

struct VertexOut {
  FragmentIn fragment_in: USER_FRAGMENT;
  float4 position: SV_Position;
};

struct FragmentOut {
  float4 color;
};

[Shader("vertex")]
VertexOut VertexMain(VertexIn vtx) {
  VertexOut result;
  result.position = vtx.position;
  result.fragment_in.color = vtx.color;
  return result;
}

[Shader("fragment")]
FragmentOut FragmentMain(FragmentIn frag: USER_FRAGMENT) : SV_Target {
  FragmentOut result;
  result.color = frag.color;
  return result;
}

// Compute Setup

RWTexture2D<uint4> uRWTexture;

[Shader("compute")]
[NumThreads(1,1,1)]
void ComputeAdder(uint3 thread_id: SV_DispatchThreadID) {
    uRWTexture[thread_id.xy] = uint4(255, 0, 255, 255);
}

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
