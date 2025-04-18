D3=D3 or {}

local slang=Shader.getShaderLanguage()

function D3._VLUA_Shader (POSITION,COLOR,TEXCOORD,NORMAL,ANIMIDX,ANIMWEIGHT,INSTMATA,INSTMATB,INSTMATC,INSTMATD,VOXELDATA,VOXELFACE) : Shader
	if OPT_TEXTURED then texCoord=TEXCOORD end
	local pos = hF4(POSITION,1.0)
	local norm = hF4(NORMAL,0.0)
	--[[
	if OPT_INSTANCED_TEST then
		local tc=hF2(((gl_InstanceID*2+1)/InstanceMapWidth)/2.0,0.5);
		local itex=texture2D(g_InstanceMap, tc);
		pos.x-=itex.r*512.0;
		pos.y+=itex.g*512.0;
		pos.z-=itex.b*512.0;
	end
	]]
	if OPT_INSTANCED then
		local imat=hF44(INSTMATA,INSTMATB,INSTMATC,INSTMATD)
		pos=imat*InstanceMatrix*pos
		norm = imat*InstanceMatrix*hF4(norm.xyz, 0.0)
	end
	if OPT_VOXEL then
		pos.x=.5+pos.x/2+VOXELDATA.x
		pos.y=.5+pos.y/2+VOXELDATA.y
		pos.z=.5+pos.z/2+VOXELDATA.z
		local msk=floor(VOXELDATA.w/VOXELFACE)
		local msk2=2*floor(msk/2)
		if msk~=msk2 then pos=hF4(0,0,0,1) end
		vcolor=texture2D(g_ColorMap,hF2(((VOXELDATA.w)%256)/255,0));
	end
	if OPT_BRICKS then
		pos.x=.5+pos.x/2+VOXELDATA.x
		pos.y=.5+pos.y/2+VOXELDATA.y
		pos.z=.5+pos.z/2+VOXELDATA.z
		local msk=floor(VOXELDATA.w/VOXELFACE)
		local msk2=2*floor(msk/2)
		if msk~=msk2 then pos=hF4(0,0,0,1) end
		local bnum=(VOXELDATA.w)%256
		texCoord=hF2(TEXCOORD.x,TEXCOORD.y+bnum/8)
		--vcolor=texture2D(g_ColorMap,hF2(()/255,0));
	end
	if OPT_ANIMATED then
		local nulv=hF4(0,0,0,0)
		local skinning=hF44(nulv,nulv,nulv,nulv)
		skinning+=bones[hI1(ANIMIDX.x)]*ANIMWEIGHT.x;
		skinning+=bones[hI1(ANIMIDX.y)]*ANIMWEIGHT.y;
		skinning+=bones[hI1(ANIMIDX.z)]*ANIMWEIGHT.z;
		skinning+=bones[hI1(ANIMIDX.w)]*ANIMWEIGHT.w;
		local npos = skinning * pos;
		local nnorm = skinning * hF4(norm.xyz, 0.0);
		pos=hF4(npos.xyz,1.0);
		norm=nnorm;
	end
	
	position = (g_MVMatrix*pos).xyz
	normalCoord = normalize((g_NMatrix*hF4(norm.xyz,0)).xyz)
	eyePos=cameraPos
	if OPT_SHADOWS then
		lightSpace = g_LMatrix*hF4(position,1.0);
	end
	if OPT_OCCLUSION then
		occlusionSpace = g_MVPMatrix * pos;
	end
	if OPT_COLORED then
		vcolor=COLOR
	end
	if OPT_FOG then
		eyeDist=length(position.xyz-cameraPos.xyz)
	end
	return g_MVPMatrix * pos;
end

