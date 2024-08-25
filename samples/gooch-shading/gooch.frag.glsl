/*

This file is part of antler.
Copyright (C) 2024 Taylor Wampler 

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
  
*/

/*
Reference:

Amy Gooch, Bruce Gooch, Peter Shirley, and Elaine Cohen. 1998.
A non-photorealistic lighting model for automatic technical illustration. In Proceedings of the 25th annual conference on Computer graphics and interactive techniques (SIGGRAPH '98).
Association for Computing Machinery, New York, NY, USA, 447–452. https://doi.org/10.1145/280814.280950
*/

#version 460

layout(location = 0) in vec3 inNormal;

layout(location = 0) out vec4 outColor;


vec3 lightDir = vec3(0.0f, 0.0f, -1.0f);		
vec3 surfaceColor = vec3(0.8f, 0.2f, 0.2f);
float blue = 0.4f;
float yellow = 0.4f;

void main()
{
	vec3 normal = normalize(inNormal);

	vec3 coolColor = blue * vec3(0.0f, 0.0f, 1.0f) + 0.2f * surfaceColor;
	vec3 warmColor = yellow * vec3(1.0f, 1.0f, 0.0f) + 0.6f * surfaceColor;

	float dot = -dot(normal, lightDir);
	vec3 color = 0.5f * ((warmColor + coolColor) + dot * (warmColor - coolColor));
	outColor = vec4(color, 1.0f);
}