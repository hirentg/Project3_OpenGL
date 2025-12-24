#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

// Represent whichever gBuffer texture you are binding to
uniform sampler2D fboAttachment;
uniform int mode;

void main()
{
	// grab data at current pixel
	vec4 texColor = texture (fboAttachment, TexCoord);
	
	// specular component
	// extract the single value to RGB
	if (mode == 1)
		FragColor = vec4 (vec3 (texColor.a), 1.0);

	// normal component
	// map to 0-1
	else if (mode == 2)
		FragColor = vec4(texColor.rgb * 0.5 + 0.5, 1.0);

	// Albedo and position component
	else 
		FragColor = vec4(texColor.rgb, 1.0);


}