
#pragma region Includes
//Include{s}
#include "Common.hlsl"
#pragma endregion


#pragma region Shader Data

// Object Tri Data
StructuredBuffer<STriVertex> BTriVertex : register(t0);

// Object Index Data
StructuredBuffer<int> indices : register(t1);

// Acceleration Structure for the object
RaytracingAccelerationStructure SceneBVH : register(t2);

// Texture Data for the object
Texture2D<float4> g_texture : register(t3);

// Sampler for the object for the texture
SamplerState g_sampler : register(s0);

// Scene Light Data
cbuffer LightParams : register(b0)
{
	float4 lightPosition;
	float4 lightAmbientColor;
	float4 lightDiffuseColor;
	float4 lightSpecularColor;
	float lightSpecularPower;
	float lightRange;
	uint shadows;
	uint shawdowRayCount;
	float4 padding;

}

// Material Data for the object
cbuffer MaterialBuffer : register(b1)
{
	uint reflection;
	float shininess;
	int maxRecursionDepth;
	uint triOutline;
	float triThickness;
	float3 triColour;
	float4 objectColour;
	float roughness;
	uint texture;
	float2 padding2;
}
#pragma endregion

#pragma region Hit Attribute Functions

// Finds the interpolated value of the attribute at the hit point for the triangle normal.
float3 HitAttribute(float3 vertexAttributes[3], Attributes attr)
{
	return vertexAttributes[0] +
		attr.bary.x * (vertexAttributes[1] - vertexAttributes[0]) +
		attr.bary.y * (vertexAttributes[2] - vertexAttributes[0]);

}

// Finds the interpolated value of the attribute at the hit point for the triangle tex-coords.
float2 HitAttribute(float2 vertexAttribute[3], Attributes attr)
{
	return vertexAttribute[0] +
		attr.bary.x * (vertexAttribute[1] - vertexAttribute[0]) +
		attr.bary.y * (vertexAttribute[2] - vertexAttribute[0]);
}

// Returns the hit position in world space.
float3 HitWorldPosition()
{
	return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
}

// Yoinked Random Function from Louise
float random(float2 uv)
{
	return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453123);
}

#pragma endregion


#pragma region Lighting Functions

// Calculates the diffuse lighting for the object.
float4 CalculateDiffuseLighting(float3 lightDirection, float3 worldNormal)
{
	float diffuseAmount = saturate(dot(lightDirection, normalize(worldNormal)));

	float diffuseCoEfficent = saturate(dot(lightDirection, worldNormal));

	float4 diffuseOut = diffuseAmount * diffuseCoEfficent * lightDiffuseColor * objectColour;

	return diffuseOut;
}

// Calculates the ambient lighting for the object.
float4 CalculateAmbientLighting(float3 worldNormal)
{

	float4 ambientColorMin = lightAmbientColor - 0.1;
	float a = 1 - saturate(dot(worldNormal, float3(0, -1, 0)));
	float4 ambientOut = objectColour * lerp(ambientColorMin, lightAmbientColor, a);

	return ambientOut;
}

// Calculates the specular lighting for the object.
float4 CalculateSpecularLighting(float3 hitWorldPosition, float3 lightDirection, float3 worldNormal)
{
	// Blinn-Phong ALERT!

	float3 viewDir = normalize(WorldRayOrigin() - hitWorldPosition);
	float3 halfDir = normalize(lightDirection + viewDir);

	float specFactor = pow(saturate(dot(worldNormal, halfDir)), lightSpecularPower);

	float4 specularCoEfficent = pow(saturate(dot(lightDirection, normalize(-WorldRayDirection()))), specFactor);

	float4 specularOut = specFactor * specularCoEfficent * lightSpecularColor;

	return specularOut;
}
#pragma endregion


#pragma region RayTracing Functions

// Calculates the reflection ray for the object.
float4 TraceReflectionRay(in RayDesc reflectionRay,in uint recursionDepth)
{
	if (recursionDepth >= maxRecursionDepth)
	{
		return float4(0.0f, 0.0f, 0.0f, 0.0f);
	}

	HitInfo reflectionPayload;

	reflectionPayload.colorAndDistance = float4(0.0f, 0.0f, 0.0f, 0.0f);
	reflectionPayload.recursiveDepth = recursionDepth + 1;

	TraceRay(SceneBVH, RAY_FLAG_FORCE_NON_OPAQUE, 0xFF, 0, 0, 0, reflectionRay, reflectionPayload);


	return reflectionPayload.colorAndDistance;
}

