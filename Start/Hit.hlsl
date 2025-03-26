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

cbuffer LightParams : register(b1)
{
	float4 lightPosition;
	float4 lightAmbientColor;
	float4 lightDiffuseColor;
	float4 lightSpecularColor;
	float lightSpecularPower;
	float lightRange;
	float2 padding;
}


float3 HitAttributeV3(float3 vertexAttributes[3], Attributes attr)
{
	return vertexAttributes[0] +
		attr.bary.x * (vertexAttributes[1] - vertexAttributes[0]) +
		attr.bary.y * (vertexAttributes[2] - vertexAttributes[0]);

}

float2 HitAttributeV2(float2 vertexAttribute[3], Attributes attr)
{
	return vertexAttribute[0] +
		attr.bary.x * (vertexAttribute[1] - vertexAttribute[0]) +
		attr.bary.y * (vertexAttribute[2] - vertexAttribute[0]);
}

float3 HitWorldPosition()
{
	return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
}

float4 CalculateDiffuseLighting(float3 lightDirection, float3 worldNormal, float4 colour)
{
	float diffuseAmount = saturate(dot(lightDirection, normalize(worldNormal)));
	float4 diffuseOut = diffuseAmount * (colour * lightDiffuseColor);

	return diffuseOut;
}

float4 CalculateAmbientLighting(float4 colour)
{
	float4 ambientOut = lightAmbientColor * colour;
	return ambientOut;
}

float4 CalculateSpecularLighting(float3 hitWorldPosition,float3 lightDirection, float3 worldNormal, float4 colour)
{
	float3 viewDir = normalize(WorldRayDirection() - hitWorldPosition);

	float3 reflectDir = reflect(lightDirection, normalize(worldNormal));

	float specFactor = pow(saturate(dot(viewDir, reflectDir)), lightSpecularPower);

	float4 specularOut = specFactor * (lightSpecularColor * colour);

	return specularOut;
}

[shader("closesthit")]
void ClosestHit(inout HitInfo payload, Attributes attrib)
{
	float3 barycentrics = float3(1.0f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);
	uint vertid = 3 * PrimitiveIndex();

	float3 vertexNormals[3];
	vertexNormals[0] = BTriVertex[indices[vertid + 0]].normal.xyz;
	vertexNormals[1] = BTriVertex[indices[vertid + 1]].normal.xyz;
	vertexNormals[2] = BTriVertex[indices[vertid + 2]].normal.xyz;

	float3 triangleNormal = HitAttributeV3(vertexNormals, attrib);
	float3 worldNormal = normalize(mul(triangleNormal, (float3x3) ObjectToWorld4x3()));

	float3 hitWorldPosition = HitWorldPosition();


	float3 lightDirection = normalize((float3) lightPosition - hitWorldPosition);

	float distance = length((float3) lightPosition - hitWorldPosition);

	float attenuation = saturate(1.0 - distance / lightRange);


	float4 diffuseColour = CalculateDiffuseLighting(lightDirection, worldNormal, objectColour) * attenuation;
	float4 ambientColour = CalculateAmbientLighting(objectColour) * attenuation;
	float4 specularColour = CalculateSpecularLighting(hitWorldPosition,lightDirection, worldNormal, objectColour) * attenuation;

	float3 colorOut = diffuseColour + ambientColour + specularColour;

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

	uint vertid = 3 * PrimitiveIndex();

	float3 vertexNormals[3];

	vertexNormals[0] = BTriVertex[indices[vertid + 0]].normal.xyz;
	vertexNormals[1] = BTriVertex[indices[vertid + 1]].normal.xyz;
	vertexNormals[2] = BTriVertex[indices[vertid + 2]].normal.xyz;


	float3 triangleNormal = HitAttributeV3(vertexNormals, attrib);

	float3 worldNormal = normalize(mul(triangleNormal, (float3x3) ObjectToWorld4x3()));

	float3 hitWorldPosition = HitWorldPosition();

	float3 lightDirection = normalize((float3) lightPosition - hitWorldPosition);

	float distance = length((float3) lightPosition - hitWorldPosition);

	float attenuation = saturate(1.0 - distance / lightRange);

	float4 diffuseColour = CalculateDiffuseLighting(lightDirection, worldNormal, planeColour) * attenuation;

	float4 ambientColour = CalculateAmbientLighting(planeColour) * attenuation;

	float3 colorOut = diffuseColour + ambientColour;

	float minB = min(barycentrics.x, min(barycentrics.y, barycentrics.z));

	float edgeThickness = 0.005f;

	if (minB < edgeThickness)
	{
		colorOut = float3(0.0, 0.0, 0.0);
	}

	payload.colorAndDistance = float4(colorOut.xyz, RayTCurrent());
}

