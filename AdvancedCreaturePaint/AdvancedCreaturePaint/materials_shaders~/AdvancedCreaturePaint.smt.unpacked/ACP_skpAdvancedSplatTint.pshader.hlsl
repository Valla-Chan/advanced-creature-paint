struct cVertOut
{
	float4 position : POSITION;
	float3 texcoord1 : TEXCOORD1;  // this is the actual texcoord for sampling the texture
};

sampler2D rigblockDiffuseTexture : register(s0);
sampler2D tintMaskTexture : register(s1);
sampler2D baseTexture : register(s2);
sampler2D coatTexture : register(s3);
sampler2D detailTexture : register(s4);
sampler2D identityTexture : register(s5);
sampler2D texturedTexture : register(s6);
extern uniform struct {
	float4 baseColor0;
	float4 baseColor1;
	float4 coatColor0;
	float4 coatColor1;
	float4 detailColor0;
	float4 detailColor1;
	float4 identityColor0;
	float4 identityColor1;
	float4 texturedColor0;
	float4 texturedColor1;
	// r=base, g=identity, b=textured, 1.0 if must apply, 0.0 otherwise
	float4 applyColor;
} customParams;

float3 mixPaintColor(cVertOut In, sampler2D inputTexture, float4 color0, float4 color1)
{
	float4 diffuseTexColor = tex2D(inputTexture, In.texcoord1.xy);
	float3 mixedColor = lerp(diffuseTexColor.rgb, color0.rgb, color0.a);
	return lerp(mixedColor, color1.rgb, color1.a * diffuseTexColor.a);
}

float4 main(cVertOut In) : COLOR
{
	float4 diffuseTexture = tex2D(rigblockDiffuseTexture, In.texcoord1.xy);
	float4 mask = tex2D(tintMaskTexture, In.texcoord1.xy);
	
	float3 baseColor = mixPaintColor(In, baseTexture, customParams.baseColor0, customParams.baseColor1);
	float3 coatColor = mixPaintColor(In, coatTexture, customParams.coatColor0, customParams.coatColor1);
	float3 detailColor = mixPaintColor(In, detailTexture, customParams.detailColor0, customParams.detailColor1);
	float3 identityColor = mixPaintColor(In, detailTexture, customParams.identityColor0, customParams.identityColor1);

	//float baseBrightness = dot(baseColor, float3(0.3, 0.6, 0.1));
	//float rigblockBrightness = dot(clr.rgb, float3(0.3, 0.6, 0.1));
	//float scale = lerp(1.0, rigblockBrightness / (baseBrightness + 0.001), 0.98);
	
	// Spore multiplies each color (scale * baseColor) with a factor that depends on the ratio
	// between the brightness of the diffuse texture and the base color
	// We ignore it and just use 1.0 factor for now
	
	// base, coat, detail are always applied;
	// if paints are not specified, they use a white texture that gets the
	// current creature base/coat/detail color

	float3 colorDiff, colorMix;
	// Apply textured region paint?
	if (customParams.applyColor.b > 0.0)
	{
		float3 texturedColor = mixPaintColor(In, texturedTexture, customParams.texturedColor0, customParams.texturedColor1);
		colorMix = texturedColor;
	}
	else
	{
		colorMix = diffuseTexture.rgb;
	}
	// Apply base color
	colorDiff = baseColor - colorMix;
	colorMix  = mask.x * colorDiff + colorMix;
	// Apply coat color
	colorDiff = coatColor - colorMix;
	colorMix  = mask.y * colorDiff + colorMix;
	// Apply detail color
	colorDiff = detailColor - colorMix;
	colorMix  = mask.z * colorDiff + colorMix;
	// Apply identity color ?
	if (customParams.applyColor.g > 0.0)
	{
		colorDiff = identityColor - colorMix;
		colorMix  = mask.a * colorDiff + colorMix;
	}
	
	float finalAlpha = diffuseTexture.a;
	// Apply base paint?
	if (customParams.applyColor.r > 0.0)
	{
		finalAlpha = 1.0;
		colorMix = lerp(baseColor, colorMix, diffuseTexture.a);
	}
	return float4(colorMix, finalAlpha);
}
