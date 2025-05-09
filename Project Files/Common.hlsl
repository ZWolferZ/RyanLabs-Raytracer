#pragma region Data Structures

struct STriVertex
{ // IMPORTANT - the C++ version of this is 'Vertex' found in the common.h file
    float3 vertex;
    float4 normal;
    float2 tex;
};

// Hit information, aka ray payload
// This sample only carries a shading color and hit distance.
// Note that the payload should be kept as small as possible,
// and that its size must be declared in the corresponding
// D3D12_RAYTRACING_SHADER_CONFIG pipeline subobjet.
struct HitInfo {
  float4 colorAndDistance;
    int recursiveDepth;
};

// Attributes output by the raytracing when hitting a surface,
// here the barycentric coordinates
struct Attributes {
  float2 bary;
};

// Hit information for shadow rays
struct ShadowHitInfo
{
    bool isHit;
};
#pragma endregion
