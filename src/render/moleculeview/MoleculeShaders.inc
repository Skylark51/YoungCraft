
constexpr float kPi = 3.14159265358979323846f;

const char *kVertexShader = R"GLSL(
#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aSurfaceColor;
out vec3 vSurfaceColor;
uniform mat4 uMvp;
uniform mat4 uModel;
uniform mat4 uLightMvp;
out vec3 vNormal;
out vec3 vWorld;
out vec4 vLightPosition;
out float vAxialPosition;
void main() {
    vec4 world = uModel * vec4(aPosition, 1.0);
    vSurfaceColor = aSurfaceColor;
    vWorld = world.xyz;
    vNormal = mat3(transpose(inverse(uModel))) * aNormal;
    gl_Position = uMvp * vec4(world.xyz, 1.0);
    vLightPosition = uLightMvp * world;
    vAxialPosition = aPosition.z + 0.5;
}
)GLSL";

const char *kFragmentShader = R"GLSL(
#version 330 core
in vec3 vNormal;
in vec3 vWorld;
in vec4 vLightPosition;
in float vAxialPosition;
in vec3 vSurfaceColor;
uniform bool uSurfaceColor;
uniform vec3 uColor;
uniform vec3 uSecondColor;
uniform bool uColorBlend;
uniform float uColorStart;
uniform float uColorEnd;
uniform vec3 uLightDirection;
uniform vec3 uSecondLightDirection;
uniform vec3 uLightColor;
uniform vec3 uSecondLightColor;
uniform float uAmbient;
uniform float uDiffuse;
uniform float uSpecular;
uniform float uOpacity;
uniform bool uLighting;
uniform bool uTwoLights;
uniform bool uDepthCueing;
uniform float uDepthCueStrength;
uniform float uShininess;
uniform float uAtomSheenStrength;
uniform float uColorSaturation;
uniform float uMaterialSpecular;
uniform float uMaterialShininess;
uniform vec3 uBackground;
uniform vec3 uCameraPosition;
uniform vec3 uViewDirection;
uniform vec3 uAmbientUp;
uniform bool uPerspective;
uniform vec3 uDepthCenter;
uniform float uDepthSpan;
uniform bool uShadows;
uniform float uShadowStrength;
uniform float uShadowFilterRadius;
uniform sampler2DShadow uShadowMap;
out vec4 fragColor;
void main() {
    vec3 materialColor = uSurfaceColor ? vSurfaceColor : uColor;
    if (uColorBlend) {
        float t = mix(uColorStart, uColorEnd, vAxialPosition);
        materialColor = mix(uColor, uSecondColor, smoothstep(0.40, 0.60, t));
    }
    float baseLuminance = dot(materialColor, vec3(0.2126, 0.7152, 0.0722));
    vec3 displayColor = mix(vec3(baseLuminance), materialColor, uColorSaturation);
    vec3 base = pow(clamp(displayColor, 0.0, 1.0), vec3(2.2));
    vec3 color = base;
    if (uLighting) {
        vec3 n = normalize(vNormal);
        vec3 l1 = normalize(uLightDirection);
        vec3 l2 = normalize(uSecondLightDirection);
        float visibility = 1.0;
        if (uShadows) {
            vec3 projected = vLightPosition.xyz / vLightPosition.w;
            projected = projected * 0.5 + 0.5;
            // The receiver's depth changes across a wide soft-shadow filter.
            // Compensate its local slope to avoid treating that slope as occlusion.
            vec3 dx = dFdx(projected), dy = dFdy(projected);
            float determinant = dx.x * dy.y - dx.y * dy.x;
            vec2 depthGradient = abs(determinant) > 1e-10
                ? vec2(dy.y * dx.z - dx.y * dy.z, dx.x * dy.z - dy.x * dx.z) / determinant
                : vec2(0.0);
            if (projected.z >= 0.0 && projected.z <= 1.0 &&
                projected.x >= 0.0 && projected.x <= 1.0 &&
                projected.y >= 0.0 && projected.y <= 1.0) {
                float blocked = 0.0;
                float weightSum = 0.0;
                vec2 texel = 1.0 / vec2(textureSize(uShadowMap, 0));
                float bias = max(0.00015, 0.00065 * (1.0 - max(dot(n, l1), 0.0)));
                const float weights[5] = float[](1.0, 0.88250, 0.60653, 0.32465, 0.13534);
                for (int x = -4; x <= 4; ++x)
                    for (int y = -4; y <= 4; ++y) {
                        vec2 offset = vec2(x, y) * texel * (uShadowFilterRadius / 4.0);
                        float receiverDepth = projected.z + clamp(dot(depthGradient, offset), -0.02, 0.02);
                        float weight = weights[abs(x)] * weights[abs(y)];
                        blocked += weight * (1.0 - texture(uShadowMap,
                            vec3(projected.xy + offset, receiverDepth - bias)));
                        weightSum += weight;
                    }
                visibility = 1.0 - (blocked / weightSum) * uShadowStrength;
            }
        }
        const float secondaryDiffuseWeight = 0.28;
        const float secondarySpecularWeight = 0.12;
        float keyNdotL = max(dot(n, l1), 0.0);
        float fillNdotL = max(dot(n, l2), 0.0);
        float keyDiffuse = keyNdotL * uDiffuse * visibility;
        float fillDiffuse = uTwoLights ? fillNdotL * uDiffuse * secondaryDiffuseWeight : 0.0;
        vec3 viewDir = uPerspective ? normalize(uCameraPosition - vWorld) : uViewDirection;
        float specularPower = max(uShininess * uMaterialShininess, 1.0);
        // Avoid undefined normalize(0) for a light exactly opposite the viewer.
        vec3 keyHalfDir = (l1 + viewDir) / max(length(l1 + viewDir), 0.0001);
        vec3 fillHalfDir = (l2 + viewDir) / max(length(l2 + viewDir), 0.0001);
        float keySpecular = keyNdotL > 0.0
            ? pow(max(dot(n, keyHalfDir), 0.0), specularPower)
              * uSpecular * uMaterialSpecular * visibility
            : 0.0;
        float fillSpecular = uTwoLights && fillNdotL > 0.0
            ? pow(max(dot(n, fillHalfDir), 0.0), specularPower)
              * uSpecular * uMaterialSpecular * secondarySpecularWeight
            : 0.0;
        // Ambient follows the user's intensity instead of adding a hidden light floor.
        float hemisphere = mix(0.60, 1.0, dot(n, uAmbientUp) * 0.5 + 0.5);
        vec3 keyColor = pow(uLightColor, vec3(2.2));
        vec3 fillColor = pow(uSecondLightColor, vec3(2.2));
        color = base * (uAmbient * hemisphere + keyColor * keyDiffuse + fillColor * fillDiffuse);
        // Neutral dielectric highlights make curvature readable without recoloring atoms.
        color += keyColor * keySpecular + fillColor * fillSpecular;
        // Broad neutral sheen beneath the focused glint, enabled by the preset.
        // Bonds use a lower material coefficient and retain their slender finish.
        float atomMaterial = smoothstep(0.75, 1.0, uMaterialSpecular);
        float softReflection = keyNdotL > 0.0
            ? pow(max(dot(n, keyHalfDir), 0.0), 12.0) * uAtomSheenStrength * uSpecular
              * atomMaterial * visibility
            : 0.0;
        color += keyColor * softReflection;
    }
    color = pow(clamp(color, 0.0, 1.0), vec3(1.0 / 2.2));
    if (uDepthCueing) {
        // Molecular depth in world units, independent of near/far clipping and zoom.
        float depth = 0.5 - dot(vWorld - uDepthCenter, uViewDirection) / max(uDepthSpan, 0.001);
        color = mix(color, uBackground, smoothstep(0.15, 1.0, depth) * clamp(uDepthCueStrength, 0.0, 0.85));
    }
    fragColor = vec4(color, uOpacity);
}
)GLSL";

const char *kDepthVertexShader = R"GLSL(
#version 330 core
layout(location = 0) in vec3 aPosition;
uniform mat4 uModel;
uniform mat4 uLightMvp;
void main() { gl_Position = uLightMvp * uModel * vec4(aPosition, 1.0); }
)GLSL";

const char *kDepthFragmentShader = R"GLSL(
#version 330 core
void main() {}
)GLSL";

const char *kLineVertexShader = R"GLSL(
#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aColor;
uniform mat4 uMvp;
out vec3 vColor;
void main() { vColor = aColor; gl_Position = uMvp * vec4(aPosition, 1.0); }
)GLSL";

const char *kLineFragmentShader = R"GLSL(
#version 330 core
in vec3 vColor;
out vec4 fragColor;
void main() { fragColor = vec4(vColor, 1.0); }
)GLSL";
