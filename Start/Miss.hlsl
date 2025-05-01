#pragma region Includes
//Include{s}
#include "Common.hlsl"
#pragma endregion

cbuffer CameraParams : register(b0)
{
    float4x4 viewI;
    float4x4 projectionI;
    float rX;
    float rY;
    float2 padding;
    float transMode;
    float3 morePadding;

}

[shader("miss")]
void Miss(inout HitInfo payload : SV_RayPayload)
{
    uint2 launchIndex = DispatchRaysIndex().xy;
    float2 dims = float2(DispatchRaysDimensions().xy);
    float2 d = (((launchIndex.xy + 0.5f) / dims.xy) * 2.f - 1.f);

    float4 target = mul(projectionI, float4(d.x, -d.y, 1, 1));
    float3 rayDir = mul(viewI, float4(target.xyz, 0));



    float3 lightBlue = float3(0.68f,0.85f,0.90f);
    float3 pink = float3(1.0f,0.75f,0.8f);
    float3 white = float3(1.0f,1.0f,1.0f);

    float yScreenRange = (rayDir.g + 1.0f) * 0.5;

    float3 colorOut = float3(0.2f, 0.2f, 0.2f);


    if (transMode == 1.0f)
    {

        if (yScreenRange < 0.2f)
        {
            colorOut = lightBlue;
        }
        else if (yScreenRange < 0.4f)
        {
            colorOut = pink;
        }
        else if (yScreenRange < 0.6f)
        {
            colorOut = white;
        }
        else if (yScreenRange < 0.8f)
        {
            colorOut = pink;
        }
        else
        {
            colorOut = lightBlue;
        }
    }
    payload.colorAndDistance = float4(colorOut, 1.0f);
}

[shader("miss")]
void ShadowMiss(inout ShadowHitInfo payload : SV_RayPayload)
{
    payload.isHit = false;
}