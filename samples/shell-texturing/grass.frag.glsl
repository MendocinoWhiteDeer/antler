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
This shader uses Gooch shading with a simplified ambient occlusion contribution.

Reference:

Amy Gooch, Bruce Gooch, Peter Shirley, and Elaine Cohen. 1998.
A non-photorealistic lighting model for automatic technical illustration. In Proceedings of the 25th annual conference on Computer graphics and interactive techniques (SIGGRAPH '98).
Association for Computing Machinery, New York, NY, USA, 447–452. https://doi.org/10.1145/280814.280950
*/

#version 460

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec3 inUvw;

layout(location = 0) out vec4 outColor;

layout(set = 2, binding = 0) uniform GrassData
{
	int resolution;
	float thickness;
	float occlusionAttenuation;
	float diffuseContrib; // between 0.0f and 1.0, it is a lerp factor between simplified ambient occlusion and the lambertian diffuse

} grass; 

const vec3 lightDir = vec3(0.0f, 0.0f, -1.0f);		
const vec3 surfaceColor = vec3(0.1f, 0.9f, 0.1f);
const float blue = 0.4f;
const float yellow = 0.4f;

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
	vec2 c = inUvw.xy * grass.resolution + 0.5f;
	float noise = rand(clamp(floor(c), 0.0f, grass.resolution));
	vec2 nudge = 0.25f * (2.0f * vec2(rand(vec2(1002.78 + 21.1 * noise, 9652411.3 - 3.67 * noise)), rand(vec2(19.22 * noise - 48382.7, 901.73 * noise + 111834.326))) - 1.0f);
	vec2 local = 2.0f * fract(c + nudge) - 1.0f;
	float r =  (1.0f - length(nudge)) * grass.thickness * (noise - inUvw.z);

	// r > rmax gives the grass a conical look
	if (inUvw.z > 10e-5 && length(local) > r) discard; // need to discard explicitly

	vec3 normal = normalize(inNormal);

	vec3 coolColor = blue * vec3(0.0f, 0.0f, 1.0f) + 0.1f * surfaceColor;
	vec3 warmColor = yellow * vec3(1.0f, 1.0f, 0.0f) + 0.7f * surfaceColor;

	float dp = -dot(normal, lightDir);
	// rudimentary, the higher the grass blade, the less occluded; it is designed to be between -1 and 1
	// If you do a Taylor expansion of the exponential ratio with respect to the attenuation, the first term is 2 * z - 1, the error = |(z - 1 ) * z * attenuation| <= 0.25 * attenuation for z in [0, 1]
	// So 4e-2 should be a good threshold value. The attenuation can't be plugged in as zero in the ratio or you are dividing by zero!
	float occlusion = 2.0f * inUvw.z - 1.0f;
	if (grass.occlusionAttenuation > 4e-2)
	   occlusion = 2.0f * (exp(grass.occlusionAttenuation * (inUvw.z - 1.0f)) - 1.0f) / (1.0f - exp(-grass.occlusionAttenuation)) + 1.0f; 
	float l = mix(occlusion, dp, grass.diffuseContrib);
	vec3 color = 0.5f * ((warmColor + coolColor) + l * (warmColor - coolColor));

	outColor = vec4(color, 1.0f);
}