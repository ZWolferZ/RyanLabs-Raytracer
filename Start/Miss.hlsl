#include "Common.hlsl"

[shader("miss")]void Miss(inout HitInfo payload : SV_RayPayload)
{

    float3 rayDir = WorldRayOrigin();

    float3 red = float3(1.0f, 0.0f, 0.0f);

    float3 blue = float3(0.0f, 0.0f, 1.0f);

    float3 colorOut = lerp(red, blue, (rayDir.g + 1) * 0.5f);

    payload.colorAndDistance = float4(colorOut, 1.0);
}
