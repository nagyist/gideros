#ifndef SHADERS_H_INCLUDED
#define SHADERS_H_INCLUDED

#include "Matrices.h"
#include <stack>
#include <vector>
#include <string>
#include <map>

class ShaderBufferCache
{
public:
	virtual	~ShaderBufferCache() { }
};

class ShaderTexture;
class ShaderProgram
{
public:
	enum DataType {
		DBYTE, DUBYTE, DSHORT, DUSHORT, DINT, DFLOAT
	};
	enum ConstantType {
		CINT,CFLOAT,CFLOAT2,CFLOAT3,CFLOAT4,CMATRIX,CTEXTURE
	};
	enum ShapeType {
		Point,
		Lines,
		LineLoop,
		Triangles,
		RSV_EX_TriangleFan,
		TriangleStrip
	};
	enum BufferingFlags {
		ForceVBO=1,
	};
    struct DataDesc {
		std::string name;
    	DataType type;
    	unsigned char mult;
    	unsigned char slot;
    	unsigned short offset;
    	unsigned int instances;
    	unsigned int stride;
    };
    enum SystemConstant {
    	SysConst_None=0,
    	SysConst_WorldViewProjectionMatrix,
    	SysConst_Color,
    	SysConst_WorldInverseTransposeMatrix,
    	SysConst_WorldMatrix,
		SysConst_TextureInfo,
		SysConst_ParticleSize,
    	SysConst_WorldInverseTransposeMatrix3,
    	SysConst_ViewMatrix,
		SysConst_Timer,
    	SysConst_ProjectionMatrix,
    	SysConst_ViewProjectionMatrix,
        SysConst_Bounds,
        SysConst_RenderTargetScale,
    };
    enum ShaderFlags {
    	Flag_None=0,
    	Flag_NoDefaultHeader=1,
		Flag_PointShader=2,
		Flag_FromCode=4
    };
    struct ConstantDesc {
    	std::string name;
    	ConstantType type;
    	int mult;
    	SystemConstant sys;
    	bool vertexShader;
    	unsigned short offset;
    	void *_localPtr;
    };
	static ShaderProgram *stdBasic;
	static ShaderProgram *stdColor;
	static ShaderProgram *stdTexture;
	static ShaderProgram *stdTextureAlpha;
	static ShaderProgram *stdTextureColor;
	static ShaderProgram *stdTextureAlphaColor;
	static ShaderProgram *stdParticle;
    static ShaderProgram *stdParticles;
    static ShaderProgram *stdParticles3;
    static ShaderProgram *pathShaderFillC;
	static ShaderProgram *pathShaderStrokeC;
	static ShaderProgram *pathShaderStrokeLC;
	enum StdData {
		DataVertex=0, DataColor=1, DataTexture=2
	};
    virtual void activate()=0;
    virtual void deactivate()=0;
    virtual void setData(int index,DataType type,int mult,const void *ptr,unsigned int count, bool modified, ShaderBufferCache **cache,int stride=0,int offset=0,int bufferingFlags=0)=0;
    virtual void setConstant(int index,ConstantType type, int mult,const void *ptr)=0;
    virtual void drawArrays(ShapeType shape, int first, unsigned int count,unsigned int instances=0)=0;
    virtual void drawElements(ShapeType shape, unsigned int count, DataType type, const void *indices, bool modified, ShaderBufferCache **cache,unsigned int first=0,unsigned int dcount=0,unsigned int instances=0,int bufferingFlags=0)=0;
	virtual void bindTexture(int num,ShaderTexture *texture);
	virtual bool isValid()=0;
    virtual const char *compilationLog()=0;
    void Retain();
    void Release();
    ShaderProgram();
    virtual ~ShaderProgram() { };
    virtual int getSystemConstant(SystemConstant t);
    virtual int getConstantByName(const char *name);
protected:
    int refCount;
    std::vector<ConstantDesc> uniforms;
    int sysconstmask;
    char sysconstidx[16]; //Current sysconst count is 9 (Timer), allocate 16 in case we add more and forget to increase this...
    void shaderInitialized();
    virtual bool updateConstant(int index,ConstantType type, int mult,const void *ptr);
    static void *LoadShaderFile(const char *fname, const char *ext, long *len);
};

class ShaderTexture
{
public:
	virtual ~ShaderTexture() { };
	virtual void setNative(void *externalTexture) { G_UNUSED(externalTexture); };
	virtual void generateMipmap() { };
	virtual void *getNative() { return NULL; };
	enum Format {
		FMT_ALPHA,
		FMT_RGB,
		FMT_RGBA,
		FMT_Y,
		FMT_YA,
		FMT_DEPTH,
        FMT_NATIVE
	};
	enum Packing {
		PK_UBYTE,
		PK_USHORT_565,
		PK_USHORT_4444,
		PK_USHORT_5551,
		PK_FLOAT,
		PK_USHORT,
		PK_UINT
	};
	enum Wrap {
		WRAP_CLAMP,
		WRAP_REPEAT
	};
	enum Filtering {
		FILT_LINEAR,
        FILT_NEAREST,
        FILT_LINEAR_MIPMAP,
	};
	virtual void updateData(ShaderTexture::Format format,ShaderTexture::Packing packing,int width,int height,const void *data,ShaderTexture::Wrap wrap,ShaderTexture::Filtering filtering)=0;
};

