/**
 * 背景差分エフェクト
 */

float width;
float height;
float subtract;

texture frame1;
texture frame2;
texture frame3;
texture result1;
texture result2;


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

sampler sampler4 = sampler_state
{
	Texture = <result1>;

	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;

	AddressU = Clamp;
	AddressV = Clamp;
};

sampler sampler5 = sampler_state
{
	Texture = <result2>;

	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;

	AddressU = Clamp;
	AddressV = Clamp;
};



// 指定分移動した位置のピクセル取得
float2 shiftTexture(float2 tex, const float shiftX, const float shiftY)
{
	tex.x = tex.x + 1.0 / width * shiftX;
	tex.y = tex.y + 1.0 / height * shiftY;

	return tex;
}

float2 mosaicTexture(float2 tex, const float pixel)
{
	float x = width  / (floor(width  * tex.x / pixel) * pixel);
	float y = height / (floor(height * tex.y / pixel) * pixel);

	return float2(x, y);
}

// RGB -> HSV変換
float3 RGBToHSV(float3 Col) {
	float3 HSV;// 出力後のHSV
	float3 RGB;
	float3 Zero = {0,0,0};
	HSV.z= max(Col.x, max(Col.y, Col.z));
	float minValue= min(Col.x, min(Col.y, Col.z));
	HSV.y= ( HSV.z-minValue) / HSV.z;
	float3 V3 = {HSV.z,HSV.z,HSV.z};
	RGB = (HSV.z-minValue!= 0) ? (V3 -Col.xyz) / (HSV.z-minValue) : Zero;
	HSV.x= Col.x== HSV.z? 60 * ( RGB.z-RGB.y) : Zero;
	HSV.x= Col.y== HSV.z? 60 * ( 2 + RGB.x-RGB.z) : HSV.x;
	HSV.x= Col.z== HSV.z? 60 * ( 4 + RGB.y-RGB.x) : HSV.x;
	HSV.x= HSV.x< 0.0 ? HSV.x+ 360 : HSV.x;
	return HSV;
}

// ラプラシアン
void laplacian(float4 in_color: COLOR0, float2 tex: TEXCOORD0, out float4 out_color: COLOR0)
{
	float4 center = tex2D(sampler3, tex);
	out_color = tex2D(sampler3, shiftTexture(tex,  0, -1)) + tex2D(sampler3, shiftTexture(tex, 0,  1))
			  + tex2D(sampler3, shiftTexture(tex, -1,  0)) + tex2D(sampler3, shiftTexture(tex, 1,  0))
			  + tex2D(sampler3, shiftTexture(tex, -1, -1)) + tex2D(sampler3, shiftTexture(tex, 1, -1))
			  + tex2D(sampler3, shiftTexture(tex, -1,  1)) + tex2D(sampler3, shiftTexture(tex, 1,  1))
			  + center * -8;
	float y = 0.298912 * out_color.r + 0.586611 * out_color.g + 0.114478 * out_color.b;
	if (y < subtract) {
		out_color = center;
	} else {
		out_color = float4(0, 0, 0, 1);
	}
}

// 差分検出
void diffx(float4 in_color:COLOR0, float2 tex:TEXCOORD0, out float4 out_color:COLOR0)
{
	//const float4x4 YIQmatrix = {0.299,  0.587,  0.114, 0,
	//							0.596, -0.275, -0.321, 0,
	//							0.212, -0.523,  0.311, 0,
	//							0,0,0,1};
	//float4 yiq1 = mul(YIQmatrix, tex2D(sampler4, tex));
	//float4 yiq2 = mul(YIQmatrix, tex2D(sampler5, tex));
	float d = distance(tex2D(sampler4, tex), tex2D(sampler5, tex));
	if (subtract < d) {
		out_color = 1;
	} else {
		out_color = 0;
	}
}

void blob(float4 in_color: COLOR0, float2 tex: TEXCOORD0, out float4 out_color: COLOR0)
{
	float4 center = tex2D(sampler4, tex);
	float4 p0 = tex2D(sampler4, shiftTexture(tex,  0, -1));
	float4 p1 = tex2D(sampler4, shiftTexture(tex, -1,  0));
	float4 p2 = tex2D(sampler4, shiftTexture(tex,  1,  0));
	float4 p3 = tex2D(sampler4, shiftTexture(tex,  0,  1));
	out_color = center;
	if (out_color.b == 1 && p0.b == 0 && p0.b == p1.b && p1.b == p2.b && p2.b == p3.b) out_color = float4(0, 0, 0, 1);
	if (out_color.b == 0 && p1.b == 1 && p1.b == p2.b) out_color = 1;
	if (out_color.b == 0 && p0.b == 1 && p0.b == p3.b) out_color = 1;
}

