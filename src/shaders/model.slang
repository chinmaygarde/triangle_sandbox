cbuffer Uniforms {
    matrix mvp;
    Texture2D texture;
    SamplerState sampler;
}

struct VertexIn {
  float3 position;
  float3 normal;
  float2 textureCoords;
};

struct VertexOut {
  FragmentIn frag : VARYING_FRAGMENT_IN;
  float4 position : SV_Position;
};

struct FragmentIn {
    float2 texture_coords;
    float3 normal;
};

struct FragmentOut {
  float4 color;
};

[Shader("vertex")]
VertexOut VertexMain(VertexIn vtx) {
  VertexOut res;
  res.frag.texture_coords = vtx.textureCoords;
  res.frag.normal = normalize(vtx.normal);
  res.position = mul(mvp, float4(vtx.position, 1.0));
  return res;
}

Sampler2D uTextureSampler;

[Shader("fragment")]
FragmentOut FragmentMain(FragmentIn frg: VARYING_FRAGMENT_IN) : SV_Target {
    FragmentOut out;
    out.color = texture.Sample(sampler, frg.texture_coords);
    return out;
}
