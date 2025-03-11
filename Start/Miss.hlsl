#include "Common.hlsl"

// TODO SET UP BUFFER WITH DAVID

[shader("miss")]
void Miss(inout HitInfo payload : SV_RayPayload)
{
    float3 rayDir = WorldRayOrigin();

    float3 lightBlue = float3(0.68f,0.85f,0.90f);
    float3 pink = float3(1.0f,0.75f,0.8f);
    float3 white = float3(1.0f,1.0f,1.0f);

    float yScreenRange = (rayDir.g + 1.0f) * 0.5f;

    float3 colorOut = float3(1.0f, 0.0f, 0.0f);


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

    payload.colorAndDistance = float4(colorOut, 1.0f);
}
