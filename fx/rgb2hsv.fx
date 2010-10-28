/**
 * RGB->HSV変換エフェクト
 */

texture frame1;


sampler sampler1 = sampler_state
{
	Texture = <frame1>;

	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;

	AddressU = Clamp;
	AddressV = Clamp;
};


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
	HSV.x= (HSV.x< 0.0 ? HSV.x+ 360 : HSV.x) / 360.0;
	return HSV;
}


void rgb2hsv(float4 in_color: COLOR0, float2 tex: TEXCOORD0, out float4 out_color: COLOR0)
{
	float4 p = tex2D(sampler1, tex);
	float3 HSV = RGBToHSV(p.xyz);
	out_color = float4(HSV.x, HSV.x, HSV.x, 1);
}



technique convertTechnique
{
	pass P0
	{
		pixelShader  = compile ps_2_0 rgb2hsv();
	}
}
