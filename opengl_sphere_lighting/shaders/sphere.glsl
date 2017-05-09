// *****************************************************************************
/**
 * Uniforms
 *
 */
uniform int u_SamplesPerPass;

uniform sampler2D u_PivotSampler;
uniform sampler2D u_RoughnessSampler;

struct Sphere {
	vec4 geometry; // xyz: pos; w: radius
	vec4 light;    // rgb: color; a: isLight
	vec4 brdf;     // r: roughness; yzw: reserved
	vec4 reserved;
};

layout(std140, binding = BUFFER_BINDING_SPHERES)
uniform Spheres {
	Sphere u_Spheres[SPHERE_COUNT];
};

struct Transform {
	mat4 model;
	mat4 modelView;
	mat4 modelViewProjection;
	mat4 viewInv;
};

layout(std140, row_major, binding = BUFFER_BINDING_TRANSFORMS)
uniform Transforms {
	Transform u_Transforms[SPHERE_COUNT];
};

layout(std140, binding = BUFFER_BINDING_RANDOM)
uniform Random {
	vec4 value[64];
} u_Random;

vec4 rand(int idx) { return u_Random.value[idx]; }
float hash(vec2 p)
{
	float h = dot(p, vec2(127.1, 311.7));
	return fract(sin(h) * 43758.5453123);
}


// *****************************************************************************
/**
 * Vertex Shader
 *
 * The shader outputs attributes relevant for shading in view space.
 */
#ifdef VERTEX_SHADER
layout(location = 0) in vec4 i_Position;
layout(location = 1) in vec4 i_TexCoord;
layout(location = 2) in vec4 i_Tangent1;
layout(location = 3) in vec4 i_Tangent2;
layout(location = 0) out vec4 o_Position;
layout(location = 1) out vec4 o_TexCoord;
layout(location = 2) out vec4 o_Tangent1;
layout(location = 3) out vec4 o_Tangent2;
layout(location = 4) flat out int o_SphereId;

void main(void)
{
	o_Position = u_Transforms[gl_InstanceID].modelView * i_Position;
	o_TexCoord = i_TexCoord;
	o_Tangent1 = u_Transforms[gl_InstanceID].modelView * i_Tangent1;
	o_Tangent2 = u_Transforms[gl_InstanceID].modelView * i_Tangent2;
	o_SphereId = gl_InstanceID;

	gl_Position = u_Transforms[gl_InstanceID].modelViewProjection * i_Position;
}
#endif // VERTEX_SHADER

// *****************************************************************************
/**
 * Fragment Shader
 *
 */
#ifdef FRAGMENT_SHADER
layout(location = 0) in vec4 i_Position;
layout(location = 1) in vec4 i_TexCoord;
layout(location = 2) in vec4 i_Tangent1;
layout(location = 3) in vec4 i_Tangent2;
layout(location = 4) flat in int i_SphereId;
layout(location = 0) out vec4 o_FragColor;

// helper function to extract the pivot parameters
// wo is assumed to be expressed in tangent space
vec3 extractPivot(vec3 wo, float alpha, out float brdfScale)
{
	// fetch pivot fit params
	float theta = acos(wo.z);
	vec2 fitLookup = vec2(sqrt(alpha), 2.0 * theta / 3.14159);
	fitLookup = fma(fitLookup, vec2(63.0 / 64.0), vec2(0.5 / 64.0));
	vec4 pivotParams = texture(u_PivotSampler, fitLookup);
	float pivotNorm = pivotParams.r;
	float pivotElev = pivotParams.g;
	vec3 pivot = pivotNorm * vec3(sin(pivotElev), 0, cos(pivotElev));

	// express the pivot in tangent space
	mat3 basis;
	basis[0] = wo.z < 0.999 ? normalize(wo - vec3(0, 0, wo.z)) : vec3(1, 0, 0);
	basis[1] = cross(vec3(0, 0, 1), basis[0]);
	basis[2] = vec3(0, 0, 1);
	pivot = basis * pivot;

	// return
	brdfScale = pivotParams.a;
	return pivot;
}

