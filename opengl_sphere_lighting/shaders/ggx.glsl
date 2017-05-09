#line 1

vec3 VNDFSampleGGX(
	vec2 u2,
	vec3 omega_i,
	float alpha
) {
	float U1 = u2.x;
	float U2 = u2.y;
	// 1. stretch omega_i
	omega_i = vec3(alpha, alpha, 1.0)*omega_i;

	// normalize
	omega_i = normalize(omega_i);

	float cos_theta = omega_i.z;
	float sin_theta = sqrt(1.0 - cos_theta*cos_theta);

	// 2. simulate P22_{omega_i}(x_slope, y_slope, 1, 1)
	float slope_x, slope_y;

	vec2 cos_sin_phi;

	// special case (normal incidence)
	if (cos_theta >= 0.9999999)
	{
		float r = sqrt(U1/(1.0 - U1));
		float p = 6.28318530718*U2;
		slope_x = r*cos(p);
		slope_y = r*sin(p);
		cos_sin_phi = vec2(1.0, 0.0);
	}
	else
	{
		// sample slope_x
		float G1 = 2.0*cos_theta/(cos_theta + 1.0);
		float A = 2.0*U1/G1 - 1.0;
		float tmp = 1.0/(A*A - 1.0);
		float B = sin_theta/cos_theta;
		float D = sqrt(B*B*tmp*tmp - (A*A - B*B)*tmp);
		float slope_x_1 = B*tmp - D;
		float slope_x_2 = B*tmp + D;

		slope_x = (A < 0.0 || slope_x_2 > 1.0/B) ? slope_x_1 : slope_x_2;

		// sample slope_y
		float S;
		if (U2 > 0.5)
		{
			S = 1.0;
			U2 = 2.0*(U2-0.5);
		}
		else
		{
			S = -1.0;
			U2 = 2.0*(0.5-U2);
		}

		float z = (U2*(U2*(U2*0.27385 - 0.73369) + 0.46341)) / (U2*(U2*(U2*0.093073 + 0.309420) - 1.000000) + 0.597999);

		slope_y = S*z*sqrt(1.0 + slope_x*slope_x);

		cos_sin_phi = normalize(omega_i.xy);
	}

	// 3. rotate
	float tmp = cos_sin_phi.x*slope_x - cos_sin_phi.y*slope_y;
	slope_y   = cos_sin_phi.y*slope_x + cos_sin_phi.x*slope_y;
	slope_x   = tmp;

	// 4. unstretch
	slope_x = alpha*slope_x;
	slope_y = alpha*slope_y;

	// 5. compute normal
	return normalize(vec3(-slope_x, -slope_y, 1.0));
}


// *****************************************************************************
/**
 * GGX Functions
 *
 */
float ggx_evalp(vec3 wi, vec3 wo, float alpha, out float pdf)
{
	if (wo.z > 0.0 && wi.z > 0.0) {
		vec3 wh = normalize(wi + wo);
		vec3 wh_xform = vec3(wh.xy / alpha, wh.z);
		vec3 wi_xform = vec3(wi.xy * alpha, wi.z);
		vec3 wo_xform = vec3(wo.xy * alpha, wo.z);
		float wh_xform_mag = length(wh_xform);
		float wi_xform_mag = length(wi_xform);
		float wo_xform_mag = length(wo_xform);
		wh_xform/= wh_xform_mag; // normalize
		wi_xform/= wi_xform_mag; // normalize
		wo_xform/= wo_xform_mag; // normalize
		float sigma_i = 0.5 + 0.5 * wi_xform.z;
		float sigma_o = 0.5 + 0.5 * wo_xform.z;
		float Gi = clamp(wi.z, 0.0, 1.0) / (sigma_i * wi_xform_mag);
		float Go = clamp(wo.z, 0.0, 1.0) / (sigma_o * wo_xform_mag);
		float J = alpha * alpha * wh_xform_mag * wh_xform_mag * wh_xform_mag;
		float Dvis = clamp(dot(wo_xform, wh_xform), 0.0, 1.0) / (sigma_o * 3.141592654 * J);
		float Gcond = Gi / (Gi + Go - Gi * Go);
		float cos_theta_d = dot(wh, wo);

		pdf = (Dvis / (cos_theta_d * 4.0));
		return pdf * Gcond;
	}
	pdf = 0.0;
	return 0.0;
}

vec3 ggx_sample_std(vec2 u2, vec3 wo)
{
	// sample the disk
	float a = 1.0 / (1.0 + wo.z);
	float r = sqrt(u2.x);
	vec2 disk;
	if (u2.y < a) {
		float phi = u2.y / a * 3.141592654;

		disk.x = r * cos(phi);
		disk.y = r * sin(phi);
	} else {
		float phi = 3.141592654 + 3.141592654 * ((u2.y - a) / (1.0 - a));

		disk.x = r * cos(phi);
		disk.y = r * sin(phi) * wo.z;
	}

	// extract normal
	vec3 t1 = (wo.z < 0.9999) ? normalize(vec3(wo.y, -wo.x, 0))
	                          : vec3(1,0,0);
	vec3 t2 = cross(t1, wo);
	vec3 wm = disk.x * t1 + disk.y * t2 + wo * sqrt(1.0 - dot(disk, disk));

	return wm;
}

vec3 ggx_sample(vec2 u2, vec3 wo, float alpha)
{
#if 0 // TODO: debug
	vec3 wo_xform = normalize(vec3(wo.xy * alpha, wo.z));
	vec3 wm_std = ggx_sample_std(u2, wo);
	vec3 wm = normalize(vec3(wm_std.xy * alpha, wm_std.z));

	return wm;
#else
	return VNDFSampleGGX(u2, wo, alpha);
#endif
}


