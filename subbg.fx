//------------------------------------------------
// グローバル変数
//------------------------------------------------
float texW;
float texH;
float subtract;
float samples;

texture bgTex;
texture currentTex;


sampler background = sampler_state
{
	Texture = <bgTex>;

	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;

	AddressU = Clamp;
	AddressV = Clamp;
};

sampler current = sampler_state
{
	Texture = <currentTex>;

	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;

	AddressU = Mirror;
	AddressV = Mirror;
};



//------------------------------------------------
// 頂点シェーダ
//------------------------------------------------
void BasicVS(float3 in_pos: POSITION, out float4 out_pos: POSITION, out float2 texUV: TEXCOORD0)
{
	in_pos.xy = sign(in_pos.xy);
	out_pos = float4(in_pos.xy, 0.0f, 1.0f);
	texUV = (float2(out_pos.x, -out_pos.y) + 1.0f) / 2.0f;
}

//------------------------------------------------
// ピクセルシェーダ
//------------------------------------------------
void BasicPS(float2 texUV: TEXCOORD0, out float4 out_color: COLOR0)
{
	const float4x4 YIQMatrix = {0.299,  0.587,  0.114, 0,
								0.596, -0.275, -0.321, 0,
								0.212, -0.523,  0.311, 0,
								0,0,0,1};
	float4 currFrameSample = tex2D(current, texUV); // 
	float4 backgroundSample = tex2D(background, texUV); // 
	float4 currFrameSampleYIQ = mul(YIQMatrix, currFrameSample); // convert to YIQ space
	float4 backgroundSampleYIQ = mul(YIQMatrix, backgroundSample);

	float dist = distance(currFrameSampleYIQ, backgroundSampleYIQ); // take liner distance
	if (dist < subtract) {
		out_color = 0;
	} else {
		out_color = currFrameSample;
	}
}

float2 texShift(float2 TexUV, const float shiftX, const float shiftY)
{
	TexUV.x = TexUV.x + 1.0 / texW * shiftX;
	TexUV.y = TexUV.y + 1.0 / texH * shiftY;

	return TexUV;
}

void movingAverage(float4 in_color: COLOR0, float2 texUV: TEXCOORD0, float2 vpos: VPOS, out float4 out_color: COLOR0)
{
	float4 currentSample = tex2D(current, texUV);
	if ((int)vpos.y % 2 == 1)
		currentSample = tex2D(current, texUV);
	else
		currentSample = tex2D(current, texShift(texUV, 0, -1));

	float4 backgroundSample = tex2D(background, texUV); // 
	out_color = backgroundSample + currentSample / samples;
	out_color.a = 1;
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

void diff(float4 in_color: COLOR0, float2 texUV: TEXCOORD0, out float4 out_color: COLOR0)
{
	const float4x4 YIQMatrix = {0.299,  0.587,  0.114, 0,
								0.596, -0.275, -0.321, 0,
								0.212, -0.523,  0.311, 0,
								0,0,0,1};
	float4 currFrameSample = tex2D(current, texUV); // 
	float4 backgroundSample = tex2D(background, texUV); // 
	float4 currFrameSampleYIQ = mul(YIQMatrix, currFrameSample); // convert to YIQ space
	float4 backgroundSampleYIQ = mul(YIQMatrix, backgroundSample);

	float dist = distance(currFrameSampleYIQ, backgroundSampleYIQ); // take liner distance
	if (dist < subtract) {
		out_color = 0;
	} else {
		out_color = 1;
	}
	out_color.a = 1;
}

void blob(float4 in_color: COLOR0, float2 texUV: TEXCOORD0, out float4 out_color: COLOR0)
{
	float4 center = tex2D(current, texUV);
	float4 p0 = tex2D(current, texShift(texUV,  0, -1));
	float4 p1 = tex2D(current, texShift(texUV, -1,  0));
	float4 p2 = tex2D(current, texShift(texUV,  1,  0));
	float4 p3 = tex2D(current, texShift(texUV,  0,  1));
	if (p1.b == 1 && p1.b == p2.b) out_color = 1;
	if (p0.b == 1 && p0.b == p3.b) out_color = 1;
	if (p0.b == 0 && p0.b == p1.b && p1.b == p2.b && p2.b == p3.b) out_color = 0;
}



void skinDetect(float4 in_color: COLOR0, float2 texUV: TEXCOORD0, out float4 out_color: COLOR0)
{
	float4 currentSample = tex2D(current, texUV);
	float3 HSV = RGBToHSV(currentSample.xyz); // HSV化
	if (HSV.x>= 6 && HSV.x<= 38) {
		// スルー
		out_color = 1;
	} else {
		out_color = 0;
	}
	out_color.a = 1;
}

// 平均化
void average(float4 in_color: COLOR0, float2 texUV: TEXCOORD0, out float4 out_color: COLOR0)
{
	out_color = 0;
	for (int i=-1; i<=1; i++) {
		for(int j=-1; j<=1; j++) {
			out_color = out_color + tex2D(current, texShift(texUV, i, j)) / 9;
		}
	}
//	out_color = in_color;
//	out_color = tex2D(current, texUV);
}

void laplacian(float4 in_color: COLOR0, float2 texUV: TEXCOORD0, out float4 out_color: COLOR0)
{
	float4 center = tex2D(current, texUV);
	out_color = tex2D(current, texShift(texUV,  0, -1)) + tex2D(current, texShift(texUV, 0,  1))
			 + tex2D(current, texShift(texUV, -1,  0)) + tex2D(current, texShift(texUV, 1,  0))
			 + tex2D(current, texShift(texUV, -1, -1)) + tex2D(current, texShift(texUV, 1, -1))
			 + tex2D(current, texShift(texUV, -1,  1)) + tex2D(current, texShift(texUV, 1,  1))
			 + center * -8;
	out_color.a = 1;
	float y = 1 - (0.298912 * out_color.r + 0.586611 * out_color.g + 0.114478 * out_color.b);
	if (y >= 0.7) y = 1;
	float3 hsv = RGBToHSV(center.xyz);
	if (hsv.z >= 0.35) {
		out_color = float4(y, y, y, 1);
	} else {
		out_color = float4(hsv.z - 0.1, hsv.z - 0.1, hsv.z, 1);
	}

	float4 backgroundSample = tex2D(background, texUV);
	out_color = backgroundSample + out_color / 5;
	out_color.a = 1;
}


//------------------------------------------------
// テクニック宣言
//------------------------------------------------
technique BasicTech
{
    pass P0
    {
		pixelShader  = compile ps_3_0 movingAverage();
	}

    pass P1
    {
		pixelShader  = compile ps_2_0 diff();
//		pixelShader  = compile ps_2_0 average();
	}

    pass P2
    {
		pixelShader  = compile ps_2_0 blob();
//		pixelShader  = compile ps_2_0 skinDetect();
	}

    pass P3
    {
//		vertexShader = compile vs_1_1 BasicVS();
		pixelShader  = compile ps_2_0 BasicPS();
	}
}