void skinDetect(float4 in_color: COLOR0, float2 tex: TEXCOORD0, out float4 out_color: COLOR0)
{
	float4 p = tex2D(sampler3, tex);
	float3 HSV = RGBToHSV(p.xyz);
	if (HSV.x >= 6 && HSV.x <= 38) {
		out_color = float4(1, 0, 0, 1);
	} else {
		out_color = float4(0, 0, 0, 1);
	}
}


// フレーム間を平均してフリッカを低減します
void frameAverage(float4 in_color: COLOR0, float2 tex: TEXCOORD0, out float4 out_color: COLOR0)
{
	float4 p1 = tex2D(sampler1, tex) * 0.3f;
	float4 p2 = tex2D(sampler2, tex) * 0.7f;
	out_color = p1 + p2;
	out_color.a = 1;
}


void maxFilter(float4 in_color: COLOR0, float2 tex: TEXCOORD0, out float4 out_color: COLOR0)
{
	float4 p00 = tex2D(sampler2, shiftTexture(tex, -1, -1));
	float4 p01 = tex2D(sampler2, shiftTexture(tex,  0, -1));
	float4 p02 = tex2D(sampler2, shiftTexture(tex,  1, -1));
	float4 p10 = tex2D(sampler2, shiftTexture(tex, -1,  0));
	float4 p11 = tex2D(sampler2, shiftTexture(tex,  0,  0));
	float4 p12 = tex2D(sampler2, shiftTexture(tex,  1,  0));
	float4 p20 = tex2D(sampler2, shiftTexture(tex, -1,  1));
	float4 p21 = tex2D(sampler2, shiftTexture(tex,  0,  1));
	float4 p22 = tex2D(sampler2, shiftTexture(tex,  1,  1));
	out_color.rgb = max(p00.rgb, p01.rgb);
	out_color.rgb = max(out_color.rgb, p02.rgb);
	out_color.rgb = max(out_color.rgb, p10.rgb);
	out_color.rgb = max(out_color.rgb, p11.rgb);
	out_color.rgb = max(out_color.rgb, p12.rgb);
	out_color.rgb = max(out_color.rgb, p20.rgb);
	out_color.rgb = max(out_color.rgb, p21.rgb);
	out_color.rgb = max(out_color.rgb, p22.rgb);
	out_color.a = 1;
}

void minFilter(float4 in_color: COLOR0, float2 tex: TEXCOORD0, out float4 out_color: COLOR0)
{
	float4 p00 = tex2D(sampler4, shiftTexture(tex, -1, -1));
	float4 p01 = tex2D(sampler4, shiftTexture(tex,  0, -1));
	float4 p02 = tex2D(sampler4, shiftTexture(tex,  1, -1));
	float4 p10 = tex2D(sampler4, shiftTexture(tex, -1,  0));
	float4 p11 = tex2D(sampler4, shiftTexture(tex,  0,  0));
	float4 p12 = tex2D(sampler4, shiftTexture(tex,  1,  0));
	float4 p20 = tex2D(sampler4, shiftTexture(tex, -1,  1));
	float4 p21 = tex2D(sampler4, shiftTexture(tex,  0,  1));
	float4 p22 = tex2D(sampler4, shiftTexture(tex,  1,  1));
	out_color.rgb = min(p00.rgb, p01.rgb);
	out_color.rgb = min(out_color.rgb, p02.rgb);
	out_color.rgb = min(out_color.rgb, p10.rgb);
	out_color.rgb = min(out_color.rgb, p11.rgb);
	out_color.rgb = min(out_color.rgb, p12.rgb);
	out_color.rgb = min(out_color.rgb, p20.rgb);
	out_color.rgb = min(out_color.rgb, p21.rgb);
	out_color.rgb = min(out_color.rgb, p22.rgb);
	out_color.a = 1;
}

// フレーム差分
void diff(float4 in_color: COLOR0, float2 tex: TEXCOORD0, out float4 out_color: COLOR0)
{
	const float4x4 YIQmatrix = {0.299,  0.587,  0.114, 0,
								0.596, -0.275, -0.321, 0,
								0.212, -0.523,  0.311, 0,
								0,0,0,1};
	float4 p1 = mul(YIQmatrix, tex2D(sampler4, tex));
	float4 p2 = mul(YIQmatrix, tex2D(sampler5, tex));
	//float4 p1 = tex2D(sampler2, tex);
	//float4 p2 = tex2D(sampler3, tex);
	if (subtract < distance(p1, p2)) {
		out_color = 1;
	} else {
		out_color = 0;
	}
}

