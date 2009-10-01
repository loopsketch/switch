//------------------------------------------------
// グローバル変数
//------------------------------------------------
float4x4 	g_wvp;
float4		g_color;

texture stage0;			// テクスチャ: ステージ0
texture stage1;			// テクスチャ: ステージ1
texture stage2;			// テクスチャ: ステージ2
texture stage3;			// テクスチャ: ステージ3

sampler sampler0 = sampler_state
{
	Texture = <stage0>;

	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;

	AddressU = Clamp;
	AddressV = Clamp;
};

sampler sampler1 = sampler_state
{
	Texture = <stage1>;

	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;

	AddressU = Clamp;
	AddressV = Clamp;
};

sampler sampler2 = sampler_state
{
	Texture = <stage2>;

	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;

	AddressU = Clamp;
	AddressV = Clamp;
};


//------------------------------------------------
// 頂点シェーダ
//------------------------------------------------
void BasicVS(float3 in_pos: POSITION, out float4 out_pos: POSITION, out float4 out_color: COLOR0)
{
	// 座標変換
	out_pos = mul(float4(in_pos, 1.0f), g_wvp);
	// 頂点の色の決定
	out_color = g_color;
}

//------------------------------------------------
// ピクセルシェーダ
//------------------------------------------------
void BasicPS(float4 in_color: COLOR0, float2 tex: TEXCOORD0, out float4 out_color: COLOR0)
{
	const float4x4 YUV4HD =    {1,  0,      1.575,  0,
								1, -0.187, -0.4678, 0,
								1,  1.8508, 0,      0,
								0,  0,      0,      1};

	float y = (tex2D(sampler0, float2(tex.x, tex.y)) - 0.0627) * 1.164;
	float u = (tex2D(sampler1, float2(tex.x, tex.y)) - 0.5   ) * 1.138;
	float v = (tex2D(sampler2, float2(tex.x, tex.y)) - 0.5   ) * 1.138;
	// YUV -> RGB
	out_color = mul(YUV4HD, float4(y, u, v, 1));
	out_color.a = in_color.a;
}

//------------------------------------------------
// テクニック宣言
//------------------------------------------------
technique BasicTech
{
    pass P0
    {
//		vertexShader = compile vs_3_0 BasicVS();
		pixelShader  = compile ps_3_0 BasicPS();
	}
}
