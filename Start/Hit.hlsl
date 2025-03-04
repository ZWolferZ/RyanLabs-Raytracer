#include "Common.hlsl"

struct STriVertex
{ // IMPORTANT - the c++ version of this is 'Vertex' found in the common.h file
    float3 vertex;
    float4 color;
};

StructuredBuffer<STriVertex> BTriVertex : register(t0);
StructuredBuffer<int> indices : register(t1);

[shader("closesthit")]void ClosestHit(inout HitInfo payload,
                                       Attributes attrib)
{
    float3 barycentrics =
      float3(1.f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);

    uint vertId = 3 * PrimitiveIndex();
    // vertId = the first index, vertId + 1 = the second, vertId + 2 = the third
    // e.g. BTriVertex[indices[vertId + 0]]

	STriVertex A = BTriVertex[indices[vertId + 0]];
    STriVertex B = BTriVertex[indices[vertId + 1]];
    STriVertex C = BTriVertex[indices[vertId + 2]];

    float3 colorOut = A.color * barycentrics.x + B.color * barycentrics.y + C.color *
barycentrics.z;
  
    payload.colorAndDistance = float4(colorOut, RayTCurrent());

}
