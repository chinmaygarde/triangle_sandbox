RWTexture2D<uint4> uRWTexture;

[Shader("compute")]
[NumThreads(1,1,1)]
void ComputeAdder(uint3 thread_id: SV_DispatchThreadID) {
    uRWTexture[thread_id.xy] = uint4(255, 0, 255, 255);
}
