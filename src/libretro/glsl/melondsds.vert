#version 140
layout(std140) uniform uConfig
{
    vec2 uScreenSize;
    uint u3DScale;
    uint uFilterMode;
    vec4 cursorPos;
};
in vec2 pos;
in vec2 texcoord;
smooth out vec2 fTexcoord;
void main()
{
    vec4 fpos;
    fpos.xy = ((pos * 2.0) / uScreenSize) - 1.0;
    fpos.y *= -1;
    fpos.z = 0.0;
    fpos.w = 1.0;
    gl_Position = fpos;
    fTexcoord = texcoord;
}