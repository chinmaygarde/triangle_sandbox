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