function D3._FLUA_Shader() : Shader
	--mat3 cotangent_frame(vec3 N, vec3 p, vec2 uv)
	function cotangent_frame(N,p,uv)
		--// récupère les vecteurs du triangle composant le pixel
		local dp1 = dFdx( p );
		local dp2 = dFdy( p );
		local duv1 = dFdx( uv );
		local duv2 = dFdy( uv );

		--// résout le système linéaire
		local dp2perp = cross( dp2, N );
		local dp1perp = cross( N, dp1 );
		local T = dp2perp * duv1.x + dp1perp * duv2.x;
		local B = dp2perp * duv1.y + dp1perp * duv2.y;

		--// construit une trame invariante à l'échelle
		local invmax = inversesqrt( max( dot(T,T), dot(B,B) ) );
		return hF33( T * invmax, B * invmax, N );
	end

	--vec3 perturb_normal( vec3 N, vec3 V, vec2 texcoord )
	function perturb_normal( N, V, texcoord )
		--// N, la normale interpolée et
		--// V, le vecteur vue (vertex dirigé vers l'œil)
		local ret=hF3(0,0,0)
		if OPT_NORMMAP then
			local map = hF3(texture2D(g_NormalMap, texcoord ).xyz);
			map = hF3(map * 255./127. - 128./127.)
			local TBN = cotangent_frame(N, -V, texcoord);
			ret=normalize(TBN * map);
		end
		return ret
	end
	function ShadowCalculation(fragPosLightSpace,fragCoord)
		local shadow=0.0		
		if OPT_SHADOWS then
			--// perform perspective divide
			local projCoords = fragPosLightSpace / fragPosLightSpace.w;
			--// transform to [0,1] range
			projCoords = projCoords * 0.5 + 0.5;
			if (projCoords.x<0.0) or (projCoords.y<0.0) or (projCoords.x>=1.0) or (projCoords.y>=1.0) then 
				shadow=1.0
			else
				projCoords.z-=0.001; --BIAS
				--[[ TODO
				#ifdef GLES2
					float shadow=shadow2DEXT(g_ShadowMap, projCoords.xyz);
				#else
				--]]
				local scale=hF3(1/1024.0,1/1024.0,1/1024.0); --shadow map size
				--[[
				//shadow+=shadow2D(g_ShadowMap, projCoords.xyz).r;
				// Poisson sampling
				/*
				shadow+=shadow2D(g_ShadowMap, projCoords.xyz+vec3(-0.94201624, -0.39906216,0.0)*scale).r;
				shadow+=shadow2D(g_ShadowMap, projCoords.xyz+vec3(0.94558609, -0.76890725 ,0.0)*scale).r;
				shadow+=shadow2D(g_ShadowMap, projCoords.xyz+vec3(-0.094184101, -0.92938870,0.0)*scale).r;
				shadow+=shadow2D(g_ShadowMap, projCoords.xyz+vec3(0.34495938, 0.29387760,0.0)*scale).r;
				shadow/=4.0;
				*/
				// 16 sampling
				/*
				float x, y;
				for (y = -1.5; y <= 1.5; y += 1.0)
				  for (x = -1.5; x <= 1.5; x += 1.0)
					shadow+=shadow2D(g_ShadowMap, projCoords.xyz+vec3(x,y,0.0)*scale).r;
				shadow/=16.0;
				*/
				//9 sampling
				/*
				float x, y;
				for (y = -1; y <= 1; y += 1.0)
				  for (x = -1; x <= 1; x += 1.0)
					shadow+=shadow2D(g_ShadowMap, projCoords.xyz+vec3(x,y,0.0)*scale).r;
				shadow/=9.0;
				*/
				]]
				--Dithered
				local offset =  hF3(mod(floor(fragCoord.xy),2.0),0)
				offset.z=0.0
				shadow+=shadow2D(g_ShadowMap, projCoords.xyz+(offset+hF3(-1.5,0.5,0.0))*scale).r
				shadow+=shadow2D(g_ShadowMap, projCoords.xyz+(offset+hF3(0.5,0.5,0.0))*scale).r
				shadow+=shadow2D(g_ShadowMap, projCoords.xyz+(offset+hF3(-1.5,-1.5,0.0))*scale).r
				shadow+=shadow2D(g_ShadowMap, projCoords.xyz+(offset+hF3(0.5,-1.5,0.0))*scale).r
				shadow/=4.0;
				if(projCoords.z >= 0.999) then
					shadow = 1.0
				end
			end
		end
		return shadow
	end
	if not OPT_DEPTHMASK then
		if OPT_OCCLUSION then
			local sp=occlusionSpace/occlusionSpace.w
			sp=sp*0.5+0.5
			local odepth=texture2D(g_OcclusionMap,sp.xy).r
			if odepth<sp.z then discard() end
		end
		local color0 = lF4(g_Color)
		if OPT_COLORED then
			color0=color0*vcolor
		end
		if OPT_VOXEL then
			color0=color0*vcolor
		end
		if OPT_TEXTURED then
			color0 = color0*texture2D(g_Texture,texCoord)
		end
		local color1 = lF3(0.5, 0.5, 0.5)
		local normal = normalize(normalCoord)

		local lightDir = normalize(lightPos.xyz - position.xyz)
		local viewDir = normalize(cameraPos.xyz-position.xyz)
		if OPT_NORMMAP then
			normal=perturb_normal(normal, viewDir, texCoord)
		end

		local diff = max(0.0, dot(normal, lightDir))
		local spec =0.0
		-- calculate shadow
		local shadow=1.0
		if OPT_SHADOWS then
			shadow = ShadowCalculation(lightSpace,FragCoord)
		end
		if (diff>0.0) then
			local nh = max(0.0, dot(reflect(-lightDir,normal),viewDir))
			spec = pow(nh, 32.0)*shadow
		end

		diff=max(diff*shadow,ambient)
		local frag = lF4(color0.rgb * diff + color1 * spec, color0.a)
		
		if OPT_FOG then
			local fogRatio=(eyeDist-fogDistance.x)/fogDistance.y
			fogRatio=clamp(fogRatio,0,1)
			frag=lF4(hF4(frag)*(1-fogRatio)+fogColor*fogRatio)
		end

		if (frag.a==0) then discard() end
		return frag
	else
		return lF4(g_Color)
	end
