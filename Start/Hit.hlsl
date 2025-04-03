#include "Common.hlsl"

struct STriVertex
{ // IMPORTANT - the C++ version of this is 'Vertex' found in the common.h file
	float3 vertex;
	float4 normal;
	float2 tex;
};

StructuredBuffer<STriVertex> BTriVertex : register(t0);
StructuredBuffer<int> indices : register(t1);
RaytracingAccelerationStructure SceneBVH : register(t2);

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
	uint shadows;
	uint shawdowRayCount;
	uint reflection;
	float shininess;
	uint maxRecursionDepth;
	float padding;

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

float4 CalculateDiffuseLighting(float3 lightDirection, float3 worldNormal)
{
	float diffuseAmount = saturate(dot(lightDirection, normalize(worldNormal)));
	float4 diffuseOut = diffuseAmount * lightDiffuseColor;

	return diffuseOut;
}

float4 CalculateAmbientLighting(float4 colour)
{
	float4 ambientOut = lightAmbientColor * colour;

	return ambientOut;
}

float4 CalculateSpecularLighting(float3 hitWorldPosition, float3 lightDirection, float3 worldNormal)
{
	// Blinn-Phong ALERT!

	float3 viewDir = normalize(WorldRayOrigin() - hitWorldPosition);
	float3 halfDir = normalize(lightDirection + viewDir);

	float specFactor = pow(saturate(dot(worldNormal, halfDir)), lightSpecularPower);
	float4 specularOut = specFactor * lightSpecularColor;

	return specularOut;
}

// Yoinked Random Function from Louise
float random(float2 uv)
{
	return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453123);
}

float4 TraceRefelctionRay(RayDesc reflectionRay, uint recursionDepth)
{
	if (recursionDepth > maxRecursionDepth)
	{
		return float4(0.0f, 0.0f, 0.0f, 1.0f);
	}

	HitInfo reflectionPayload;

	reflectionPayload.colorAndDistance = float4(0.0f, 0.0f, 0.0f, 1.0f);
	reflectionPayload.recursiveDepth = recursionDepth + 1;

	TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 0, 0, 0, reflectionRay, reflectionPayload);

	return reflectionPayload.colorAndDistance;
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

	float4 diffuseColour = CalculateDiffuseLighting(lightDirection, worldNormal) * attenuation;
	float4 ambientColour = CalculateAmbientLighting(objectColour) * attenuation;
	float4 specularColour = CalculateSpecularLighting(hitWorldPosition,lightDirection, worldNormal) * attenuation;
	float3 colorOut = ambientColour;

	float minB = min(barycentrics.x, min(barycentrics.y, barycentrics.z));
	float edgeThickness = 0.01f;
	if (minB < edgeThickness)
	{
		colorOut = float3(0.0, 0.0, 0.0);
	}


	if (shadows == 0)
	{
		colorOut += diffuseColour;
		colorOut += specularColour;
		payload.colorAndDistance += float4(colorOut.xyz, RayTCurrent());

	}
	else
	{
		{
			RayDesc ray;

			ray.Origin = hitWorldPosition + (worldNormal * 0.01f);
			ray.Direction = lightDirection;

			ray.TMin = 0.00001f;
			ray.TMax = 100000;

			ShadowHitInfo shadowPayload;
			shadowPayload.isHit = false;
			TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 1, 0, 1, ray, shadowPayload);

			if (!shadowPayload.isHit)
			{
				colorOut += diffuseColour;
				colorOut += specularColour;
			}
		}


		float shadowTotal = 0.0f;
		for (int i = 0; i < shawdowRayCount; i++)
		{

			float offset = random(float2(i, i++)) - 0.5f;

			float3 offsetDir = normalize(lightDirection + 0.05f * offset);

			RayDesc ray;
			ray.Origin = hitWorldPosition + (worldNormal * 0.01f);
			ray.Direction = offsetDir;
			ray.TMin = 0.00001f;
			ray.TMax = 100000;

			ShadowHitInfo shadowPayload;
			shadowPayload.isHit = false;
			TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 1, 0, 1, ray, shadowPayload);

			shadowTotal += shadowPayload.isHit ? 0.0f : 1.0f;
		}

		float shadowFactor = shadowTotal / shawdowRayCount;
		colorOut *= shadowFactor;
	}

	if (reflection == 1)
	{
	 
	
		RayDesc reflectionRay;

		reflectionRay.Origin = hitWorldPosition + (worldNormal * 0.01f);
		reflectionRay.Direction = reflect(WorldRayDirection(), worldNormal);
		reflectionRay.TMin = 0.00001f;
		reflectionRay.TMax = 100000;

		float4 reflectionColor = TraceRefelctionRay(reflectionRay, payload.recursiveDepth);


		float4 reflectionOut = shininess * reflectionColor;


		colorOut += reflectionOut;
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

	float4 diffuseColour = CalculateDiffuseLighting(lightDirection, worldNormal) * attenuation;
	float4 ambientColour = CalculateAmbientLighting(planeColour) * attenuation;

	float3 colorOut = ambientColour;

	float minB = min(barycentrics.x, min(barycentrics.y, barycentrics.z));
	float edgeThickness = 0.005f;
	if (minB < edgeThickness)
	{
		colorOut = float3(0.0, 0.0, 0.0);
	}

	if (shadows == 0)
	{
		colorOut += diffuseColour;
		payload.colorAndDistance += float4(colorOut.xyz, RayTCurrent());
	}
	else
	{
		{
			RayDesc ray;

			ray.Origin = hitWorldPosition + (worldNormal * 0.01f);
			ray.Direction = lightDirection;

			ray.TMin = 0.00001f;
			ray.TMax = 100000;

			ShadowHitInfo shadowPayload;
			shadowPayload.isHit = false;
			TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 1, 0, 1, ray, shadowPayload);


			if (!shadowPayload.isHit)
			{
				colorOut += diffuseColour;
			}

		}


		float shadowTotal = 0.0f;
		for (int i = 0; i < shawdowRayCount; i++)
		{
			float offset = random(float2(i, i++)) - 0.5f;

			float3 offsetDir = normalize(lightDirection + 0.05f * offset);

			RayDesc ray;
			ray.Origin = hitWorldPosition + (worldNormal * 0.01f);
			ray.Direction = offsetDir;
			ray.TMin = 0.00001f;
			ray.TMax = 100000;

			ShadowHitInfo shadowPayload;
			shadowPayload.isHit = false;
			TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 1, 0, 1, ray, shadowPayload);

			shadowTotal += shadowPayload.isHit ? 0.0f : 1.0f;
		}

		float shadowFactor = shadowTotal / shawdowRayCount;



		colorOut *= shadowFactor;
	}


	payload.colorAndDistance = float4(colorOut.xyz, RayTCurrent());
}

[shader("closesthit")]
void ShadowHit(inout ShadowHitInfo payload, Attributes attrib)
{
	payload.isHit = true;
}
