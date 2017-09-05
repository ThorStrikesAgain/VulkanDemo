#version 450 core

layout(set=0, binding=0) uniform sampler2D inputSampler;

layout(location = 0) in frag_in
{
    vec2 st;
};

layout(location = 0) out vec4 color;

void main()
{
    color = vec4(texture(inputSampler, st).rgb, 0);
}
