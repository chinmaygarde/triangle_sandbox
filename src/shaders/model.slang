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
};

struct FragmentOut {
  float4 color;
};

[Shader("vertex")]
VertexOut VertexMain(VertexIn vtx) {
  VertexOut res;
  res.frag.texture_coords = vtx.textureCoords;
  res.position = float4(vtx.position, 1.0);
  return res;
}

[Shader("fragment")]
FragmentOut FragmentMain(FragmentIn frg : VARYING_FRAGMENT_IN) : SV_Target {
  FragmentOut res;
  res.color = float4(1.0);
  return res;
}