class ShaderBuffer
{
    float scaleX,scaleY;
public:
	virtual ~ShaderBuffer() { };
	virtual void prepareDraw()=0;
	virtual void readPixels(int x,int y,int width,int height,ShaderTexture::Format format,ShaderTexture::Packing packing,void *data)=0;
	virtual void unbound()=0;
	virtual void needDepthStencil()=0;
    virtual void setScale(float x,float y) { scaleX=x; scaleY=y; }
    virtual void getScale(float &x,float &y) { x=scaleX; y=scaleY; }
};

class ShaderEngine
{
public:
	enum StencilOp {
		STENCIL_KEEP,
		STENCIL_ZERO,
		STENCIL_REPLACE,
		STENCIL_INCR,
		STENCIL_INCR_WRAP,
		STENCIL_DECR,
		STENCIL_DECR_WRAP,
		STENCIL_INVERT
	};
	enum StencilFunc {
		STENCIL_DISABLE,
		STENCIL_NEVER,
		STENCIL_LESS,
		STENCIL_LEQUAL,
		STENCIL_GREATER,
		STENCIL_GEQUAL,
		STENCIL_EQUAL,
		STENCIL_NOTEQUAL,
		STENCIL_ALWAYS
	};
	enum CullMode {
		CULL_NONE,
		CULL_FRONT,
		CULL_BACK,
	};
	struct DepthStencil {
		bool dTest;
		bool dClear;
        bool dMask;
		StencilFunc sFunc;
		int sRef;
        int sClearValue;
		unsigned int sMask;
		unsigned int sWMask;
		StencilOp sFail;
		StencilOp dFail;
		StencilOp dPass;
		bool sClear;
		CullMode cullMode;
        DepthStencil() : dTest(false), dClear(false), dMask(true), sFunc(STENCIL_DISABLE), sRef(0), sClearValue(0),sMask(0xFF), sWMask(0xFF),
            sFail(STENCIL_KEEP), dFail(STENCIL_KEEP), dPass(STENCIL_KEEP), sClear(false), cullMode(CULL_NONE)
        {};
        bool operator==(const DepthStencil &o) const {
            return dTest==o.dTest && dClear==o.dClear;
        }
        bool operator<(const DepthStencil &o)  const {
#define CHECK(v1,v2) if (v1<v2) return true; else if (v1>v2) return false;
            CHECK(sRef,o.sRef)
            CHECK(sMask,o.sMask)
            CHECK(sWMask,o.sWMask)
            CHECK(sClearValue,o.sClearValue)
            unsigned int d1=(dTest?1:0)|(dClear?2:0)|(sClear?4:0)|(dMask?8:0)
            |(((unsigned int)sFunc)<<4)
            |(((unsigned int)sFail)<<8)
            |(((unsigned int)dFail)<<12)
            |(((unsigned int)dPass)<<16)
            |(((unsigned int)cullMode)<<20);
            unsigned int d2=(o.dTest?1:0)|(o.dClear?2:0)|(o.sClear?4:0)|(o.dMask?8:0)
            |(((unsigned int)o.sFunc)<<4)
            |(((unsigned int)o.sFail)<<8)
            |(((unsigned int)o.dFail)<<12)
            |(((unsigned int)o.dPass)<<16)
            |(((unsigned int)o.cullMode)<<20);
            CHECK(d1,d2)
#undef CHECK
            return false;
        }
	};
protected:
	//CONSTANTS
	Matrix4 oglProjection; //Projection Matrix
	Matrix4 oglView; //View (camera) Matrix
	Matrix4 oglVPProjection; //Viewport projection Matrix
	Matrix4 oglVPProjectionUncorrected;
	Matrix4 oglModel; //Model matrix
	Matrix4 oglCombined; //MVP Matrix
	float constCol[4];
    float screenScaleX,screenScaleY;
	//Scissor structs
	struct Scissor
	{
		Scissor() {}
		Scissor(int x,int y,int w,int h) : x(x), y(y), w(w), h(h) {}
		Scissor(const Scissor &p,int nx,int ny,int nw,int nh) : x(nx), y(ny), w(nw), h(nh)
		{
			int x2=x+w;
			int y2=y+h;
			int px2=p.x+p.w;
			int py2=p.y+p.h;
			if (p.x>x)
				x=p.x;
			if (p.y>y)
				y=p.y;
			if (px2<x2)
				x2=px2;
			if (py2<y2)
				y2=py2;
			w=x2-x;
			h=y2-y;
			if ((w<0)||(h<0))
			{
				w=0;
				h=0;
			}
		}

