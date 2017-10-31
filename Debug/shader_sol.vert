#version 150

in vec2 position;
in vec3 colour;
in vec2 texcoord;

out vec3 Colour;
out vec2 Texcoord;

void main()
{
	Texcoord = texcoord;
	Colour = colour;
    gl_Position = vec4(position, 0.0, 1.0);
}

