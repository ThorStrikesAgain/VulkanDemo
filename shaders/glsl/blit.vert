#version 450 core

layout(location = 0) in vec4 pos;

layout(location = 0) out vert_out
{
    vec4 uv;
};

void main()
{
    gl_Position = pos;
    uv = (pos + 1) * 0.5;
}