end
	
D3._FLUA_Shader_FDEF={
	--mat3 cotangent_frame(vec3 N, vec3 p, vec2 uv)
	{name="cotangent_frame", rtype="hF33", acount=3,
		args={
			{name="N", type="hF3"},
			{name="p", type="hF3"},
			{name="uv", type="hF2"},
		},
	},
	--vec3 perturb_normal( vec3 N, vec3 V, vec2 texcoord )
	{name="perturb_normal", rtype="hF3", acount=3,
		args={
			{name="N", type="hF3"},
			{name="V", type="hF3"},
			{name="texcoord", type="hF2"},
		},
	},
	{name="ShadowCalculation", rtype="hF1", acount=2,
		args={
			{name="fragPosLightSpace", type="hF4"},
			{name="fragCoord", type="hF4"},
		},
	},
}



if slang=="glsl" then
D3._V_Shader=
[[
attribute vec4 POSITION;
#ifdef TEXTURED
attribute vec2 TEXCOORD;
#endif
attribute vec3 NORMAL;
#ifdef ANIMATED
attribute vec4 ANIMIDX;
attribute vec4 ANIMWEIGHT;
#endif

uniform mat4 g_MVPMatrix;
uniform mat4 g_MVMatrix;
uniform mat4 g_NMatrix;
uniform mat4 g_LMatrix;

varying highp vec3 position;
#ifdef TEXTURED
varying mediump vec2 texCoord;
#endif
varying mediump vec3 normalCoord;
#ifdef SHADOWS
varying highp vec4 lightSpace;
#endif
#ifdef ANIMATED
uniform mat4 bones[16];
#endif
#ifdef INSTANCED_TEST
uniform highp sampler2D g_InstanceMap;
uniform float InstanceMapWidth;
#endif
#ifdef INSTANCED
uniform mat4 InstanceMatrix;
attribute vec4 INSTMATA;
attribute vec4 INSTMATB;
attribute vec4 INSTMATC;
attribute vec4 INSTMATD;
#endif

void main()
{
#ifdef TEXTURED
	texCoord = TEXCOORD;
#endif
	highp vec4 pos=POSITION;
	mediump vec4 norm=vec4(NORMAL,0.0);
#ifdef INSTANCED_TEST
	vec2 tc=vec2(((gl_InstanceID*2+1)/InstanceMapWidth)/2.0,0.5);
	vec4 itex=texture2D(g_InstanceMap, tc);
	pos.x-=itex.r*512.0;
	pos.y+=itex.g*512.0;
	pos.z-=itex.b*512.0;
#endif
#ifdef INSTANCED
	highp mat4 imat=mat4(INSTMATA,INSTMATB,INSTMATC,INSTMATD);
	pos=imat*InstanceMatrix*pos;
	norm = imat*InstanceMatrix*vec4(norm.xyz, 0.0);
#endif

#ifdef ANIMATED
	mat4 skinning=mat4(0.0);
	skinning+=bones[int(ANIMIDX.x)]*ANIMWEIGHT.x;
	skinning+=bones[int(ANIMIDX.y)]*ANIMWEIGHT.y;
	skinning+=bones[int(ANIMIDX.z)]*ANIMWEIGHT.z;
	skinning+=bones[int(ANIMIDX.w)]*ANIMWEIGHT.w;
    vec4 npos = skinning * pos;
    vec4 nnorm = skinning * vec4(norm.xyz, 0.0);
	pos=vec4(npos.xyz,1.0);
	norm=nnorm;
#endif
	position = (g_MVMatrix*pos).xyz;
	normalCoord = normalize((g_NMatrix*vec4(norm.xyz,0)).xyz);
#ifdef SHADOWS
	lightSpace = g_LMatrix*vec4(position,1.0);
#endif
	gl_Position = g_MVPMatrix * pos;
}
]]

