#version 450 core

layout(location = 0) in vec4 pos;

layout(location = 0) out vert_out
{
    vec2 st;
};

void main()
{
    gl_Position = pos;
    st = (pos.xy + 1) * 0.5;
}
