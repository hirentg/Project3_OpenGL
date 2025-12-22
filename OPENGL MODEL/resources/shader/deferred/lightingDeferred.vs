#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 2) in vec2 aTexCoord;	// in render quad texcoord is location 2

out vec2 TexCoord;

void main()
{
	TexCoord = aTexCoord;
	gl_Position = vec4 (aPos, 1.0);
}