// Calculates the fresnel reflectance for the object.
float3 FresnelReflectanceSchlick(float3 worldNormal, float3 albedo)
{
	float cosi = saturate(dot(-WorldRayDirection(), worldNormal));
	return albedo + (1 - albedo) * pow(1 - cosi, 5);
}

// Test if the object has reflection rays
float3 TestReflectionRays(float3 colorOut, float3 hitWorldPosition, float3 worldNormal, HitInfo payload)
{

	if (reflection == 1)
	{
		RayDesc reflectionRay;

		reflectionRay.Origin = hitWorldPosition + (worldNormal * 0.01f);
		reflectionRay.Direction = reflect(WorldRayDirection(), worldNormal);
		reflectionRay.TMin = 0.00001f;
		reflectionRay.TMax = 100000;

		float4 reflectionColor = TraceReflectionRay(reflectionRay, payload.recursiveDepth);
		float3 fresnelReflectance = FresnelReflectanceSchlick( worldNormal, objectColour.xyz);


		float4 reflectionOut = shininess * float4(fresnelReflectance, 1) * reflectionColor;


		colorOut += reflectionOut;
	}


	return colorOut;
}

// Calculates the shadow rays for the object and adds the diffuse and specular lighting to the colorOut.
float3 TraceShadowRays(float3 colorOut, float4 diffuseColour, float4 specularColour, float3 hitWorldPosition, float3 worldNormal, float3 lightDirection)
{
	if (shadows == 0)
	{
		colorOut += diffuseColour;
		colorOut += specularColour;
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
			TraceRay(SceneBVH, RAY_FLAG_FORCE_NON_OPAQUE, 0xFF, 1, 0, 1, ray, shadowPayload);

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
			TraceRay(SceneBVH, RAY_FLAG_FORCE_NON_OPAQUE, 0xFF, 1, 0, 1, ray, shadowPayload);

			shadowTotal += shadowPayload.isHit ? 0.0f : 1.0f;
		}

		float shadowFactor = shadowTotal / shawdowRayCount;
		colorOut *= shadowFactor;
	}

	return colorOut;
}
#pragma endregion

#pragma region Outline Functions

// Draws the triangle outlines for the object.
float3 DrawTriOutlines(float3 colorOut, float3 barycentrics)
{

	if (triOutline == 1)
	{
		float minB = min(barycentrics.x, min(barycentrics.y, barycentrics.z));

		if (minB < triThickness)
		{
			colorOut = triColour;
		}
	}
	return colorOut;
}
#pragma endregion


#pragma region Normal Functions
// Calculates the triangle normal for the object.
float3 CalculateTriangleNormal(uint vertid, Attributes attrib)
{
	float3 vertexNormals[3];
	vertexNormals[0] = BTriVertex[indices[vertid + 0]].normal.xyz;
	vertexNormals[1] = BTriVertex[indices[vertid + 1]].normal.xyz;
	vertexNormals[2] = BTriVertex[indices[vertid + 2]].normal.xyz;
	float3 triangleNormal = HitAttribute(vertexNormals, attrib);
	return triangleNormal;
}

// Calculates the roughness normal for the object.
float3 CalculateRoughnessNormal(float3 hitWorldPosition,float3 worldNormal)
{

	if (roughness == 0.0f)
	{
		return worldNormal;
	}

	float noise = random(hitWorldPosition.xy);
	float scaledNoise = noise * 2.0 - 1.0;

	float rand1 = random(hitWorldPosition.xy + float2(0.1, 0.2));
	float rand2 = random(hitWorldPosition.xy + float2(0.3, 0.4));
	float rand3 = random(hitWorldPosition.xy + float2(0.5, 0.6));

	float3 randomVector = float3(rand1, rand2, rand3);

	randomVector *= scaledNoise * roughness;

	return normalize(worldNormal + randomVector);
}
#pragma endregion

