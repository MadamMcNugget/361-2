#version 130

in  vec4 color;
out vec4  fColor;

void main() 
{ 
	if (color.rgba == vec4(0.0, 0.0, 0.0, 1.0))
		discard;

	fColor = color;
} 