// フレーム差分時間
void diffTime(float4 in_color: COLOR0, float2 tex: TEXCOORD0, out float4 out_color: COLOR0)
{
	float4 p1 = tex2D(sampler4, tex);
	float4 p2 = tex2D(sampler5, tex);
	if (p1.a == 1) {
		out_color.rgb = min(p2.r + 0.01f, 1.0f);
	} else {
		out_color.rgb = max(p2.r - 0.01f, 0);
	}
	out_color.a = 1;
}

// 平均化
void mosaic(float4 in_color: COLOR0, float2 tex: TEXCOORD0, out float4 out_color: COLOR0)
{
	float4 p00 = tex2D(sampler2, shiftTexture(tex,  -1, -1));
	float4 p10 = tex2D(sampler2, shiftTexture(tex,   0, -1));
	float4 p20 = tex2D(sampler2, shiftTexture(tex,   1, -1));
	float4 p01 = tex2D(sampler2, shiftTexture(tex,  -1,  0));
	float4 p11 = tex2D(sampler2, shiftTexture(tex,   0,  0));
	float4 p21 = tex2D(sampler2, shiftTexture(tex,   1,  0));
	float4 p02 = tex2D(sampler2, shiftTexture(tex,  -1,  1));
	float4 p12 = tex2D(sampler2, shiftTexture(tex,   0,  1));
	float4 p22 = tex2D(sampler2, shiftTexture(tex,   1,  1));
	out_color.rgb = max(p00.rgb, p10.rgb);
	out_color.rgb = max(out_color.rgb, p20.rgb);
	out_color.rgb = max(out_color.rgb, p01.rgb);
	out_color.rgb = max(out_color.rgb, p11.rgb);
	out_color.rgb = max(out_color.rgb, p21.rgb);
	out_color.rgb = max(out_color.rgb, p02.rgb);
	out_color.rgb = max(out_color.rgb, p12.rgb);
	out_color.rgb = max(out_color.rgb, p22.rgb);
	out_color.a = 1;
}


void featureExtract1(float4 in_color: COLOR0, float2 tex: TEXCOORD0, out float4 out_color: COLOR0)
{
	float4 center = tex2D(sampler3, tex);
	out_color = center;
	float d0 = distance(center, tex2D(sampler3, shiftTexture(tex,  0, -1)));
	float d1 = distance(center, tex2D(sampler3, shiftTexture(tex, -1,  0)));
	float d2 = distance(center, tex2D(sampler3, shiftTexture(tex,  1,  0)));
	float d3 = distance(center, tex2D(sampler3, shiftTexture(tex,  0,  1)));
	if (subtract < d0 + d1 + d2 + d3) {
		out_color = float4(0, 0, 0, 1);
	} else {
		out_color = center;
	}
}

void featureExtract2(float4 in_color: COLOR0, float2 tex: TEXCOORD0, out float4 out_color: COLOR0)
{
	float4 res = tex2D(sampler4, tex);
	out_color = res;
	if (res.a == 1) {
		float4 center = tex2D(sampler4, tex);
		float4 c = tex2D(sampler4, shiftTexture(tex, -1, -2))
				 + tex2D(sampler4, shiftTexture(tex,  0, -2))
				 + tex2D(sampler4, shiftTexture(tex,  1, -2))
				 + tex2D(sampler4, shiftTexture(tex,  2, -1))
				 + tex2D(sampler4, shiftTexture(tex,  2,  0))
				 + tex2D(sampler4, shiftTexture(tex,  2, -1))
				 + tex2D(sampler4, shiftTexture(tex, -1, -2))
				 + tex2D(sampler4, shiftTexture(tex,  0, -2))
				 + tex2D(sampler4, shiftTexture(tex,  1, -2))
				 + tex2D(sampler4, shiftTexture(tex, -2, -1))
				 + tex2D(sampler4, shiftTexture(tex, -2,  0))
				 + tex2D(sampler4, shiftTexture(tex, -2,  1));
		float d = distance(center, c / 12);
		if (0.500f < d) {
			out_color = float4(0, 0, 0, 1);
		}
	} else {
		out_color = float4(0, 0, 0, 1);
	}
}


technique diffTechnique
{
	pass P0
	{
		pixelShader  = compile ps_2_0 frameAverage();
	}
	pass P1
	{
		pixelShader  = compile ps_2_0 maxFilter();
	}
	pass P2
	{
		pixelShader  = compile ps_2_0 minFilter();
	}
	pass P3
	{
		pixelShader  = compile ps_2_0 diff();
	}
}