D3._F_Shader=[[
#ifdef GLES2
#extension GL_OES_standard_derivatives : enable
#ifdef SHADOWS
#extension GL_EXT_shadow_samplers : require
#endif
#endif
uniform lowp vec4 g_Color;
uniform highp vec4 lightPos;
uniform lowp float ambient;
uniform highp vec4 cameraPos;
#ifdef TEXTURED
uniform lowp sampler2D g_Texture;
#endif
#ifdef NORMMAP
uniform lowp sampler2D g_NormalMap;
#endif
#ifdef SHADOWS
uniform highp sampler2DShadow g_ShadowMap;
//uniform highp sampler2D g_ShadowMap;
#endif

#ifdef TEXTURED
varying mediump vec2 texCoord;
#endif
varying mediump vec3 normalCoord;
varying highp vec3 position;
#ifdef SHADOWS
varying highp vec4 lightSpace;
#endif

#ifdef GLES2
precision highp float;
#endif

#ifdef NORMMAP
mat3 cotangent_frame(vec3 N, vec3 p, vec2 uv)
{
    // récupère les vecteurs du triangle composant le pixel
    vec3 dp1 = dFdx( p );
    vec3 dp2 = dFdy( p );
    vec2 duv1 = dFdx( uv );
    vec2 duv2 = dFdy( uv );

    // résout le système linéaire
    vec3 dp2perp = cross( dp2, N );
    vec3 dp1perp = cross( N, dp1 );
    vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;

    // construit une trame invariante à l'échelle 
    float invmax = inversesqrt( max( dot(T,T), dot(B,B) ) );
    return mat3( T * invmax, B * invmax, N );
}

vec3 perturb_normal( vec3 N, vec3 V, vec2 texcoord )
{
    // N, la normale interpolée et
    // V, le vecteur vue (vertex dirigé vers l'œil)
    vec3 map = texture2D(g_NormalMap, texcoord ).xyz;
    map = map * 255./127. - 128./127.;
    mat3 TBN = cotangent_frame(N, -V, texcoord);
    return normalize(TBN * map);
}
#endif

#ifdef SHADOWS
float ShadowCalculation(vec4 fragPosLightSpace)
{
    // perform perspective divide
    vec4 projCoords = fragPosLightSpace / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
	if ((projCoords.x<0.0)||(projCoords.y<0.0)||(projCoords.x>=1.0)||(projCoords.y>=1.0))
		return 1.0;
	projCoords.z-=0.001; //BIAS
#ifdef GLES2	
	float shadow=shadow2DEXT(g_ShadowMap, projCoords.xyz); 
#else
	// Single sampling
	float shadow=0.0;
	vec3 scale=vec3(1/1024.0,1/1024.0,1/1024.0); //shadow map size
	//shadow+=shadow2D(g_ShadowMap, projCoords.xyz).r; 	
	// Poisson sampling
	/*
	shadow+=shadow2D(g_ShadowMap, projCoords.xyz+vec3(-0.94201624, -0.39906216,0.0)*scale).r; 	
	shadow+=shadow2D(g_ShadowMap, projCoords.xyz+vec3(0.94558609, -0.76890725 ,0.0)*scale).r; 	
	shadow+=shadow2D(g_ShadowMap, projCoords.xyz+vec3(-0.094184101, -0.92938870,0.0)*scale).r; 	
	shadow+=shadow2D(g_ShadowMap, projCoords.xyz+vec3(0.34495938, 0.29387760,0.0)*scale).r; 
  shadow/=4.0;	
	*/
	// 16 sampling
	/*
	float x, y;
	for (y = -1.5; y <= 1.5; y += 1.0)
	  for (x = -1.5; x <= 1.5; x += 1.0)
		shadow+=shadow2D(g_ShadowMap, projCoords.xyz+vec3(x,y,0.0)*scale).r; 	
	shadow/=16.0;	
	*/
	//9 sampling
	/*
	float x, y;
	for (y = -1; y <= 1; y += 1.0)
	  for (x = -1; x <= 1; x += 1.0)
		shadow+=shadow2D(g_ShadowMap, projCoords.xyz+vec3(x,y,0.0)*scale).r; 	
	shadow/=9.0;	
	*/
	//Ditehered
	vec3 offset =  mod(floor(gl_FragCoord.xyz),2.0);  // mod
	offset.z=0.0;
	shadow+=shadow2D(g_ShadowMap, projCoords.xyz+(offset+vec3(-1.5,0.5,0.0))*scale).r; 	
	shadow+=shadow2D(g_ShadowMap, projCoords.xyz+(offset+vec3(0.5,0.5,0.0))*scale).r; 	
	shadow+=shadow2D(g_ShadowMap, projCoords.xyz+(offset+vec3(-1.5,-1.5,0.0))*scale).r; 	
	shadow+=shadow2D(g_ShadowMap, projCoords.xyz+(offset+vec3(0.5,-1.5,0.0))*scale).r; 
	shadow/=4.0;	
#endif
	if(projCoords.z >= 0.999)
        shadow = 1.0;
	return shadow;
}  
#endif

void main()
{
#ifdef TEXTURED
	lowp vec4 color0 = g_Color*texture2D(g_Texture, texCoord);
#else
	lowp vec4 color0 = g_Color;
#endif
	lowp vec3 color1 = vec3(0.5, 0.5, 0.5);
	highp vec3 normal = normalize(normalCoord);
	
	highp vec3 lightDir = normalize(lightPos.xyz - position.xyz);
	highp vec3 viewDir = normalize(cameraPos.xyz-position.xyz);
#ifdef NORMMAP	
	normal=perturb_normal(normal, viewDir, texCoord);
#endif	
	
	mediump float diff = max(0.0, dot(normal, lightDir));
	mediump float spec =0.0;
    // calculate shadow
	float shadow=1.0;
#ifdef SHADOWS
    shadow = ShadowCalculation(lightSpace);       
#endif	
	if (diff>0.0)
	{
		mediump float nh = max(0.0, dot(reflect(-lightDir,normal),viewDir));
		spec = pow(nh, 32.0)*shadow;
	}

	diff=max(diff*shadow,ambient);
	gl_FragColor = vec4(color0.rgb * diff + color1 * spec, color0.a);
}
]]

