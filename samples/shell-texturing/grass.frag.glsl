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
This shader uses Gooch lighting with an simple ambient occlusion contribution.

Reference:

Amy Gooch, Bruce Gooch, Peter Shirley, and Elaine Cohen. 1998.
A non-photorealistic lighting model for automatic technical illustration. In Proceedings of the 25th annual conference on Computer graphics and interactive techniques (SIGGRAPH '98).
Association for Computing Machinery, New York, NY, USA, 447–452. https://doi.org/10.1145/280814.280950
*/

#version 460

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec3 inUvw;

layout(location = 0) out vec4 outColor;

vec3 lightDir = vec3(0.0f, 0.0f, -1.0f);		
vec3 surfaceColor = vec3(0.1f, 0.9f, 0.1f);
float blue = 0.4f;
float yellow = 0.4f;
float attenuation = 0.1f;
float diffuseContrib = 0.2f; // between 0.0f and 1.0, it is a lerp factor between ambient occlusion and the lambertian diffuse

/*
REY, W. 1998. On generating random numbers, with help of y= [(a+x)sin(bx)] mod 1.
In 22nd European Meeting of Statisticians and the 7th Vilnius Conference on Probability
Theory and Mathematical Statistics. VSP, Zeist, The Netherlands.
*/
float rand(vec2 coord)
{
    float t = dot(vec2(19.244, 107.118), coord);
    return fract(sin(t) * 393919.062);
}

void main()
{
	if (inUvw.z > rand(inUvw.xy) + 10e-5) discard; // need to discard explicitly! 

	vec3 normal = normalize(inNormal);

	vec3 coolColor = blue * vec3(0.0f, 0.0f, 1.0f) + 0.2f * surfaceColor;
	vec3 warmColor = yellow * vec3(1.0f, 1.0f, 0.0f) + 0.6f * surfaceColor;

	float dp = -dot(normal, lightDir);
	float occlusion = 2.0f * (exp(attenuation * (inUvw.z - 1.0f)) - 1.0f) / (1.0f - exp(-attenuation)) + 1.0f; // rudimentary, the higher the grass blade, the less occluded
	float l = mix(occlusion, dp, diffuseContrib);
	vec3 color = 0.5f * ((warmColor + coolColor) + l * (warmColor - coolColor));

	outColor = vec4(color, 1.0f);
}