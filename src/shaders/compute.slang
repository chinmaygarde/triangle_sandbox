RWTexture2D<float4> uRWTexture;

[Shader("compute")]
void ComputeAdder(uint3 thread_id: SV_DispatchThreadID) {
    uRWTexture[thread_id.xy] = float4(1.0, 1.0, 1.0, 1.0);
}
