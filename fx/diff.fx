/**
 * îwåiç∑ï™ÉGÉtÉFÉNÉg
 */

float width;
float height;
int samples;

texture frame1; // ÉJÉåÉìÉgâfëú
texture frame2; // ãPìxïΩãœ
texture frame3; // ãPìxêUïù


sampler sampler1 = sampler_state
{
	Texture = <frame1>;

	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;

	AddressU = Clamp;
	AddressV = Clamp;
};

sampler sampler2 = sampler_state
{
	Texture = <frame2>;

	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;

	AddressU = Clamp;
	AddressV = Clamp;
};

sampler sampler3 = sampler_state
{
	Texture = <frame3>;

	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;

	AddressU = Clamp;
	AddressV = Clamp;
};


// ÉtÉåÅ[ÉÄç∑ï™
void diff(float4 in_color: COLOR0, float2 pos: TEXCOORD0, out float4 out_color: COLOR0)
{
	float4 p1 = tex2D(sampler1, pos);
	float4 p2 = tex2D(sampler2, pos);
	float y = 0.298912 * p1.r + 0.586611 * p1.g + 0.114478 * p1.b;
	float t = 1.0 / samples;
	float i = min((1 - t) * p2.g + t * y, 1.0);
	float lookup = 0;
	if (samples <= 100) {
		//t = 1;
	} else {
		if (p2.r < 0.5) {
			// îwåi
			t = 1.0 / 500;
		} else {
			// ëOåi
			t = 1.0 / 2000;
		}
		lookup = y > 0.063 && (y < i - p2.b || y > i + p2.b)?1 :0;
	}
	float amp = (1 - t) * p2.b + t * sqrt(2 * abs(y - p2.g));
	out_color = float4(lookup, i, amp, 1);
}

// ãPìxêUïù
void intensityAmplitude(float4 in_color: COLOR0, float2 pos: TEXCOORD0, out float4 out_color: COLOR0)
{
	float4 p1 = tex2D(sampler1, pos);
	float4 p2 = tex2D(sampler2, pos);
	float y = 0.298912 * p1.r + 0.586611 * p1.g + 0.114478 * p1.b;

	float4 p3 = tex2D(sampler3, pos);
	float t = 1.0 / 1;
	float amp = (1 - t) * p3.g + t * sqrt(2 * abs(y - p2.g));
	out_color = float4(amp, amp, amp, 1);
}



// éwíËï™à⁄ìÆÇµÇΩà íuÇÃÉsÉNÉZÉãéÊìæ
float2 shiftTexture(float2 tex, const float shiftX, const float shiftY)
{
	tex.x = tex.x + 1.0 / width * shiftX;
	tex.y = tex.y + 1.0 / height * shiftY;

	return tex;
}

void maxFilter(float4 in_color: COLOR0, float2 pos: TEXCOORD0, out float4 out_color: COLOR0)
{
	float4 p00 = tex2D(sampler2, shiftTexture(pos, -1, -1));
	float4 p01 = tex2D(sampler2, shiftTexture(pos,  0, -1));
	float4 p02 = tex2D(sampler2, shiftTexture(pos,  1, -1));
	float4 p10 = tex2D(sampler2, shiftTexture(pos, -1,  0));
	float4 p11 = tex2D(sampler2, shiftTexture(pos,  0,  0));
	float4 p12 = tex2D(sampler2, shiftTexture(pos,  1,  0));
	float4 p20 = tex2D(sampler2, shiftTexture(pos, -1,  1));
	float4 p21 = tex2D(sampler2, shiftTexture(pos,  0,  1));
	float4 p22 = tex2D(sampler2, shiftTexture(pos,  1,  1));
	out_color = p11;
	out_color.r = max(p00.r, p01.r);
	out_color.r = max(out_color.r, p02.r);
	out_color.r = max(out_color.r, p10.r);
	out_color.r = max(out_color.r, p11.r);
	out_color.r = max(out_color.r, p12.r);
	out_color.r = max(out_color.r, p20.r);
	out_color.r = max(out_color.r, p21.r);
	out_color.r = max(out_color.r, p22.r);
}

void minFilter(float4 in_color: COLOR0, float2 pos: TEXCOORD0, out float4 out_color: COLOR0)
{
	float4 p00 = tex2D(sampler2, shiftTexture(pos, -1, -1));
	float4 p01 = tex2D(sampler2, shiftTexture(pos,  0, -1));
	float4 p02 = tex2D(sampler2, shiftTexture(pos,  1, -1));
	float4 p10 = tex2D(sampler2, shiftTexture(pos, -1,  0));
	float4 p11 = tex2D(sampler2, shiftTexture(pos,  0,  0));
	float4 p12 = tex2D(sampler2, shiftTexture(pos,  1,  0));
	float4 p20 = tex2D(sampler2, shiftTexture(pos, -1,  1));
	float4 p21 = tex2D(sampler2, shiftTexture(pos,  0,  1));
	float4 p22 = tex2D(sampler2, shiftTexture(pos,  1,  1));
	out_color = p11;
	out_color.r = min(p00.r, p01.r);
	out_color.r = min(out_color.r, p02.r);
	out_color.r = min(out_color.r, p10.r);
	out_color.r = min(out_color.r, p11.r);
	out_color.r = min(out_color.r, p12.r);
	out_color.r = min(out_color.r, p20.r);
	out_color.r = min(out_color.r, p21.r);
	out_color.r = min(out_color.r, p22.r);
}



technique diffTechnique
{
	pass P0
	{
		pixelShader  = compile ps_2_0 diff();
	}
	pass P1
	{
		//pixelShader  = compile ps_2_0 maxFilter();
	}
	pass P2
	{
		//pixelShader  = compile ps_2_0 minFilter();
	}
}