		int x,y,w,h;
	};
	std::stack<Scissor> scissorStack;
	std::stack<DepthStencil> dsStack;
	DepthStencil dsCurrent;
	static bool ready;
    int vpx,vpy,vpw,vph;
public:
	enum BlendFactor
	{
		NONE,
		ZERO,
		ONE,
		SRC_COLOR,
		ONE_MINUS_SRC_COLOR,
		DST_COLOR,
		ONE_MINUS_DST_COLOR,
		SRC_ALPHA,
		ONE_MINUS_SRC_ALPHA,
		DST_ALPHA,
		ONE_MINUS_DST_ALPHA,
		//CONSTANT_COLOR,
		//ONE_MINUS_CONSTANT_COLOR,
		//CONSTANT_ALPHA,
		//ONE_MINUS_CONSTANT_ALPHA,
		SRC_ALPHA_SATURATE,
	};
	static ShaderEngine *Engine;
	virtual ~ShaderEngine() { };
	virtual void reset(bool reinit=false);
	virtual const char *getVersion()=0;
	virtual const char *getShaderLanguage()=0;
	virtual ShaderTexture::Packing getPreferredPackingForTextureFormat(ShaderTexture::Format format);
	virtual ShaderTexture *createTexture(ShaderTexture::Format format,ShaderTexture::Packing packing,int width,int height,const void *data,ShaderTexture::Wrap wrap,ShaderTexture::Filtering filtering,bool forRT=false)=0;
	virtual ShaderBuffer *createRenderTarget(ShaderTexture *texture,bool forDepth=false)=0;
    virtual ShaderBuffer *setFramebuffer(ShaderBuffer *fbo)=0;
    virtual ShaderBuffer *getFramebuffer()=0;
    virtual ShaderProgram *createShaderProgram(const char *vshader,const char *pshader,int flags,
	                     const ShaderProgram::ConstantDesc *uniforms, const ShaderProgram::DataDesc *attributes)=0;
    virtual void setViewport(int x,int y,int width,int height) { vpx=x; vpy=y; vpw=width; vph=height; };
    void getViewport(int &x,int &y,int &width,int &height) { x=vpx; y=vpy; width=vpw; height=vph; }
	virtual void resizeFramebuffer(int width,int height)=0;
    virtual void setScreenScale(float scaleX,float scaleY) { screenScaleX=scaleX; screenScaleY=scaleY; };
	enum StandardProgram {
		STDP_UNSPECIFIED=0,
		STDP_BASIC=1,
		STDP_COLOR,
		STDP_TEXTURE,
		STDP_TEXTUREALPHA,
		STDP_TEXTURECOLOR,
		STDP_TEXTUREALPHACOLOR,
		STDP_PARTICLE,
		STDP_PARTICLES,
		STDP_PATHFILLCURVE,
		STDP_PATHSTROKECURVE,
		STDP_PATHSTROKELINE
	};
    enum StandardProgramVariant {
        STDPV_TEXTURED=1,
        STDPV_3D
    };
	virtual ShaderProgram *getDefault(StandardProgram id,int variant=0);
	//Matrices
	virtual Matrix4 setFrustum(float l, float r, float b, float t, float n, float f);
	virtual Matrix4 setOrthoFrustum(float l, float r, float b, float t, float n, float f,bool forRenderTarget);
	virtual void setProjection(const Matrix4 p) { oglProjection=p; }
	virtual void setView(const Matrix4 v) { oglView=v; }
	virtual void setViewportProjection(const Matrix4 vp, float width, float height);
	virtual void adjustViewportProjection(Matrix4 &vp, float width, float height) { G_UNUSED(vp); G_UNUSED(width); G_UNUSED(height);};
	virtual void setModel(const Matrix4 m);
    virtual const Matrix4 getModel() { return oglModel; }
	virtual const Matrix4 getView() { return oglView; }
	virtual const Matrix4 getProjection() { return oglProjection; }
	virtual const Matrix4 getViewportProjection() { return oglVPProjectionUncorrected; }
    //Attributes
	virtual void setColor(float r,float g,float b,float a);
	virtual void getColor(float &r,float &g,float &b,float &a);
	virtual void clearColor(float r,float g,float b,float a)=0;
	virtual void bindTexture(int num,ShaderTexture *texture)=0;
	virtual ShaderEngine::DepthStencil pushDepthStencil();
	virtual void popDepthStencil();
	virtual void setDepthStencil(DepthStencil state)=0;
	virtual void setBlendFunc(BlendFactor sfactor, BlendFactor dfactor)=0;
	//Clipping
	virtual void pushClip(float x,float y,float w,float h);
	virtual void popClip();
	virtual bool checkClip(float x,float y,float w,float h);
	virtual void setClip(int x,int y,int w,int h)=0;
    virtual int hasClip();
    //Internal
	virtual void prepareDraw(ShaderProgram *program);
	//Parameters
	virtual void setVBOThreshold(int freeze,int unfreeze) { G_UNUSED(freeze); G_UNUSED(unfreeze); };
	virtual void getProperties(std::map<std::string,std::string> &props) { G_UNUSED(props); };
	//Ready
	static void setReady(bool r) { ready=r; };
	static bool isReady() { return ready; };
};

#endif

