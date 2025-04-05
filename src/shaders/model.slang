cbuffer Uniforms {
    matrix mvp;
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

[Shader("fragment")]
FragmentOut FragmentMain(FragmentIn frg : VARYING_FRAGMENT_IN) : SV_Target {
    FragmentOut res;
    var light_direction = float3(0.0, 0.0, 1.0);
    var intensity = dot(frg.normal, light_direction);
    res.color = float4(float3(intensity), 1.0);
    return res;
}
