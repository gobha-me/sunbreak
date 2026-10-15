shader_type canvas_item;

uniform sampler2D u_tex_normal : filter_linear;
uniform float u_flip = 1.0;
uniform vec2 u_prop_scale = vec2(1.12, 1.10);
uniform vec3 u_zone_bust_l = vec3(0.50, 0.37, 0.068);
uniform vec3 u_zone_bust_r = vec3(0.57, 0.37, 0.068);
uniform vec3 u_zone_hip_l = vec3(0.46, 0.56, 0.075);
uniform vec3 u_zone_hip_r = vec3(0.53, 0.56, 0.075);
uniform vec3 u_zone_hair = vec3(0.34, 0.22, 0.14);

uniform vec2 u_disp_bust_l = vec2(0.0);
uniform vec2 u_disp_bust_r = vec2(0.0);
uniform vec2 u_disp_hip_l = vec2(0.0);
uniform vec2 u_disp_hip_r = vec2(0.0);
uniform vec2 u_disp_hair = vec2(0.0);

uniform vec3 u_suit_color = vec3(0.96, 0.20, 0.42);
uniform vec3 u_trim_color = vec3(1.00, 0.82, 0.28);
uniform vec3 u_hair_color = vec3(0.62, 0.22, 0.24);
uniform vec3 u_skin_tint = vec3(0.98, 0.78, 0.67);
uniform vec3 u_light_dir = vec3(-0.45, -0.52, 0.72);
uniform vec3 u_sun_color = vec3(1.0, 0.84, 0.42);
uniform float u_wetness = 0.25;
uniform float u_sand_coat = 0.08;
uniform float u_on_fire = 0.0;

float hash21(vec2 p) {
	p = fract(p * vec2(123.34, 456.21));
	p += dot(p, p + 45.32);
	return fract(p.x * p.y);
}

void apply_soft_zone(
	vec2 uv,
	vec3 zone,
	vec2 disp,
	float scale_boost,
	inout vec2 warped_uv,
	inout vec2 normal_tilt,
	out float weight_out
) {
	vec2 delta = (uv - zone.xy) / max(0.01, zone.z);
	float r2 = dot(delta, delta);
	weight_out = 0.0;
	if (r2 < 1.0) {
		float w = (1.0 - r2);
		w = w * w;
		weight_out = w;
		float radial_scale = 1.0 - (scale_boost - 1.0) * 0.42 * w;
		vec2 scaled_uv = zone.xy + (uv - zone.xy) * radial_scale;
		warped_uv += (scaled_uv - uv) - disp * w;
		normal_tilt += disp * w * 5.8 + (uv - zone.xy) * (scale_boost - 1.0) * w * 2.5;
	}
}

void fragment() {
	vec2 base_uv = UV;
	vec2 warped_uv = base_uv;
	vec2 normal_tilt = vec2(0.0);
	float w_bl = 0.0;
	float w_br = 0.0;
	float w_hl = 0.0;
	float w_hr = 0.0;
	float w_hair = 0.0;

	apply_soft_zone(base_uv, u_zone_bust_l, u_disp_bust_l, u_prop_scale.x, warped_uv, normal_tilt, w_bl);
	apply_soft_zone(base_uv, u_zone_bust_r, u_disp_bust_r, u_prop_scale.x, warped_uv, normal_tilt, w_br);
	apply_soft_zone(base_uv, u_zone_hip_l, u_disp_hip_l, u_prop_scale.y, warped_uv, normal_tilt, w_hl);
	apply_soft_zone(base_uv, u_zone_hip_r, u_disp_hip_r, u_prop_scale.y, warped_uv, normal_tilt, w_hr);
	apply_soft_zone(base_uv, u_zone_hair, u_disp_hair, 1.0, warped_uv, normal_tilt, w_hair);

	vec4 tex_col = texture(TEXTURE, clamp(warped_uv, vec2(0.0), vec2(1.0)));
	if (tex_col.a < 0.18) {
		discard;
	}

	vec3 col = tex_col.rgb;
	float lum = dot(col, vec3(0.299, 0.587, 0.114));
	float red_over_green = col.r - col.g;
	float yellowness = min(col.r, col.g) - col.b;

	bool is_hair_envelope = warped_uv.y < 0.26 || w_hair > 0.08 || (warped_uv.x < 0.44 && warped_uv.y < 0.64);
	bool is_hair_pixel = is_hair_envelope && (col.r > 0.22) && (col.g < 0.22) && (red_over_green > 0.20);
	bool is_bikini_pixel = !is_hair_envelope && (col.r > 0.56) && (red_over_green > 0.48) && (col.g <= col.b * 1.42);

	if (is_hair_pixel) {
		col = u_hair_color * (0.42 + lum * 1.35);
	} else if (is_bikini_pixel) {
		col = u_suit_color * (0.38 + lum * 1.18);
	} else if (yellowness > 0.26 && col.r > 0.72 && col.g > 0.58 && (w_bl + w_br + w_hl + w_hr) > 0.05) {
		col = mix(col, u_trim_color * (0.55 + lum * 0.75), 0.75);
	} else if (col.r > col.g && col.g > col.b && lum > 0.22) {
		col *= mix(vec3(1.0), u_skin_tint / vec3(0.98, 0.78, 0.67), 0.55);
	}

	vec3 n_sample = texture(u_tex_normal, clamp(warped_uv, vec2(0.0), vec2(1.0))).xyz * 2.0 - 1.0;
	n_sample.x *= u_flip;
	vec3 n = normalize(vec3(n_sample.x + normal_tilt.x * u_flip, n_sample.y + normal_tilt.y, max(0.25, n_sample.z)));
	vec3 l = normalize(vec3(-u_light_dir.x, -u_light_dir.y, u_light_dir.z));
	float ndl = clamp(dot(n, l), 0.0, 1.0);

	float soft_mask = clamp(w_bl + w_br + w_hl + w_hr, 0.0, 1.0);
	float bounce_shade = 0.88 + 0.24 * ndl;
	col *= mix(0.94 + 0.12 * ndl, bounce_shade, soft_mask);

	vec3 v = vec3(0.0, 0.0, 1.0);
	vec3 h = normalize(l + v);
	float spec_power = mix(18.0, 36.0, u_wetness);
	float spec = pow(max(0.0, dot(n, h)), spec_power);
	float spec_mask = (0.35 + 0.65 * soft_mask) * (0.25 + 0.75 * u_wetness);
	col += u_sun_color * spec * spec_mask * 0.68;

	if (u_sand_coat > 0.08 && warped_uv.y > 0.46) {
		vec2 grain_cell = floor(warped_uv * vec2(128.0, 160.0));
		float h_grain = hash21(grain_cell);
		float lower_mask = smoothstep(0.46, 0.78, warped_uv.y);
		if (h_grain > (1.0 - u_sand_coat * 0.38 * lower_mask) && lum > 0.18) {
			col = mix(col, vec3(0.96, 0.80, 0.55), 0.72);
		}
	}

	if (u_on_fire > 0.05) {
		float rim = 1.0 - max(0.0, n.z);
		col += vec3(1.0, 0.42, 0.08) * pow(rim, 1.8) * u_on_fire * 0.85;
	}

	COLOR = vec4(clamp(col, vec3(0.0), vec3(1.0)), tex_col.a);
}