elseif slang=="msl" then
D3._V_Shader=
[=[
#include <metal_stdlib>
using namespace metal;

struct InVertex
{
    float3 POSITION0 [[attribute(0)]];
    //float4 vColor [[attribute(1)]];
#ifdef TEXTURED
    float2 TEXCOORD0 [[attribute(2)]];
#endif
    float3 NORMAL0 [[attribute(3)]];
#ifdef ANIMATED
    float4 ANIMIDX [[attribute(4)]];
    float4 ANIMWEIGHT [[attribute(5)]];
#endif
#ifdef INSTANCED
    float4 INSTMAT1 [[attribute(6)]];
    float4 INSTMAT2 [[attribute(7)]];
    float4 INSTMAT3 [[attribute(8)]];
    float4 INSTMAT4 [[attribute(9)]];
#endif
};

struct PVertex
{
    float4 gposition [[position]];
    float3 position [[user(position)]];
    half4 color [[user(color)]];
    float3 normalCoord [[user(normalCoord)]];
#ifdef TEXTURED
    float2 texCoord [[user(texCoord)]];
#endif
#ifdef SHADOWS
    float4 lightSpace [[user(lightSpace)]];
#endif
};


struct Uniforms
{
    float4x4 g_MVPMatrix;
    float4x4 g_MVMatrix;
    float4x4 g_NMatrix;
    float4x4 g_LMatrix;
	float4 g_Color;
	float4 lightPos;
	float4 cameraPos;
	float ambient;
	float4x4 bones[16];
#ifdef INSTANCED_TEST
//uniform highp sampler2D g_InstanceMap;
	float InstanceMapWidth;
#endif
	float4x4 InstanceMatrix;
};


vertex PVertex vmain(InVertex inVertex [[stage_in]],
                      constant Uniforms &u [[buffer(0)]])
{
    PVertex o;
#ifdef TEXTURED
	o.texCoord = inVertex.TEXCOORD0;
#endif
	float4 pos=float4(inVertex.POSITION0,1.0);
	float4 norm=float4(inVertex.NORMAL0,0.0);
#ifdef INSTANCED_TEST
	float2 tc=float2(((gl_InstanceID*2+1)/InstanceMapWidth)/2.0,0.5);
	float4 itex=texture2D(g_InstanceMap, tc);
	pos.x-=itex.r*512.0;
	pos.y+=itex.g*512.0;
	pos.z-=itex.b*512.0;
#endif
#ifdef INSTANCED
	float4x4 imat=float4x4(inVertex.INSTMAT1,inVertex.INSTMAT2,inVertex.INSTMAT3,inVertex.INSTMAT4);
	pos=imat*u.InstanceMatrix*pos;
	norm = imat*u.InstanceMatrix*float4(norm.xyz, 0.0);
#endif

#ifdef ANIMATED
	float4x4 skinning=float4x4(0.0);
	skinning+=u.bones[int(inVertex.ANIMIDX.x)]*inVertex.ANIMWEIGHT.x;
	skinning+=u.bones[int(inVertex.ANIMIDX.y)]*inVertex.ANIMWEIGHT.y;
	skinning+=u.bones[int(inVertex.ANIMIDX.z)]*inVertex.ANIMWEIGHT.z;
	skinning+=u.bones[int(inVertex.ANIMIDX.w)]*inVertex.ANIMWEIGHT.w;
    float4 npos = skinning * pos;
    float4 nnorm = skinning * float4(norm.xyz, 0.0);
	pos=float4(npos.xyz,1.0);
	norm=nnorm;
#endif
	o.position = (u.g_MVMatrix*pos).xyz;
	o.normalCoord = normalize((u.g_NMatrix*float4(norm.xyz,0)).xyz);
#ifdef SHADOWS
	o.lightSpace = u.g_LMatrix*float4(o.position,1.0);
#endif
	o.gposition = u.g_MVPMatrix * pos;	

    return o;
}

]=]