#pragma region Texture Functions
// Calculates the texture colour for the object.
float4 CalculateTextureColour(uint vertid, Attributes attrib)
{
	float4 textureColour = { 0, 0, 0, 0 };

	if (texture == 1)
	{
		float2 texCoords[3];
		texCoords[0] = BTriVertex[indices[vertid + 0]].tex;
		texCoords[1] = BTriVertex[indices[vertid + 1]].tex;
		texCoords[2] = BTriVertex[indices[vertid + 2]].tex;

		float2 texCoord = HitAttribute(texCoords, attrib);

	   textureColour = g_texture.SampleLevel(g_sampler, texCoord, 0);
	}

	return textureColour;
}
#pragma endregion


#pragma region Hit Shaders
// Hit shader for objects that are not planes.
[shader("closesthit")]
void ClosestHit(inout HitInfo payload, Attributes attrib)
{
	float3 barycentrics = float3(1.0f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);
	uint vertid = 3 * PrimitiveIndex();



	float3 triangleNormal = CalculateTriangleNormal(vertid, attrib);

	float3 worldNormal = normalize(mul(triangleNormal, (float3x3) ObjectToWorld4x3()));
	float3 hitWorldPosition = HitWorldPosition();

	float3 lightDirection = normalize((float3) lightPosition - hitWorldPosition);
	float distance = length((float3) lightPosition - hitWorldPosition);
	float attenuation = saturate(1.0 - distance / lightRange);

	float3 roughnessNormal = CalculateRoughnessNormal(hitWorldPosition, worldNormal);
	float4 textureColour = CalculateTextureColour(vertid, attrib) * attenuation;
	float4 diffuseColour = CalculateDiffuseLighting(lightDirection, roughnessNormal) * attenuation;
	float4 ambientColour = CalculateAmbientLighting(roughnessNormal) * attenuation;
	float4 specularColour = CalculateSpecularLighting(hitWorldPosition, lightDirection, roughnessNormal) * attenuation;

	float3 colorOut = textureColour + ambientColour;

	colorOut = DrawTriOutlines(colorOut, barycentrics);

	colorOut = TraceShadowRays(colorOut, diffuseColour, specularColour, hitWorldPosition, roughnessNormal, lightDirection);

	colorOut = TestReflectionRays(colorOut, hitWorldPosition, roughnessNormal, payload);

	payload.colorAndDistance += float4(colorOut.xyz, RayTCurrent());
}

// Anyhit shader for objects.
[shader("anyhit")]
void AnyHit(inout HitInfo payload, Attributes attrib)
{
	float3 colorOut = { 0.0f, 0.0f, 0.0f };

	payload.colorAndDistance += float4(colorOut, RayTCurrent());
}


// Hit shader for objects that are planes.
[shader("closesthit")]
void PlaneClosestHit(inout HitInfo payload, Attributes attrib)
{

	float3 barycentrics = float3(1.0f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);

	uint vertid = 3 * PrimitiveIndex();

	float3 triangleNormal = CalculateTriangleNormal(vertid, attrib);

	float3 worldNormal = normalize(mul(triangleNormal, (float3x3) ObjectToWorld4x3()));
	float3 hitWorldPosition = HitWorldPosition();

	float3 lightDirection = normalize((float3) lightPosition - hitWorldPosition);
	float distance = length((float3) lightPosition - hitWorldPosition);
	float attenuation = saturate(1.0 - distance / lightRange);

	float3 roughnessNormal = CalculateRoughnessNormal(hitWorldPosition, worldNormal);
	float4 textureColour = CalculateTextureColour(vertid, attrib) * attenuation;
	float4 diffuseColour = CalculateDiffuseLighting(lightDirection, roughnessNormal) * attenuation;
	float4 ambientColour = CalculateAmbientLighting(roughnessNormal) * attenuation;

	float3 colorOut = textureColour + ambientColour;

	colorOut = DrawTriOutlines(colorOut, barycentrics);

	colorOut = TraceShadowRays(colorOut, diffuseColour, float4(0, 0, 0, 0), hitWorldPosition, roughnessNormal, lightDirection);

	colorOut = TestReflectionRays(colorOut, hitWorldPosition, roughnessNormal, payload);

	payload.colorAndDistance = float4(colorOut.xyz, RayTCurrent());
}

// Shadow hit shader for objects.
[shader("closesthit")]
void ShadowHit(inout ShadowHitInfo payload, Attributes attrib)
{
	payload.isHit = true;
}
#pragma endregion