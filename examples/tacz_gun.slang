// First-person TACZ gun renderer.
//
// Like depth_buffering.slang, but each face also carries a depth level and
// a face normal. In eye space every face is shifted away from the eye,
// along its normal, by level * depthBias, so that coplanar faces (which the
// bedrock models contain in large numbers) can never end up with identical
// depth, which is what causes z-fighting. The shift is a fraction of a
// pixel, so the model is visually unchanged.
struct VSInput {
    float3 inPosition;
    float3 inColor;
    float2 inTexCoord;
    float  inDepthLevel;
    float3 inNormal;
};

struct UniformBuffer {
    float4x4 model;
    float4x4 view;
    float4x4 proj;
    float    depthBias;
};
ConstantBuffer<UniformBuffer> ubo;

struct VSOutput
{
    float4 pos : SV_Position;
    float3 fragColor;
    float2 fragTexCoord;
};

[shader("vertex")]
VSOutput vertMain(VSInput input) {
    VSOutput output;
    float4 eye = mul(ubo.view, mul(ubo.model, float4(input.inPosition, 1.0)));
    // Face normal in eye space (model/view are rotation + uniform scale).
    float3 nEye = normalize(mul(ubo.view, mul(ubo.model, float4(input.inNormal, 0.0))).xyz);
    eye.xyz -= input.inDepthLevel * ubo.depthBias * nEye;
    output.pos = mul(ubo.proj, eye);
    output.fragColor = input.inColor;
    output.fragTexCoord = input.inTexCoord;
    return output;
}

Sampler2D texture;

[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_TARGET {
   return texture.Sample(vertIn.fragTexCoord);
}