D3._F_Shader=[=[
#include <metal_stdlib>
using namespace metal;

struct PVertex
{
    float4 gposition [[position]];
    float3 position [[user(position)]];
    half4 color [[user(color)]];
    float3 normalCoord [[user(normalCoord)]];
#ifdef TEXTURED
    float2 texCoord [[user(texCoord)]];
#endif
#ifdef SHADOWS
    float4 lightSpace [[user(lightSpace)]];
#endif
};


struct Uniforms
{
    float4x4 g_MVPMatrix;
    float4x4 g_MVMatrix;
    float4x4 g_NMatrix;
    float4x4 g_LMatrix;
	float4 g_Color;
	float4 lightPos;
	float4 cameraPos;
	float ambient;
	float4x4 bones[16];
#ifdef INSTANCED_TEST
//uniform highp sampler2D g_InstanceMap;
	float InstanceMapWidth;
#endif
	float4x4 InstanceMatrix;
};



#ifdef NORMMAP
float3x3 cotangent_frame(float3 N, float3 p, float2 uv)
{
    // récupère les vecteurs du triangle composant le pixel
    float3 dp1 = dFdx( p );
    float3 dp2 = dFdy( p );
    float2 duv1 = dFdx( uv );
    float2 duv2 = dFdy( uv );

    // résout le système linéaire
    float3 dp2perp = cross( dp2, N );
    float3 dp1perp = cross( N, dp1 );
    float3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    float3 B = dp2perp * duv1.y + dp1perp * duv2.y;

    // construit une trame invariante à l'échelle 
    float invmax = inversesqrt( max( dot(T,T), dot(B,B) ) );
    return float3x3( T * invmax, B * invmax, N );
}

float3 perturb_normal( float3 N, float3 V, float2 texcoord,texture2d<half> ntex [[texture(1)]], sampler nsmp [[sampler(1)]] )
{
    // N, la normale interpolée et
    // V, le vecteur vue (vertex dirigé vers l'œil)
    float3 map = ntex.sample(nsmp, texcoord ).xyz;
    map = map * 255./127. - 128./127.;
    float3x3 TBN = cotangent_frame(N, -V, texcoord);
    return normalize(TBN * map);
}
#endif

#ifdef SHADOWS
float ShadowCalculation(float4 fragPosLightSpace,depth2d<float> stex [[texture(2)]], sampler ssmp [[sampler(2)]])
{
    // perform perspective divide
    float3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords.xy = projCoords.xy * 0.5 +0.5;
	if ((projCoords.x<0.0)||(projCoords.y<0.0)||(projCoords.x>=1.0)||(projCoords.y>=1.0))
		return 1.0;
	projCoords.z-=0.005; //BIAS
	float shadow=stex.sample_compare(ssmp, projCoords.xy, projCoords.z);
	if(projCoords.z >= 0.999)
        shadow = 1.0;
	return shadow;
}  
#endif

fragment half4 fmain(
	PVertex vert [[stage_in]],
    constant Uniforms &u [[buffer(0)]]
	#ifdef TEXTURED
	,texture2d<half> ttex [[texture(0)]], sampler tsmp [[sampler(0)]]
	#endif
	#ifdef NORMMAP
	,texture2d<half> ntex [[texture(1)]], sampler nsmp [[sampler(1)]]
	#endif
	#ifdef SHADOWS
	,depth2d<float> stex [[texture(2)]], sampler ssmp [[sampler(2)]]
	#endif
)
{
#ifdef TEXTURED
	half4 color0 = half4(u.g_Color)*ttex.sample(tsmp, vert.texCoord);
#else
	half4 color0 = half4(u.g_Color);
#endif
	half3 color1 = half3(0.5, 0.5, 0.5);
	float3 normal = normalize(vert.normalCoord);
	
	float3 lightDir = normalize(u.lightPos.xyz - vert.position.xyz);
	float3 viewDir = normalize(u.cameraPos.xyz-vert.position.xyz);
#ifdef NORMMAP	
	normal=perturb_normal(normal, viewDir, texCoord,ntex,nsmp);
#endif	
	
	float diff = max(0.0, dot(normal, lightDir));
	float spec =0.0;
    // calculate shadow
	float shadow=1.0;
#ifdef SHADOWS
    shadow = ShadowCalculation(vert.lightSpace,stex,ssmp);       
#endif	
	if (diff>0.0)
	{
		float nh = max(0.0, dot(reflect(-lightDir,normal),viewDir));
		spec = pow(nh, 32.0)*shadow;
	}

	diff=(half)max((diff*shadow),u.ambient);
	return half4(color0.rgb * diff + color1 * spec, color0.a);
}
]=]