void main(void)
{
	// extract attributes
	vec3 wx = normalize(i_Tangent1.xyz);
	vec3 wy = normalize(i_Tangent2.xyz);
	vec3 wn = normalize(cross(wx, wy));
	vec3 wo = normalize(-i_Position.xyz);
	mat3 tg = transpose(mat3(wx, wy, wn));
	float alpha = max(5e-3, texture(u_RoughnessSampler, i_TexCoord.xy).r);

	// express data in tangent space
	wo = tg * wo;
	wn = vec3(0, 0, 1);

	// initialize emitted and outgoing radiance
	vec3 Le = u_Spheres[i_SphereId].light.rgb;
	vec3 Lo = vec3(0);

// -----------------------------------------------------------------------------
/**
 * Area Light Shading
 *
 * The surface is shaded with a spherical light. The shading computes 
 * the integral of a GGX BRDF against a spherical cap. The computation is
 * not exact: the GGX BRDF is approximated with a pivot distribution, which
 * can be integrated in closed form against spherical caps. For more details,
 * see my paper "A Spherical Cap Preserving Parameterization for Spherical 
 * Distributions".
 */
#if SHADE_PIVOT
	// fetch pivot fit params
	float brdfScale;
	vec3 pivot = extractPivot(wo, alpha, brdfScale);

	for (int i = 0; i < SPHERE_COUNT; ++i) {
		if (i_SphereId == i) continue;
		if (u_Spheres[i].light.a == 0.0) continue;
		vec3 spherePos = tg * (u_Spheres[i].geometry.xyz - i_Position.xyz);
		float sphereRadius = (u_Spheres[i].geometry.w);
		sphere s = sphere(spherePos, sphereRadius);

		Lo+= GGXSphereLightingPivotApprox(s, wo, pivot)
		   * u_Spheres[i].light.rgb;
	}
	Lo*= brdfScale;
	Lo+= Le;

	o_FragColor = vec4(Lo, 1);

// -----------------------------------------------------------------------------
/**
 * Area Light Shading with Spherical Light Importance Sampling
 *
 * The surface is shaded with a spherical light. The shading computes 
 * the integral of a GGX BRDF against a spherical cap. The computation is
 * exact and performed numerically with Monte Carlo importance sampling.
 * Note that most of these technique converge very slowly; they are provided
 * for pedagogical and debugging purposes.
 */
#elif (SHADE_MC_GGX || SHADE_MC_CAP || SHADE_MC_COS || SHADE_MC_H2 || SHADE_MC_S2)
	// iterate over all spheres
	for (int i = 0; i < SPHERE_COUNT; ++i) {
		if (i_SphereId == i) continue;
		if (u_Spheres[i].light.a == 0.0) continue;
		vec3 spherePos = tg * (u_Spheres[i].geometry.xyz - i_Position.xyz);
		float sphereRadius = u_Spheres[i].geometry.w;
		sphere s = sphere(spherePos, sphereRadius);
		vec3 Li = u_Spheres[i].light.rgb;
		float invSphereMagSqr = 1.0 / dot(s.pos, s.pos);
		vec3 capDir = s.pos * sqrt(invSphereMagSqr);
		float capCos = sqrt(1.0 - s.r * s.r * invSphereMagSqr);
		cap c = cap(capDir, capCos);

		// loop over all samples
		for (int j = 0; j < u_SamplesPerPass; ++j) {
			// compute a uniform sample
			float h1 = hash(gl_FragCoord.xy);
			float h2 = hash(gl_FragCoord.yx);
			vec2 u2 = mod(vec2(h1, h2) + rand(j).xy, vec2(1.0));
#if SHADE_MC_CAP
			vec3 wi = u2_to_cap(u2, c);
			float pdf = pdf_cap(wi, c);
#elif SHADE_MC_COS
			vec3 wi = u2_to_cos(u2);
			float pdf = pdf_cos(wi);
#elif SHADE_MC_H2
			vec3 wi = u2_to_h2(u2);
			float pdf = pdf_h2(wi);
#elif SHADE_MC_S2
			vec3 wi = u2_to_s2(u2);
			float pdf = pdf_s2(wi);
#elif SHADE_MC_GGX
			vec3 wm = ggx_sample(u2, wo, alpha);
			vec3 wi = 2.0 * wm * dot(wo, wm) - wo;
			float pdf = 0.0; // initialized below
#endif
			float pdf_dummy;
			float frp = ggx_evalp(wi, wo, alpha, pdf_dummy);
			float raySphereIntersection = pdf_cap(wi, c);
#if SHADE_MC_GGX
			pdf = pdf_dummy;
#endif

			if (pdf > 0.0 && raySphereIntersection > 0.0)
				Lo+= Li * frp / pdf;
		}
	}
	Lo+= Le * u_SamplesPerPass;
	o_FragColor = vec4(Lo, u_SamplesPerPass);

// -----------------------------------------------------------------------------
/**
 * Area Light Shading with MIS
 *
 * The surface is shaded with a spherical light. The shading computes 
 * the integral of a GGX BRDF against a spherical cap. The computation is
 * exact and performed numerically with Monte Carlo and MIS. Two strategies
 * are combined, namely a GGX VNDF strategy and a spherical cap strategy.
 * Such combinations are found in state of the art Monte Carlo offline 
 * renderers.
 */
#elif SHADE_MC_MIS
	// iterate over all spheres
	for (int i = 0; i < SPHERE_COUNT; ++i) {
		if (i_SphereId == i) continue;
		if (u_Spheres[i].light.a == 0.0) continue;
		vec3 spherePos = tg * (u_Spheres[i].geometry.xyz - i_Position.xyz);
		float sphereRadius = u_Spheres[i].geometry.w;
		sphere s = sphere(spherePos, sphereRadius);
		vec3 Li = u_Spheres[i].light.rgb;
		float invSphereMagSqr = 1.0 / dot(s.pos, s.pos);
		vec3 capDir = s.pos * sqrt(invSphereMagSqr);
		float capCos = sqrt(1.0 - s.r * s.r * invSphereMagSqr);
		cap c = cap(capDir, capCos);

		// loop over all samples
		for (int j = 0; j < u_SamplesPerPass; ++j) {
			// compute a uniform sample
			float h1 = hash(gl_FragCoord.xy);
			float h2 = hash(gl_FragCoord.yx);
			vec2 u2 = mod(vec2(h1, h2) + rand(j).xy, vec2(1.0));

			// importance sample the BRDF
			if (true) {
				vec3 wm = ggx_sample(u2, wo, alpha);
				vec3 wi = 2.0 * wm * dot(wo, wm) - wo;
				float pdf1;
				float frp = ggx_evalp(wi, wo, alpha, pdf1);
				float raySphereIntersection = pdf_cap(wi, c);

				// raytrace the sphere light
				if (pdf1 > 0.0 && raySphereIntersection > 0.0) {
					float pdf2 = raySphereIntersection;
					float misWeight = pdf1 * pdf1;
					float misNrm = pdf1 * pdf1 + pdf2 * pdf2;

					Lo+= Li * frp / pdf1 * misWeight / misNrm;
				}
			}

			// importance sample the spherical cap
			if (true) {
				vec3 wi = u2_to_cap(u2, c);
				float pdf1;
				float frp = ggx_evalp(wi, wo, alpha, pdf1);
				float pdf2 = pdf_cap(wi, c);

				if (pdf2 > 0.0) {
					float misWeight = pdf2 * pdf2;
					float misNrm = pdf1 * pdf1 + pdf2 * pdf2;

					Lo+= Li * frp / pdf2 * misWeight / misNrm;
				}
			}
		}
	}
	Lo+= Le * u_SamplesPerPass;
	o_FragColor = vec4(Lo, u_SamplesPerPass);

// -----------------------------------------------------------------------------
/**
 * Area Light Shading with Joint MIS
 *
 * The surface is shaded with a spherical light. The shading computes 
 * the integral of a GGX BRDF against a spherical cap. The computation is
 * exact and performed numerically with Monte Carlo and MIS. Two strategies
 * are combined, namely a GGX VNDF strategy and a pivot transformed spherical 
 * cap strategy. The pivot is chosen so as to produce a density close to that 
 * of the GGX microfacet BRDF. This combination is faster than state of the art
 * MIS techniques.
 */
#elif SHADE_MC_MIS_JOINT
	// fetch pivot fit params
	float brdfScale; // this won't be used here
	vec3 pivot = extractPivot(wo, alpha, brdfScale);

	// iterate over all spheres
	for (int i = 0; i < SPHERE_COUNT; ++i) {
		if (i_SphereId == i) continue;
		if (u_Spheres[i].light.a == 0.0) continue;
		vec3 spherePos = tg * (u_Spheres[i].geometry.xyz - i_Position.xyz);
		float sphereRadius = u_Spheres[i].geometry.w;
		sphere s = sphere(spherePos, sphereRadius);
		vec3 Li = u_Spheres[i].light.rgb;
		float invSphereMagSqr = 1.0 / dot(s.pos, s.pos);
		vec3 capDir = s.pos * sqrt(invSphereMagSqr);
		float capCos = sqrt(1.0 - s.r * s.r * invSphereMagSqr);
		cap c = cap(capDir, capCos);
		cap c_std = cap_to_pcap(c, pivot);

		if (c.z < 0.99) {
			// Joint MIS: loop over all samples
			for (int j = 0; j < u_SamplesPerPass; ++j) {
				// compute a uniform sample
				float h1 = hash(gl_FragCoord.xy);
				float h2 = hash(gl_FragCoord.yx);
				vec2 u2 = mod(vec2(h1, h2) + rand(j).xy, vec2(1.0));

				// importance sample the BRDF
				if (true) {
					vec3 wm = ggx_sample(u2, wo, alpha);
					vec3 wi = 2.0 * wm * dot(wo, wm) - wo;
					float pdf1;
					float frp = ggx_evalp(wi, wo, alpha, pdf1);
					float raySphereIntersection = pdf_cap(wi, c);

					// raytrace the sphere light
					if (pdf1 > 0.0 && raySphereIntersection > 0.0) {
						float pdf2 = pdf_pcap_fast(wi, c_std, pivot);
						float misWeight = pdf1 * pdf1;
						float misNrm = pdf1 * pdf1 + pdf2 * pdf2;

						Lo+= Li * frp / pdf1 * misWeight / misNrm;
					}
				}

				// importance sample the pivot transformed spherical cap
				if (true) {
					vec3 wi = u2_to_pcap(u2, c_std, pivot);
					float pdf1;
					float frp = ggx_evalp(wi, wo, alpha, pdf1);
					float pdf2 = pdf_pcap_fast(wi, c_std, pivot);

					if (pdf2 > 0.0) {
						float misWeight = pdf2 * pdf2;
						float misNrm = pdf1 * pdf1 + pdf2 * pdf2;

						Lo+= Li * frp / pdf2 * misWeight / misNrm;
					}
				}
			}
		} else {
			// classic MIS: loop over all samples
			for (int j = 0; j < u_SamplesPerPass; ++j) {
				// compute a uniform sample
				float h1 = hash(gl_FragCoord.xy);
				float h2 = hash(gl_FragCoord.yx);
				vec2 u2 = mod(vec2(h1, h2) + rand(j).xy, vec2(1.0));

				// importance sample the BRDF
				if (true) {
					vec3 wm = ggx_sample(u2, wo, alpha);
					vec3 wi = 2.0 * wm * dot(wo, wm) - wo;
					float pdf1;
					float frp = ggx_evalp(wi, wo, alpha, pdf1);
					float raySphereIntersection = pdf_cap(wi, c);

					// raytrace the sphere light
					if (pdf1 > 0.0 && raySphereIntersection > 0.0) {
						float pdf2 = pdf_cap(wi, c);
						float misWeight = pdf1 * pdf1;
						float misNrm = pdf1 * pdf1 + pdf2 * pdf2;

						Lo+= Li * frp / pdf1 * misWeight / misNrm;
					}
				}

				// importance sample the spherical cap
				if (true) {
					vec3 wi = u2_to_cap(u2, c);
					float pdf1;
					float frp = ggx_evalp(wi, wo, alpha, pdf1);
					float pdf2 = pdf_cap(wi, c);

					if (pdf2 > 0.0) {
						float misWeight = pdf2 * pdf2;
						float misNrm = pdf1 * pdf1 + pdf2 * pdf2;

						Lo+= Li * frp / pdf2 * misWeight / misNrm;
					}
				}
			}
		}
	}
	Lo+= Le * u_SamplesPerPass;
	o_FragColor = vec4(Lo, u_SamplesPerPass);

// -----------------------------------------------------------------------------
/**
 * Debug Shading
 *
 * Do whatever you like in here.
 */
#elif SHADE_DEBUG
	o_FragColor = vec4(0, 1, 0, 1);
// -----------------------------------------------------------------------------
#endif // SHADE

}
#endif // FRAGMENT_SHADER


