RWTexture2D<float4> uRWTexture;

#define NUM_THREADS_X 32
#define NUM_THREADS_Y 32

[Shader("compute")]
[NumThreads(NUM_THREADS_X, NUM_THREADS_Y,1)]
void ComputeAdder(uint3 global_thread_id: SV_DispatchThreadID,
                  uint group_index: SV_GroupIndex,
                  uint3 group_id : SV_GroupID) {
    var white = float4(1.0,1.0,1.0,1.0);
    var black = float4(0.0,0.0,0.0,1.0);
    var lerf_factor = group_index / (NUM_THREADS_X * NUM_THREADS_Y * 1.0f);
    if (group_id.x % 2 == 0) {
        lerf_factor = 1.0 - lerf_factor;
    }
    if (group_id.y % 2 == 0) {
        lerf_factor = 1.0 - lerf_factor;
    }
    var mixed = lerp(white, black, lerf_factor);
    uRWTexture[global_thread_id.xy] = mixed;
}