elseif slang=="hlsl" then
D3._V_Shader=
[=[

struct PVertex
{
    float3 position : POSITION;
//    half4 color : COLOR;
#ifdef TEXTURED
    float2 texCoord : TEXCOORD;
#endif
    float3 normalCoord : NORMAL;
#ifdef SHADOWS
    float4 lightSpace : LIGHTSPACE;
#endif
    float4 gposition : SV_POSITION;
};


cbuffer cbv : register(b0)
{
    float4x4 g_MVPMatrix;
    float4x4 g_MVMatrix;
    float4x4 g_NMatrix;
    float4x4 g_LMatrix;
	float4x4 bones[16];
#ifdef INSTANCED_TEST
//uniform highp sampler2D g_InstanceMap;
	float InstanceMapWidth;
#endif
	float4x4 InstanceMatrix;
};

PVertex VShader(
float3 POSITION0 : POSITION,
#ifdef TEXTURED
float2 TEXCOORD0 : TEXCOORD,
#endif
#ifdef ANIMATED
float4 ANIMIDX : ANIMIDX,
float4 ANIMWEIGHT : ANIMWEIGHT,
#endif
#ifdef INSTANCED
float4 INSTMAT1 : INSTMATA,
float4 INSTMAT2 : INSTMATB,
float4 INSTMAT3 : INSTMATC,
float4 INSTMAT4 : INSTMATD,
#endif
float3 NORMAL0 : NORMAL)
{
    PVertex o;
#ifdef TEXTURED
	o.texCoord = TEXCOORD0;
#endif
	float4 pos=float4(POSITION0,1.0);
	float4 norm=float4(NORMAL0,0.0);
#ifdef INSTANCED_TEST
	float2 tc=float2(((gl_InstanceID*2+1)/InstanceMapWidth)/2.0,0.5);
	float4 itex=texture2D(g_InstanceMap, tc);
	pos.x-=itex.r*512.0;
	pos.y+=itex.g*512.0;
	pos.z-=itex.b*512.0;
#endif
#ifdef INSTANCED
	float4x4 imat=float4x4(INSTMAT1,INSTMAT2,INSTMAT3,INSTMAT4);
	float4x4 mmat=mul(transpose(imat),InstanceMatrix);
	pos=mul(mmat,pos);
	norm = mul(mmat,float4(norm.xyz, 0.0));
#endif

#ifdef ANIMATED
	float4x4 skinning={{0.0,0.0,0.0,0.0},{0.0,0.0,0.0,0.0},{0.0,0.0,0.0,0.0},{0.0,0.0,0.0,0.0}};
	skinning+=bones[int(ANIMIDX.x)]*ANIMWEIGHT.x;
	skinning+=bones[int(ANIMIDX.y)]*ANIMWEIGHT.y;
	skinning+=bones[int(ANIMIDX.z)]*ANIMWEIGHT.z;
	skinning+=bones[int(ANIMIDX.w)]*ANIMWEIGHT.w;
    float4 npos = mul(skinning,pos);
    float4 nnorm = mul(skinning,float4(norm.xyz, 0.0));
	pos=float4(npos.xyz,1.0);
	norm=nnorm;
#endif
	o.position = mul(g_MVMatrix,pos).xyz;
	o.normalCoord = normalize(mul(g_NMatrix,float4(norm.xyz,0)).xyz);
#ifdef SHADOWS
	o.lightSpace = mul(g_LMatrix,float4(o.position,1.0));
#endif
	o.gposition = mul(g_MVPMatrix,pos);	

    return o;
}
]=]

