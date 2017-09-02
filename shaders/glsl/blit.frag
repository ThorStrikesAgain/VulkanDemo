#version 450 core

layout(location = 0) in frag_in
{
    vec4 uv;
};

out vec4 color;

void main()
{
    // TODO: Actually blit from a texture instead of outputting a constant...
    color = vec4(1, 1, 0, 1);
}
