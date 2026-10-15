shader_type canvas_item;

uniform int u_theme = 0; // 0 = Sunset, 1 = Noon, 2 = Neon
uniform float u_time = 0.0;
uniform vec2 u_res = vec2(1280.0, 720.0);
uniform vec4 u_ball = vec4(640.0, 480.0, 42.0, 0.0);
uniform vec4 u_p0 = vec4(240.0, 500.0, 0.0, 0.0);
uniform vec4 u_p1 = vec4(470.0, 430.0, 0.0, 0.0);
uniform vec4 u_p2 = vec4(1040.0, 460.0, 0.0, 0.0);
uniform vec4 u_p3 = vec4(810.0, 520.0, 0.0, 0.0);

float hash21(vec2 p) {
	p = fract(p * vec2(123.34, 456.21));
	p += dot(p, p + 45.32);
	return fract(p.x * p.y);
}

float noise2d(vec2 p) {
	vec2 i = floor(p);
	vec2 f = fract(p);
	f = f * f * (3.0 - 2.0 * f);
	float a = hash21(i);
	float b = hash21(i + vec2(1.0, 0.0));
	float c = hash21(i + vec2(0.0, 1.0));
	float d = hash21(i + vec2(1.0, 1.0));
	return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

float player_shadow(vec2 px, vec4 pl) {
	vec2 d = (px - pl.xy) * vec2(1.0, 2.1);
	float r = max(14.0, 28.0 - pl.z * 0.16);
	float dist2 = dot(d, d);
	if (dist2 < r * r) {
		return (1.0 - dist2 / (r * r)) * 0.45;
	}
	return 0.0;
}

void fragment() {
	vec2 pix_uv = floor(UV * vec2(640.0, 360.0)) / vec2(640.0, 360.0);
	vec2 px = pix_uv * u_res;

	vec3 sky_top = vec3(0.11, 0.07, 0.24);
	vec3 sky_hor = vec3(0.98, 0.42, 0.34);
	vec3 sun_col = vec3(1.0, 0.84, 0.42);
	vec3 ocean_deep = vec3(0.05, 0.22, 0.38);
	vec3 ocean_shallow = vec3(0.12, 0.56, 0.68);
	vec3 sand_lit = vec3(0.94, 0.76, 0.55);
	vec3 sand_shadow = vec3(0.68, 0.46, 0.34);
	vec3 light_dir = normalize(vec3(-0.45, -0.52, 0.72));

	if (u_theme == 1) {
		sky_top = vec3(0.12, 0.48, 0.88);
		sky_hor = vec3(0.58, 0.86, 0.98);
		sun_col = vec3(1.0, 0.98, 0.86);
		ocean_deep = vec3(0.04, 0.35, 0.64);
		ocean_shallow = vec3(0.16, 0.78, 0.84);
		sand_lit = vec3(0.98, 0.90, 0.74);
		sand_shadow = vec3(0.76, 0.64, 0.48);
		light_dir = normalize(vec3(-0.18, -0.28, 0.94));
	} else if (u_theme == 2) {
		sky_top = vec3(0.04, 0.02, 0.12);
		sky_hor = vec3(0.45, 0.08, 0.52);
		sun_col = vec3(1.0, 0.18, 0.68);
		ocean_deep = vec3(0.04, 0.08, 0.24);
		ocean_shallow = vec3(0.08, 0.42, 0.62);
		sand_lit = vec3(0.32, 0.25, 0.48);
		sand_shadow = vec3(0.15, 0.10, 0.28);
		light_dir = normalize(vec3(0.35, -0.45, 0.82));
	}

	if (pix_uv.y < 0.18) {
		float t = pix_uv.y / 0.18;
		vec3 col = mix(sky_top, sky_hor, pow(t, 0.75));
		vec2 sun_pos = vec2(0.5, 0.11);
		float d_sun = length((pix_uv - sun_pos) * vec2(u_res.x / u_res.y, 1.0));
		if (d_sun < 0.065) {
			float stripe = step(0.22, fract(pix_uv.y * 95.0 - u_time * 0.4));
			col = mix(col, sun_col, stripe);
		} else {
			col += sun_col * exp(-d_sun * 14.0) * 0.45;
		}
		COLOR = vec4(col, 1.0);
	} else {
		float wave_line = 0.315 + 0.012 * sin(pix_uv.x * 18.0 + u_time * 2.1) + 0.006 * cos(pix_uv.x * 37.0 - u_time * 1.4);
		if (pix_uv.y < wave_line) {
			float wt = (pix_uv.y - 0.18) / max(0.001, (wave_line - 0.18));
			vec3 water = mix(ocean_deep, ocean_shallow, pow(wt, 0.85));
			float caustic = noise2d(vec2(pix_uv.x * 48.0 + u_time * 0.8, pix_uv.y * 95.0 - u_time * 1.6));
			water += vec3(0.18, 0.32, 0.38) * step(0.68, caustic) * wt;
			float shimmer_x = abs(pix_uv.x - 0.5 + 0.02 * sin(pix_uv.y * 80.0 + u_time * 3.0));
			if (shimmer_x < 0.08 * (1.0 - wt * 0.5)) {
				water = mix(water, sun_col, 0.55);
			}
			if (wave_line - pix_uv.y < 0.014) {
				water = mix(water, vec3(0.94, 0.98, 1.0), 0.85);
			}
			COLOR = vec4(water, 1.0);
		} else {
			float h_c = noise2d(pix_uv * vec2(140.0, 220.0)) * 0.18;
			float h_x = noise2d((pix_uv + vec2(0.002, 0.0)) * vec2(140.0, 220.0)) * 0.18;
			float h_y = noise2d((pix_uv + vec2(0.0, 0.002)) * vec2(140.0, 220.0)) * 0.18;
			vec3 sand_normal = normalize(vec3((h_c - h_x) * 4.5, (h_c - h_y) * 4.5, 1.0));
			float ndl = clamp(dot(sand_normal, vec3(-light_dir.x, -light_dir.y, light_dir.z)), 0.0, 1.0);
			vec3 sand_col = mix(sand_shadow, sand_lit, smoothstep(0.15, 0.85, ndl));

			float tide_wet = clamp(1.0 - (pix_uv.y - wave_line) / 0.045, 0.0, 1.0);
			if (tide_wet > 0.0) {
				sand_col = mix(sand_col, sand_col * 0.68 + ocean_shallow * 0.18, tide_wet * 0.75);
			}

			float sparkle = hash21(floor(pix_uv * vec2(640.0, 360.0)));
			if (sparkle > 0.992) {
				sand_col += sun_col * 0.35;
			}

			float y_norm = (px.y - 295.0) / (655.0 - 295.0);
			if (y_norm >= -0.02 && y_norm <= 1.02) {
				float half_w = mix(340.0, 445.0, clamp(y_norm, 0.0, 1.0));
				float left_x = 640.0 - half_w;
				float right_x = 640.0 + half_w;
				bool on_border = (abs(px.x - left_x) < 4.5 && y_norm >= 0.0 && y_norm <= 1.0) ||
				                 (abs(px.x - right_x) < 4.5 && y_norm >= 0.0 && y_norm <= 1.0) ||
				                 (abs(px.y - 295.0) < 3.5 && px.x >= left_x && px.x <= right_x) ||
				                 (abs(px.y - 655.0) < 4.0 && px.x >= left_x && px.x <= right_x) ||
				                 (abs(px.x - 640.0) < 3.0 && y_norm >= 0.0 && y_norm <= 1.0);
				if (on_border) {
					vec3 line_col = (u_theme == 2) ? vec3(0.18, 0.92, 1.0) : vec3(0.98, 0.96, 0.90);
					sand_col = mix(sand_col, line_col, 0.88);
				}
			}

			float sh = 0.0;
			sh = max(sh, player_shadow(px, u_p0));
			sh = max(sh, player_shadow(px, u_p1));
			sh = max(sh, player_shadow(px, u_p2));
			sh = max(sh, player_shadow(px, u_p3));

			vec2 db = (px - u_ball.xy) * vec2(1.0, 1.85);
			float rb = max(9.0, 20.0 - u_ball.z * 0.09);
			if (dot(db, db) < rb * rb) {
				sh = max(sh, (1.0 - dot(db, db) / (rb * rb)) * 0.55);
			}
			sand_col *= (1.0 - sh);

			if (u_ball.w > 0.1) {
				float d_glow = length(px - u_ball.xy);
				vec3 glow_col = (u_ball.w > 1.2) ? vec3(1.0, 0.38, 0.05) : vec3(1.0, 0.25, 0.55);
				sand_col += glow_col * exp(-d_glow * 0.022) * 0.42 * u_ball.w;
			}

			COLOR = vec4(sand_col, 1.0);
		}
	}
}