D3._F_Shader=[=[
cbuffer cbp : register(b1)
{
	float4 g_Color;
	float4 lightPos;
	float4 cameraPos;
	float ambient;
};

#ifdef TEXTURED
Texture2D ttex : register(t0);
SamplerState tsmp : register(s0);
#endif
#ifdef NORMMAP
Texture2D ntex : register(t1);
SamplerState nsmp : register(s1);
#endif
#ifdef SHADOWS
Texture2D stex : register(t2);
SamplerComparisonState ssmp : register(s2);
#endif

#ifdef NORMMAP
float3x3 cotangent_frame(float3 N, float3 p, float2 uv)
{
    // récupère les vecteurs du triangle composant le pixel
    float3 dp1 = dFdx( p );
    float3 dp2 = dFdy( p );
    float2 duv1 = dFdx( uv );
    float2 duv2 = dFdy( uv );

    // résout le système linéaire
    float3 dp2perp = cross( dp2, N );
    float3 dp1perp = cross( N, dp1 );
    float3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    float3 B = dp2perp * duv1.y + dp1perp * duv2.y;

    // construit une trame invariante à l'échelle 
    float invmax = inversesqrt( max( dot(T,T), dot(B,B) ) );
    return float3x3( T * invmax, B * invmax, N );
}

float3 perturb_normal( float3 N, float3 V)
{
    // N, la normale interpolée et
    // V, le vecteur vue (vertex dirigé vers l'œil)
    float3 map = ntex.Sample(nsmp, texcoord ).xyz;
    map = map * 255./127. - 128./127.;
    float3x3 TBN = cotangent_frame(N, -V, texcoord);
    return normalize(TBN * map);
}
#endif

#ifdef SHADOWS
float ShadowCalculation(float4 fragPosLightSpace)
{
    // perform perspective divide
    float3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords.xy = projCoords.xy * 0.5 + 0.5;
	float shadow=1.0;
	if (!((projCoords.x<0.0)||(projCoords.y<0.0)||(projCoords.x>=1.0)||(projCoords.y>=1.0))) {
		projCoords.z-=0.001; //BIAS
		if(projCoords.z >= 0.999)
			shadow = 1.0;
		else
			shadow=stex.SampleCmp(ssmp, projCoords.xy,projCoords.z);
	}
	return shadow;
}  
#endif

float4 PShader(
	float3 position : POSITION,
#ifdef TEXTURED
	float2 texCoord : TEXCOORD,
#endif
	float3 normalCoord : NORMAL,
#ifdef SHADOWS
	float4 lightSpace : LIGHTSPACE,
#endif
	float4 gposition : SV_POSITION
) : SV_TARGET
{
#ifdef TEXTURED
	half4 color0 = half4(g_Color)*ttex.Sample(tsmp, texCoord);
#else
	half4 color0 = half4(g_Color);
#endif
	half3 color1 = half3(0.5, 0.5, 0.5);
	float3 normal = normalize(normalCoord);
	
	float3 lightDir = normalize(lightPos.xyz - position.xyz);
	float3 viewDir = normalize(cameraPos.xyz-position.xyz);
#ifdef NORMMAP	
	normal=perturb_normal(normal, viewDir, texCoord);
#endif	
	
	float diff = max(0.0, dot(normal, lightDir));
	float spec =0.0;
    // calculate shadow
	float shadow=1.0;
#ifdef SHADOWS
    shadow = ShadowCalculation(lightSpace);       
#endif	
	if (diff>0.0)
	{
		float nh = max(0.0, dot(reflect(-lightDir,normal),viewDir));
		spec = pow(nh, 32.0)*shadow;
	}

	diff=(half)max((diff*shadow),ambient);
	return half4(color0.rgb * diff + color1 * spec, color0.a);
}
]=]

end
