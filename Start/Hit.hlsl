#include "Common.hlsl"

struct STriVertex
{ // IMPORTANT - the C++ version of this is 'Vertex' found in the common.h file
	float3 vertex;
	float4 normal;
	float2 tex;
};

StructuredBuffer<STriVertex> BTriVertex : register(t0);
StructuredBuffer<int> indices : register(t1);


cbuffer ColourBuffer : register(b0)
{
	float4 objectColour;
	float4 planeColour;
}



[shader("closesthit")]
void ClosestHit(inout HitInfo payload, Attributes attrib)
{
	float3 barycentrics = float3(1.0f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);


	float3 colorOut = objectColour;

	float minB = min(barycentrics.x, min(barycentrics.y, barycentrics.z));

	float edgeThickness = 0.01f;

	if (minB < edgeThickness)
	{
		colorOut = float3(0.0, 0.0, 0.0);
	}

	payload.colorAndDistance += float4(colorOut.xyz, RayTCurrent());
}

[shader("anyhit")]
void AnyHit(inout HitInfo payload, Attributes attrib)
{

	float3 colorOut = { 0.0f, 0.0f, 0.0f };

	payload.colorAndDistance += float4(colorOut, RayTCurrent());
}



[shader("closesthit")]
void PlaneClosestHit(inout HitInfo payload, Attributes attrib)
{

	float3 barycentrics = float3(1.0f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);

	float3 colorOut = planeColour;

	float minB = min(barycentrics.x, min(barycentrics.y, barycentrics.z));

	float edgeThickness = 0.005f;

	if (minB < edgeThickness)
	{
		colorOut = float3(0.0, 0.0, 0.0);
	}

	payload.colorAndDistance = float4(colorOut.xyz, RayTCurrent());
}

