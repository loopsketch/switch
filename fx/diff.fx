/**
 * 背景差分エフェクト
 */

float width;
float height;
float subtract;

texture texture1;
texture texture2;


sampler sampler1 = sampler_state
{
	Texture = <texture1>;

	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;

	AddressU = Clamp;
	AddressV = Clamp;
};

sampler sampler2 = sampler_state
{
	Texture = <texture2>;

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

void skin(float2 tex:TEXCOORD0, out float4 out_color:COLOR0) {
	float4 p = tex2D(sampler1, tex);
	float3 HSV = RGBToHSV(p.xyz);
	if (HSV.x >= 6 && HSV.x <= 38) {
		out_color = 1;
	} else {
		out_color = 0;
	}
	out_color.a = 1;
}

// 差分検出
void diff(float4 in_color: COLOR0, float2 tex:TEXCOORD0, out float4 out_color:COLOR0) {
	float4x4 YIQmatrix =
		{0.299,  0.587,  0.114, 0,
		 0.596, -0.275, -0.321, 0,
		 0.212, -0.523,  0.311, 0,
		 0,0,0,1};
	float4 p1 = tex2D(sampler1, tex);
	float4 p2 = tex2D(sampler2, tex);
	float4 YIQ1 = mul(YIQmatrix, p1);
	float4 YIQ2 = mul(YIQmatrix, p2);

	float dist = distance(YIQ1, YIQ2);
	if (dist < subtract) {
		out_color = 0;
	} else {
		out_color = 1;
	}
	out_color.a = 1;
}

// 平均化
void average(float4 in_color: COLOR0, float2 tex: TEXCOORD0, out float4 out_color: COLOR0)
{
	float4 s[10];
	int i = 0;
	for (int dy = -1; dy <= 1; dy++) {
		for (int dx = -1; dx <= 1; dx++) {
			s[i] = tex2D(sampler1, shiftTexture(tex, dx, dy));
			s[i].a = 1;
			i++;
		}
	}
	float max = 0;
	for (int i = 0; i < 9; i++) {
		for (int j = i + 1; j < 9; j++) {
			if (s[i].a > 0 && distance(s[i].rgb, s[j].rgb) == 0) {
				s[i].a++;
				s[j].a = 0;
			}
		}
		if (max < s[i].a) {
			max = s[i].a;
			out_color = s[9];
		}
	}
}

technique diffTechnique
{
	pass P0
	{
		pixelShader  = compile ps_2_0 average();
	}
}
