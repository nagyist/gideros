#include "sprite.h"
#include "ogl.h"
#include <algorithm>
#include <cassert>
#include <stack>
#include "color.h"
#include "blendfunc.h"
#include "stage.h"
#include <application.h>
#include "layoutevent.h"
#include "bitmap.h"

std::set<Sprite*> Sprite::allSprites_;
std::set<Sprite*> Sprite::allSpritesWithListeners_;

#if 0
template<class T> class faststack {
private:
	std::vector<std::vector<T*>> data;
	size_t first_index, last_index;
	size_t lsize = 0;
	const size_t chunk = 1024;
public:
	faststack() {
		first_index = 0;
		last_index = 0;
		data.reserve(16);
	}
	T* pop_front()
	{
		if ((lsize <= 1) && (first_index == last_index)) return nullptr;
		T* r = data[0][first_index++];
		if (first_index == chunk) {
			data.erase(data.begin());
			first_index = 0;
			lsize--;
		}
		return r;
	}
	T* pop()
	{
		if ((lsize <= 1) && (first_index == last_index)) return nullptr;
		if (last_index == 0)
			last_index = chunk - 1;
		else
			last_index--;

		T* r = data[lsize-1][last_index];
		if (last_index == 0) {
			data.erase(data.end()-1);
			lsize--;
		}
		return r;
	}
	void push(T* val)
	{
		if (last_index == 0) {
			lsize++;
			data.resize(lsize);
			data[lsize - 1].resize(chunk);
		}
		data[lsize - 1][last_index++] = val;
		if (last_index == chunk) last_index = 0;
	}
	void push_all(T** vals, size_t count)
	{
		while (count > 0) {
			if (last_index == 0) {
				lsize++;
				data.resize(lsize);
				data[lsize - 1].resize(chunk);
			}
			size_t max = chunk - last_index;
			if (max > count) max = count;

			T** vc = data[lsize - 1].data() + last_index;
			size_t cp = max;
			while (cp--)
				*(vc++) = *(vals++);
			//memcpy(, vals, sizeof(T*) * max);
			last_index += max;
			count -= max;

			if (last_index == chunk) last_index = 0;
		}
	}
};
#else
template<class T> class faststack {
	std::stack<T *> s;
public:
	T* pop()
	{
        if (s.empty()) return nullptr;
		T *r=s.top();
        s.pop();
        return r;
	}
	void push(T* v) { s.push(v); };
	void push_all(T** vals, size_t count)
	{
		while ((count--) > 0)
			s.push(*(vals++));
	}
};
#endif

Sprite::Sprite(Application* application) :
        spriteWithLayoutCount(0), spriteWithEffectCount(0), application_(application), isVisible_(true), parent_(NULL), reqWidth_(0), reqHeight_(0), drawCount_(0) {
    setTypeMap(GREFERENCED_TYPEMAP_SPRITE);
    allSprites_.insert(this);

//	graphicsBases_.push_back(GraphicsBase());
	stopPropagationMask_=0;

	alpha_ = 1;
	colorTransform_ = 0;
	effectsMode_ = CONTINUOUS;
//	graphics_ = 0;

	sfactor_ = (ShaderEngine::BlendFactor) -1;
	dfactor_ = (ShaderEngine::BlendFactor) -1;

	clipy_ = -1;
	clipx_ = -1;
	clipw_ = -1;
	cliph_ = -1;

	stencil_.dTest=false;
    stencilMask_=0;
	layoutState=NULL;
	layoutConstraints=NULL;
	checkClip_=false;
    int meta=INV_TRANSFORM|INV_LAYOUT|INV_CONSTRAINTS|INV_VISIBILITY|INV_CLIP;
    changes_=(ChangeSet)(0x0FF&~meta); //All invalid, except constraints which is never actually revalidated
	hasCustomShader_=false;
    worldAlign_=false;
    shaders_=nullptr;
    viewports=nullptr;
    effectStack_=nullptr;
    skipSet_=nullptr;
    ghosts_=nullptr;
    autoSort_=false;
    for (size_t i=0;i<BOUNDSCACHE_MAX;i++)
        boundsCacheValid[i]=false;
    boundsCacheRef=nullptr;
}

void Sprite::cloneFrom(Sprite *s) {
    effectsMode_=s->effectsMode_;
    hasCustomShader_=s->hasCustomShader_;
    if (s->effectStack_)
        effectStack_=new std::vector<Effect>(*(s->effectStack_));
    if (s->skipSet_)
        skipSet_=new std::vector<char>(*(s->skipSet_));
    checkClip_=s->checkClip_;
    if (s->layoutConstraints)
    {
        layoutConstraints=getLayoutConstraints();
        *layoutConstraints=*s->layoutConstraints;
    }
    if (s->layoutState)
    {
        layoutState=getLayoutState();
        *layoutState=*s->layoutState;
    }
    spriteWithLayoutCount=s->spriteWithLayoutCount;
    spriteWithEffectCount=s->spriteWithEffectCount;
    isVisible_=s->isVisible_;
    localTransform_=s->localTransform_;
    worldTransform_=s->worldTransform_;
    sfactor_=s->sfactor_;
    dfactor_=s->dfactor_;
    for (auto ss=s->children_.cbegin();ss!=s->children_.cend();ss++) {
        Sprite *sc=(*ss)->clone();
        sc->parent_=this;
        children_.push_back(sc);
    }
    colorTransform_=s->colorTransform_;
    if (colorTransform_)
        colorTransform_=new ColorTransform(*(s->colorTransform_));
    alpha_=s->alpha_;
    clipx_=s->clipx_;
    clipy_=s->clipy_;
    clipw_=s->clipw_;
    cliph_=s->cliph_;
    reqWidth_=s->reqWidth_;
    reqHeight_=s->reqHeight_;
    stopPropagationMask_=s->stopPropagationMask_;
    if (s->shaders_)
    {
        shaders_=new std::map<int,struct _ShaderSpec>(*(s->shaders_));
        for (auto ss=shaders_->cbegin();ss!=shaders_->cend();ss++)
            if (ss->second.shader) ss->second.shader->Retain();
    }
    stencil_=s->stencil_;
    stencilMask_=s->stencilMask_;
    worldAlign_=s->worldAlign_;
    autoSort_=s->autoSort_;
    if (s->ghosts_)
    {
        //TODO should we clone ghosts ?
    }
}

Sprite::~Sprite() {
    if (colorTransform_)
        delete colorTransform_;
    //	delete graphics_;

    for (std::size_t i = 0; i < children_.size(); ++i){
        children_[i]->parent_ = 0;
        children_[i]->unref();
    }

	allSprites_.erase(this);
	allSpritesWithListeners_.erase(this);

    if (shaders_!=nullptr)
    {
        std::map<int,struct _ShaderSpec>::iterator it=shaders_->begin();
        while (it!=shaders_->end())
        {
            if (it->second.shader)
                it->second.shader->Release();
            it++;
        }
        delete shaders_;
    }
    if (viewports)
        delete viewports;

    clearLayoutState();
    clearLayoutConstraints();

    if (effectStack_)
        delete effectStack_;
    if (skipSet_)
        delete skipSet_;
    if (ghosts_)
        for (std::size_t i = 0; i < ghosts_->size(); ++i)
            delete (*ghosts_)[i];
}

void Sprite::setupShader(struct _ShaderSpec &spec) {
	if (spec.shader) {
		int sc=spec.shader->getSystemConstant(ShaderProgram::SysConst_Bounds);
		if (sc>=0)
		{
			float bounds[4]={0,0,0,0};
			extraBounds(bounds+0,bounds+1,bounds+2,bounds+3);
			spec.shader->setConstant(sc,ShaderProgram::CFLOAT4,1,bounds);
		}
    }

	for(std::map<std::string,ShaderParam>::iterator it = spec.params.begin(); it != spec.params.end(); ++it) {
			ShaderParam *p=&(it->second);
			int idx=spec.shader->getConstantByName(p->name.c_str());
			if (idx>=0)
				spec.shader->setConstant(idx,p->type,p->mult,&(p->data[0]));
	}
}

void Sprite::setShader(ShaderProgram *shader,ShaderEngine::StandardProgram id,int variant,bool inherit) {
	int sid=(id<<8)|variant;
    RENDER_LOCK();
    if (shaders_==nullptr)
        shaders_=new std::map<int,struct _ShaderSpec>();
    std::map<int,struct _ShaderSpec>::iterator it=shaders_->find(sid);
	if (shader)
		shader->Retain();
    if (it!=shaders_->end()) {
		if (it->second.shader)
			it->second.shader->Release();
		if (shader) {
			it->second.shader=shader;
			it->second.inherit=inherit;
		}
		else {
			it->second.params.clear();
            shaders_->erase(it);
		}
	}
	else if (shader) {
		struct _ShaderSpec sp;
		sp.shader=shader;
		sp.inherit=inherit;
        (*shaders_)[sid]=sp;
	}
    RENDER_UNLOCK();
    invalidate(INV_GRAPHICS|INV_SHADER);
}

bool Sprite::setShaderConstant(ShaderParam p,ShaderEngine::StandardProgram id,int variant)
{
    if (shaders_==nullptr) return false;
	int sid=(id<<8)|variant;
    std::map<int,struct _ShaderSpec>::iterator it=shaders_->find(sid);
    if (it!=shaders_->end()) {
		it->second.params[p.name]=p;
		invalidate(INV_GRAPHICS);
		return true;
	}
	else
		return false;
}

ShaderProgram *Sprite::getShader(ShaderEngine::StandardProgram id,int variant)
{
	if (hasCustomShader_||(changes_&INV_SHADER)) {
		hasCustomShader_=false;
		revalidate(INV_SHADER);
		int sid=(id<<8)|variant;
		std::map<int, struct _ShaderSpec>::iterator it;
        if ((shaders_)&&!shaders_->empty()) {
			hasCustomShader_=true;
            it = shaders_->find(sid);
            if (it != shaders_->end()) {
				setupShader(it->second);
				return it->second.shader;
			}
            it = shaders_->find(0);
            if (it != shaders_->end()) {
				setupShader(it->second);
				return it->second.shader;
			}
		}
		Sprite *p=parent();
		while (p) {
            if ((p->shaders_)&&!p->shaders_->empty()) {
				hasCustomShader_=true;
                it = p->shaders_->find(sid);
                if (it != p->shaders_->end()) {
					if (it->second.inherit) {
						setupShader(it->second);
						return it->second.shader;
					}
				}
			}
			p=p->parent();
		}
	}
	return ShaderEngine::Engine->getDefault(id, variant);
}

void Sprite::doDraw(const CurrentTransform&, float sx, float sy, float ex,
		float ey) {
 G_UNUSED(sx); G_UNUSED(sy); G_UNUSED(ex); G_UNUSED(ey);
}

void setupEffectShader(Bitmap *source,Sprite::Effect &e)
{
	source->setShader(e.shader);
    if (e.shader)
	for(std::map<std::string,Sprite::ShaderParam>::iterator it = e.params.begin(); it != e.params.end(); ++it) {
			Sprite::ShaderParam *p=&(it->second);
			int idx=e.shader->getConstantByName(p->name.c_str());
			if (idx>=0)
				e.shader->setConstant(idx,p->type,p->mult,&(p->data[0]));
	}
    for (size_t t=0;t<e.textures.size();t++)
        if (e.textures[t]) {
            if (t==0)
                source->setTexture(e.textures[t]);
            else
                ShaderEngine::Engine->bindTexture(t,e.textures[t]->data->id());
        }
}

void Sprite::setEffectStack(std::vector<Effect> effects,EffectUpdateMode mode) {
    RENDER_LOCK();
    int diff=0;
    if (effectStack_&&effectStack_->size()>0) {
		diff-=1;
        for (size_t i=0;i<effectStack_->size();i++) {
            if ((*effectStack_)[i].shader)
                (*effectStack_)[i].shader->Release();
            if ((*effectStack_)[i].buffer)
                (*effectStack_)[i].buffer->unref();
            for (size_t t=0;t<(*effectStack_)[i].textures.size();t++)
                if ((*effectStack_)[i].textures[t])
                    (*effectStack_)[i].textures[t]->unref();
		}
	}
    if (effects.empty()) {
        if (effectStack_)
            effectStack_->clear();
    }
    else {
        if (effectStack_==nullptr)
            effectStack_=new std::vector<Effect>(effects);
        else
            effectStack_->assign(effects.cbegin(),effects.cend());
        if (effectStack_->size()>0) {
            diff+=1;
            for (size_t i=0;i<effectStack_->size();i++) {
                if ((*effectStack_)[i].shader)
                    (*effectStack_)[i].shader->Retain();
                if ((*effectStack_)[i].buffer)
                    (*effectStack_)[i].buffer->ref();
                for (size_t t=0;t<(*effectStack_)[i].textures.size();t++)
                    if ((*effectStack_)[i].textures[t])
                        (*effectStack_)[i].textures[t]->ref();
            }
        }
    }
	invalidate(INV_EFFECTS);
	effectsMode_=mode;

	Sprite *p=this;
	while (p) {
        p->spriteWithEffectCount+=diff;
        p=p->parent();
	}
    RENDER_UNLOCK();
}

bool Sprite::setEffectShaderConstant(size_t effectNumber,ShaderParam p)
{
    if ((effectStack_==nullptr)||(effectNumber>=effectStack_->size()))
		return false;
    (*effectStack_)[effectNumber].params[p.name]=p;
	invalidate(INV_EFFECTS);
	return true;
}

void Sprite::updateEffects()
{
	if (effectsMode_!=CONTINUOUS) {
		if (!(changes_&INV_EFFECTS)) return;
	}
    if ((effectStack_)&&!effectStack_->empty()) {
		effectsDrawing_ = true;
		float swidth, sheight;
		float minx, miny, maxx, maxy;

        objectBounds(&minx, &miny, &maxx, &maxy,true);

		if (minx > maxx || miny > maxy)
			return; //Empty Sprite, do nothing
		swidth = maxx;
		sheight = maxy;
        for (size_t i = 0; i < effectStack_->size(); i++) {
            if ((*effectStack_)[i].buffer) {
				if (i == 0) { //First stage, draw the Sprite normally onto the first buffer
					Matrix xform;
                    if ((*effectStack_)[i].autoBuffer) {
						float maxx, maxy;

                        (*effectStack_)[i].autoTransform.transformPoint(0, 0, &maxx, &maxy);
						float tx, ty;
                        (*effectStack_)[i].autoTransform.transformPoint(swidth, 0, &tx, &ty);
						maxx = std::max(maxx, tx); maxy = std::max(maxy, ty);
                        (*effectStack_)[i].autoTransform.transformPoint(0, sheight, &tx, &ty);
						maxx = std::max(maxx, tx); maxy = std::max(maxy, ty);
                        (*effectStack_)[i].autoTransform.transformPoint(swidth, sheight, &tx, &ty);
						maxx = std::max(maxx, tx); maxy = std::max(maxy, ty);
						maxx = std::max(maxx, .0F); maxy = std::max(maxy, .0F);
						swidth = maxx; sheight = maxy;
                        float sx = application_->getLogicalScaleX()/application_->getScale();
                        float sy = application_->getLogicalScaleY()/application_->getScale();
                        swidth *= sx;
                        sheight *= sy;
                        int bw=ceil(swidth);
                        int bh=ceil(sheight);
                        (*effectStack_)[i].buffer->resize(bw, bh, sx, sy);
                       // xform.scale(1.0/sx, 1.0/sy, 1);
					}
                    if ((*effectStack_)[i].clearBuffer)
                        (*effectStack_)[i].buffer->clear(0, 0, 0, 0, -1, -1);
                    xform = xform * (*effectStack_)[i].transform;
					Matrix invL = localTransform_.matrix().inverse();
					xform = xform * invL;

                    (*effectStack_)[i].buffer->draw(this, xform);
				}
                else if ((*effectStack_)[i - 1].buffer) {
                    if ((*effectStack_)[i].autoBuffer) {
						float maxx, maxy;

                        (*effectStack_)[i].autoTransform.transformPoint(0, 0, &maxx, &maxy);
						float tx, ty;
                        (*effectStack_)[i].autoTransform.transformPoint(swidth, 0, &tx, &ty);
						maxx = std::max(maxx, tx); maxy = std::max(maxy, ty);
                        (*effectStack_)[i].autoTransform.transformPoint(0, sheight, &tx, &ty);
						maxx = std::max(maxx, tx); maxy = std::max(maxy, ty);
                        (*effectStack_)[i].autoTransform.transformPoint(swidth, sheight, &tx, &ty);
						maxx = std::max(maxx, tx); maxy = std::max(maxy, ty);
						maxx = std::max(maxx, .0F); maxy = std::max(maxy, .0F);
						swidth = maxx; sheight = maxy;
                        (*effectStack_)[i].buffer->resize(ceilf(swidth), ceilf(sheight), application_->getLogicalScaleX(), application_->getLogicalScaleY());
					}
                    if ((*effectStack_)[i].clearBuffer)
                        (*effectStack_)[i].buffer->clear(0, 0, 0, 0, -1, -1);
                    Bitmap source(application_, (*effectStack_)[i - 1].buffer);
                    setupEffectShader(&source, (*effectStack_)[i - 1]);
                    (*effectStack_)[i].buffer->draw(&source, (*effectStack_)[i].transform);
				}
			}
		}
		revalidate(INV_EFFECTS);
		effectsDrawing_ = false;
	}
}

void Sprite::redrawEffects() {
	invalidate(INV_EFFECTS);
}

void Sprite::updateAllEffects() {
    faststack<Sprite> stack;

    stack.push(this);
    while (true) {
        Sprite *sprite = stack.pop();
        if (sprite == nullptr) break;
        if (sprite->spriteWithEffectCount==0) continue;

        if (sprite->isVisible_ == false) {
            continue;
        }

        stack.push_all(sprite->children_.data(),sprite->children_.size());
        sprite->updateEffects();
    }
}

GridBagLayout *Sprite::getLayoutState()
{
	if (!layoutState) {
		layoutState=new GridBagLayout();
		Sprite *p=this;
		while (p) {
			p->spriteWithLayoutCount++;
			p=p->parent();
		}
	}
    invalidate(INV_LAYOUT);
    return layoutState;
}

void Sprite::clearLayoutState() {
	if (layoutState) {
		delete layoutState;
		Sprite *p=this;
		while (p) {
			p->spriteWithLayoutCount--;
            p=p->parent();
		}
	}
	layoutState=NULL;
}

GridBagConstraints *Sprite::getLayoutConstraints()
{
	if (!layoutConstraints)
		layoutConstraints=new GridBagConstraints();
    invalidate(INV_CONSTRAINTS);
    return layoutConstraints;
}

void Sprite::clearLayoutConstraints()
{
	if (layoutConstraints)
		delete layoutConstraints;
	layoutConstraints=NULL;
}

void Sprite::layoutSizesChanged() {
    if (layoutConstraints) {
        if ((layoutConstraints->prefWidth==-1)
                ||(layoutConstraints->aminWidth==-1)
                ||(layoutConstraints->prefHeight==-1)
                ||(layoutConstraints->aminHeight==-1)
                ) {
            invalidate(INV_CONSTRAINTS);
        }
    }
}


void Sprite::childrenDrawn() {
}

template<typename T>
class GGPool {
public:
	~GGPool() {
		for (size_t i = 0; i < v_.size(); ++i)
			delete v_[i];
	}

	T *create() {
		T *result;

		if (v_.empty()) {
			result = new T;
		} else {
			result = v_.back();
			v_.pop_back();
		}

		return result;

	}

	void destroy(T *t) {
		v_.push_back(t);
	}

private:
	std::vector<T*> v_;
};

void Sprite::draw(const CurrentTransform& transform, float sx, float sy,
		float ex, float ey) {
	static GGPool<faststack<Sprite> > stackPool;
	faststack<Sprite>& stack = *stackPool.create();

	stack.push(this);

	while (true) {
		Sprite* sprite = stack.pop();
		bool pop = false;
		if (sprite == nullptr) {
			pop = true;
			sprite = stack.pop();
			if (sprite == nullptr)
				break;
		}

        bool lastEffect=((!sprite->effectsDrawing_)&&sprite->effectStack_&&(sprite->effectStack_->size()>0));

		if (pop == true) {
            if (sprite->ghosts_)
                for (auto it=sprite->ghosts_->begin();it!=sprite->ghosts_->end();it++)
                {
                    Sprite *model=(*it)->getModel();
                    model->applyGhost(sprite,*it);
                    model->draw(sprite->renderTransform_,sx,sy,ex,ey);
                }
            sprite->drawCount_=1;
            for (size_t i = 0;i< sprite->children_.size();i++)
                sprite->drawCount_+=sprite->children_[i]->drawCount_;
            sprite->childrenDrawn();
            if (sprite->colorTransform_ != 0 || sprite->alpha_ != 1)
				glPopColor();
			if (sprite->sfactor_ != (ShaderEngine::BlendFactor)-1)
				glPopBlendFunc();
            if ((sprite->cliph_ >= 0) && (sprite->clipw_ >= 0) &&(sprite->renderTransform_.type!=Matrix4::FULL))
				ShaderEngine::Engine->popClip();
            if (sprite->stencilMask_)
				ShaderEngine::Engine->popDepthStencil();
			continue;
		}

        sprite->drawCount_=0;
        sprite->revalidate(INV_GRAPHICS); //Revalidate even if not drawn

		if (sprite->isVisible_ == false) {
			continue;
		}

        int clipState=checkClip_?ShaderEngine::Engine->hasClip():-1;
        if (clipState>0)
            continue;

		if ((sprite != this) && (sprite->parent_))
            sprite->renderTransform_ = sprite->parent_->renderTransform_ * sprite->localTransform_.matrix();
		else
            sprite->renderTransform_ = transform * localTransform_.matrix();

        if (sprite->worldAlign_&&(sprite->renderTransform_.type!=Matrix4::FULL)) { //Adjust to integer world coordinates
            float dx,dy;
            sprite->renderTransform_.transformPoint(0,0,&dx,&dy);
            dx=roundf(dx)-dx;
            dy=roundf(dy)-dy;
            if (dx||dy)
                sprite->renderTransform_.translate(dx,dy,0);
        }
        ShaderEngine::Engine->setModel(sprite->renderTransform_);

        if (clipState==0) {
            float minx,miny,maxx,maxy;
            sprite->extraBounds(&minx, &miny, &maxx, &maxy);
            if ((maxx>=minx)&&(maxy>miny)&&ShaderEngine::Engine->checkClip(minx,miny,maxx-minx,maxy-miny))
                    continue;
        }


		if (sprite->colorTransform_ != 0 || sprite->alpha_ != 1) {
			glPushColor();

			float r = 1, g = 1, b = 1, a = 1;

			if (sprite->colorTransform_) {
				r = sprite->colorTransform_->redMultiplier();
				g = sprite->colorTransform_->greenMultiplier();
				b = sprite->colorTransform_->blueMultiplier();
				a = sprite->colorTransform_->alphaMultiplier();
			}

            if (!lastEffect)
                glMultColor(r, g, b, a * sprite->alpha_);
		}

		if (sprite->sfactor_ != (ShaderEngine::BlendFactor)-1) {
			glPushBlendFunc();
			if (!lastEffect)
				glSetBlendFunc(sprite->sfactor_, sprite->dfactor_);
		}

        if ((sprite->cliph_ >= 0) && (sprite->clipw_ >= 0) && (sprite->renderTransform_.type!=Matrix4::FULL))
			ShaderEngine::Engine->pushClip(sprite->clipx_, sprite->clipy_,
					sprite->clipw_, sprite->cliph_);

        if (sprite->stencilMask_)
		{
			ShaderEngine::DepthStencil stencil =
				ShaderEngine::Engine->pushDepthStencil();
            unsigned int sm=sprite->stencilMask_;
            if (sm&STENCILMASK_DTEST)
                stencil.dTest=sprite->stencil_.dTest;
            if (sm&STENCILMASK_SCLEAR)
                stencil.sClear=sprite->stencil_.sClear;
            if (sm&STENCILMASK_DCLEAR)
                stencil.dClear=sprite->stencil_.dClear;
            if (sm&STENCILMASK_DMASK)
                stencil.dMask=sprite->stencil_.dMask;
            if (sm&STENCILMASK_SFAIL)
                stencil.sFail=sprite->stencil_.sFail;
            if (sm&STENCILMASK_DFAIL)
                stencil.dFail=sprite->stencil_.dFail;
            if (sm&STENCILMASK_DPASS)
                stencil.dPass=sprite->stencil_.dPass;
            if (sm&STENCILMASK_SFUNC)
                stencil.sFunc=sprite->stencil_.sFunc;
            if (sm&STENCILMASK_SMASK)
                stencil.sMask=sprite->stencil_.sMask;
            if (sm&STENCILMASK_SWMASK)
                stencil.sWMask=sprite->stencil_.sWMask;
            if (sm&STENCILMASK_SCLEARVALUE)
                stencil.sClearValue=sprite->stencil_.sClearValue;
            if (sm&STENCILMASK_SREF)
                stencil.sRef=sprite->stencil_.sRef;
            if (sm&STENCILMASK_CULLMODE)
                stencil.cullMode=sprite->stencil_.cullMode;
			if (!lastEffect)
				ShaderEngine::Engine->setDepthStencil(stencil);
		}

		if (lastEffect)
		{
            size_t i=sprite->effectStack_->size()-1;
            Bitmap source(application_,(*sprite->effectStack_)[i].buffer);
            setupEffectShader(&source,(*sprite->effectStack_)[i]);
            Matrix4 xform;
            if (sprite->parent_)
                xform=sprite->parent_->renderTransform_;
            else
                xform=transform;
            xform=xform*sprite->localTransform_.matrix();
            xform=xform*(*sprite->effectStack_)[i].postTransform;
            if ((*sprite->effectStack_)[0].autoBuffer) {
                Matrix mscale;
                mscale.scale(1/application_->getLogicalScaleX(),1/application_->getLogicalScaleY(),1);
                xform=xform*mscale;
            }
            ShaderEngine::Engine->setModel(xform);
            ((Sprite *)&source)->doDraw(xform, sx, sy, ex, ey);
		}
		else
            sprite->doDraw(sprite->renderTransform_, sx, sy, ex, ey);

		stack.push(sprite);
		stack.push(nullptr); //End marker

        if (!lastEffect) //Don't draw subs if rendering last effect
        {
            int sc=sprite->skipSet_?sprite->skipSet_->size():0;
            char *sd=sc?sprite->skipSet_->data():NULL;
            int sz = sprite->children_.size();
            if (autoSort_&&(sz>1)) {
                std::vector<Sprite *> cs;
                for (int i = sz - 1; i >= 0; --i)
                {
                    if ((i>=sc)||(!sd[i]))
                        cs.push_back(sprite->children_[i]);
                }
                //Sort children according to distance (nearest last)
                Matrix4 vm=ShaderEngine::Engine->getView();
                Matrix4 tv=vm*sprite->renderTransform_;
                float cx=0,cy=0,cz=0;
                tv.inverseTransformPoint(cx,cy,cz,&cx,&cy,&cz);
                for (auto it:cs) {
                    float dx=it->x()-cx;
                    float dy=it->y()-cy;
                    float dz=it->z()-cz;
                    it->distance_=sqrtf(dx*dx+dy*dy+dz*dz);
                }
                //Sort in reverse order since children are pushd onto a stack
                std::sort(cs.begin(),cs.end(),[](Sprite *a, Sprite *b)
                {
                    return a->distance_ < b->distance_;
                });
                stack.push_all(cs.data(),cs.size());
            }
            else
                for (int i = sz - 1; i >= 0; --i)
                {
                    if ((i>=sc)||(!sd[i]))
                        stack.push(sprite->children_[i]);
                }
        }
	}

	stackPool.destroy(&stack);
}

void Sprite::logicalTransformChanged()
{
    changes_=(ChangeSet)(changes_|INV_EFFECTS);
    for (size_t i = 0; i < children_.size(); ++i)
        children_[i]->logicalTransformChanged();
}

void Sprite::computeLayout(Stage *stage) {
	if (!spriteWithLayoutCount) return;
	static GGPool<faststack<Sprite>> stackPool;
	faststack<Sprite> &stack = *stackPool.create();

    size_t outerLoops=10;
    while ((outerLoops--)&&(stage->needLayout)) {
        stage->needLayout=false;
        stack.push(this);
        while (true) {
            Sprite *sprite = stack.pop();
            if (sprite == nullptr) break;

            if ((sprite->isVisible_ == false)||(!(sprite->spriteWithLayoutCount))) {
                continue;
            }

            if (sprite->layoutState&&sprite->layoutState->dirty)
            {
                int loops=100; //Detect endless loops
                while(sprite->layoutState->dirty&&(loops--))
                {
                    sprite->layoutState->dirty=false;
                    float pwidth,pheight;
                    sprite->getDimensions(pwidth, pheight);
                    sprite->layoutState->ArrangeGrid(sprite,pwidth,pheight);
                }
                if (loops==0) //Gave up, mark as clean to prevent going through endless loop again
                    sprite->layoutState->dirty=false;
            }

            stack.push_all(sprite->children_.data(),sprite->children_.size());
        }
    }

	stackPool.destroy(&stack);
}

bool Sprite::canChildBeAdded(Sprite* sprite, GStatus* status) {
	if (sprite == this) {
		if (status != 0)
			*status = GStatus(2024); // Error #2024: An object cannot be added as a child of itself.

		return false;
	}

	if (sprite->contains(this) == true) {
		if (status != 0)
			*status = GStatus(2150);// Error #2150: An object cannot be added as a child to one of it's children (or children's children, etc.).

		return false;
	}

	return true;
}

bool Sprite::canChildBeAddedAt(Sprite* sprite, int index, GStatus* status) {
	if (canChildBeAdded(sprite, status) == false)
		return false;

	if (index < 0 || index > childCount()) {
		if (status)
			*status = GStatus(2006); // Error #2006: The supplied index is out of bounds.
		return false;
	}

	return true;
}

int Sprite::addChild(Sprite* sprite, GStatus* status) {
    return addChildAt(sprite, childCount(), status);
}

int Sprite::addChildAt(Sprite* sprite, int index, GStatus* status) {
    G_UNUSED(status);
	/* This is not necessary, we are only called by spritebinder and the check is already done there.
	if (canChildBeAddedAt(sprite, index, status) == false)
		return -1;
	*/
    invalidate(INV_GRAPHICS|INV_BOUNDS|INV_LAYOUT);
	Stage* stage1 = sprite->getStage();

	if (stage1)
		stage1->setSpritesWithListenersDirty();

	if (sprite->parent_ == this) {
        auto it=std::find(children_.begin(), children_.end(), sprite);
        size_t cindex=it-children_.begin();
        if ((int)cindex!=index) {
            RENDER_LOCK();
            children_.erase(it);
            if (cindex<(size_t)index) index--;
            children_.insert(children_.begin() + index, sprite);
            RENDER_UNLOCK();
        }
        return index;
	}

	bool connected1 = stage1 != NULL;

	sprite->ref();		// guard

    RENDER_LOCK();
    if (sprite->parent_) {
        if (sprite->spriteWithLayoutCount) {
            Sprite *p=sprite->parent_;
            while (p) {
                p->spriteWithLayoutCount-=sprite->spriteWithLayoutCount;
                p=p->parent();
            }
        }
        if (sprite->spriteWithEffectCount) {
            Sprite *p=sprite->parent_;
            while (p) {
                p->spriteWithEffectCount-=sprite->spriteWithEffectCount;
                p=p->parent();
            }
        }
		SpriteVector& children = sprite->parent_->children_;
		children.erase(std::find(children.begin(), children.end(), sprite));
		sprite->unref();
	}
	sprite->parent_ = this;

	children_.insert(children_.begin() + index, sprite);
    RENDER_UNLOCK();
    sprite->ref();
    if (layoutState&&sprite->layoutConstraints) {
        layoutState->dirty=true;
        layoutState->layoutInfoCache[0].valid=false;
        layoutState->layoutInfoCache[1].valid=false;
    }

    sprite->unref();	// unguard

	Stage *stage2 = sprite->getStage();

    if (stage2) {
		stage2->setSpritesWithListenersDirty();
        if ((sprite->layoutState)||(layoutState&&sprite->layoutConstraints))
            stage2->needLayout=true;
    }

	Sprite *p=this;
	while (p) {
        p->spriteWithLayoutCount+=sprite->spriteWithLayoutCount;
        p->spriteWithEffectCount+=sprite->spriteWithEffectCount;
        p=p->parent();
	}


	bool connected2 = stage2 != NULL;

	if (connected1 && !connected2) {
		Event event(Event::REMOVED_FROM_STAGE);
		sprite->recursiveDispatchEvent(&event, false, false);
	} else if (!connected1 && connected2) {
		Event event(Event::ADDED_TO_STAGE);
		sprite->recursiveDispatchEvent(&event, false, false);
	}
    return index;
}

/**
 Returns the index position of a child Sprite instance.
 */
int Sprite::getChildIndex(Sprite* sprite, GStatus* status) {
	SpriteVector::iterator iter = std::find(children_.begin(), children_.end(),
			sprite);

	if (iter == children_.end()) {
		if (status)
			*status = GStatus(2025); // Error #2025: The supplied Sprite must be a child of the caller.
	}

	return iter - children_.begin();
}

/**
 Changes the position of an existing child in the display object container.
 */
void Sprite::setChildIndex(Sprite* child, int index, GStatus* status) {
	int currentIndex = getChildIndex(child, status);

	if (currentIndex == childCount())
		return;

	if (index < 0 || index > childCount()) {
		if (status)
			*status = GStatus(2006); // Error #2006: The supplied index is out of bounds.
		return;
	}

    RENDER_LOCK();
	children_.erase(children_.begin() + currentIndex);
	children_.insert(children_.begin() + index, child);    
    RENDER_UNLOCK();
}

void Sprite::swapChildren(Sprite* child1, Sprite* child2, GStatus* status) {
	int index1 = getChildIndex(child1, status);
	if (index1 == childCount())
		return;

	int index2 = getChildIndex(child2, status);
	if (index2 == childCount())
		return;

    RENDER_LOCK();
	std::swap(children_[index1], children_[index2]);
    RENDER_UNLOCK();
}

void Sprite::swapChildrenAt(int index1, int index2, GStatus* status) {
	if (index1 < 0 || index1 >= childCount()) {
		if (status)
			*status = GStatus(2006); // Error #2006: The supplied index is out of bounds.
		return;
	}

	if (index2 < 0 || index2 >= childCount()) {
		if (status)
			*status = GStatus(2006); // Error #2006: The supplied index is out of bounds.
		return;
	}

    RENDER_LOCK();
    std::swap(children_[index1], children_[index2]);
    RENDER_UNLOCK();
}

Sprite* Sprite::getChildAt(int index, GStatus* status) const {
	if (index < 0 || index >= childCount()) {
		if (status)
			*status = GStatus(2006); // Error #2006: The supplied index is out of bounds.
		return 0;
	}

	return children_[index];
}

void Sprite::checkInside(float x,float y,bool visible, bool nosubs,std::vector<std::pair<int,Sprite *>> &children, std::vector<Matrix4> &pxform, const Sprite *ref, bool xformValid) const {
    float minx, miny, maxx, maxy;
    int parentidx=children.size();
    size_t sc=skipSet_?skipSet_->size():0;
    char *sd=sc?skipSet_->data():NULL;
    for (size_t i = 0; i < children_.size(); ++i) {
        if ((i>=sc)||(!sd[i])) {
            Sprite *c=children_[i];
            if (c->isVisible_) {
                Matrix transform=pxform.back() * c->localTransform_.matrix();
                bool xformValid2=xformValid;
                c->boundsHelper(transform, &minx, &miny, &maxx, &maxy, pxform, nullptr, visible, nosubs, ref?BOUNDS_REF:BOUNDS_GLOBAL, ref, &xformValid2);
                if (x >= minx && y >= miny && x <= maxx && y <= maxy) {
                    children.push_back(std::pair<int,Sprite *>(parentidx,c));
                    if ((!nosubs)&&(!c->children_.empty())) {
                        pxform.push_back(transform);
                        c->checkInside(x,y,visible,nosubs,children,pxform,ref,xformValid2); //We are recursing so matrix must have been set already
                        pxform.pop_back();
                    }
                }
            }
        }
    }
}

void Sprite::getChildrenAtPoint(float x, float y, const Sprite *ref, bool visible, bool nosubs,std::vector<std::pair<int,Sprite *>> &children) const {
	Matrix transform;
    std::vector<Matrix4> pxform;
	std::stack<const Sprite *> pstack;
    const Sprite *curr = this;
    const Sprite *last = NULL;

    while (curr!=ref) {
		pstack.push(curr);
		last = curr;
		curr = curr->parent_;
	}    
    if (ref&&(!curr)) return;
    if (visible&&(!(ref?curr->isVisible_:last->isStage()))) return;

    while (!pstack.empty()) {
		curr=pstack.top();
		pstack.pop();
        pxform.push_back(transform);
		transform = transform * curr->localTransform_.matrix();
	}

    pxform.push_back(transform);
    checkInside(x,y,visible,nosubs,children,pxform,ref);
}

void Sprite::removeChildAt(int index, GStatus* status) {
	if (index < 0 || index >= childCount()) {
		if (status)
			*status = GStatus(2006); // Error #2006: The supplied index is out of bounds.
		return;
	}
    invalidate(INV_GRAPHICS|INV_BOUNDS|INV_LAYOUT);

	void *pool = application_->createAutounrefPool();

	Sprite* child = children_[index];
    child->invalidate(INV_CONSTRAINTS);

	Stage *stage = child->getStage();

	if (stage)
		stage->setSpritesWithListenersDirty();

	bool connected = stage != NULL;

    RENDER_LOCK();
    child->parent_ = 0;
	children_.erase(children_.begin() + index);
    if (child->spriteWithLayoutCount) {
        Sprite *p=this;
        while (p) {
            p->spriteWithLayoutCount-=child->spriteWithLayoutCount;
            p=p->parent();
        }
    }
    if (child->spriteWithEffectCount) {
        Sprite *p=this;
        while (p) {
            p->spriteWithEffectCount-=child->spriteWithEffectCount;
            p=p->parent();
        }
    }
    RENDER_UNLOCK();

	application_->autounref(child);

	if (connected) {
		Event event(Event::REMOVED_FROM_STAGE);
		child->recursiveDispatchEvent(&event, false, false);
	}

	application_->deleteAutounrefPool(pool);
}

void Sprite::removeChild(Sprite* child, GStatus* status) {
	int index = getChildIndex(child, status);
	if (index == childCount()) {
		if (status)
			*status = GStatus(2025); // Error #2025: The supplied Sprite must be a child of the caller.
		return;
	}

	removeChildAt(index);
}

void Sprite::removeChild(int index, GStatus* status) {
	if (index < 0 || index >= childCount()) {
		if (status)
			*status = GStatus(2025); // Error #2025: The supplied Sprite must be a child of the caller.
		return;
	}

	removeChildAt(index);
}

bool Sprite::contains(Sprite* sprite) const {
	std::stack<const Sprite*> stack;
	stack.push(this);

	while (stack.empty() == false) {
		const Sprite* s = stack.top();
		stack.pop();

		if (s == sprite)
			return true;

		for (int i = 0; i < s->childCount(); ++i)
			stack.push(s->child(i));
	}

	return false;
}

void Sprite::replaceChild(Sprite* oldChild, Sprite* newChild) {
	// TODO: burada addedToStage ile removedFromStage'i halletmek lazim
	SpriteVector::iterator iter = std::find(children_.begin(), children_.end(),
			oldChild);

	assert(iter != children_.end());

	if (oldChild == newChild)
		return;

    RENDER_LOCK();
    oldChild->invalidate(INV_CONSTRAINTS);
    oldChild->parent_ = 0;

	newChild->ref();
	oldChild->unref();
    *iter = newChild;
    newChild->parent_ = this;
    RENDER_UNLOCK();

	invalidate(INV_GRAPHICS|INV_BOUNDS);
}

void Sprite::localToGlobal(float x, float y, float z, float* tx, float* ty, float* tz) const {
	const Sprite* curr = this;

	while (curr) {
		curr->matrix().transformPoint(x, y, z, &x, &y, &z);
		curr = curr->parent_;
	}

	if (tx)
		*tx = x;

	if (ty)
		*ty = y;

	if (tz)
		*tz = z;
}

void Sprite::globalToLocal(float x, float y, float z, float* tx, float* ty, float* tz) const {
	std::stack<const Sprite*> stack;

	const Sprite* curr = this;
	while (curr) {
		stack.push(curr);
		curr = curr->parent_;
	}

	while (stack.empty() == false) {
		stack.top()->matrix().inverseTransformPoint(x, y, z, &x, &y, &z);
		stack.pop();
	}

	if (tx)
		*tx = x;

	if (ty)
		*ty = y;

	if (tz)
		*tz = z;
}

bool Sprite::spriteToLocal(const Sprite *ref,float x, float y, float z, float* tx, float* ty, float* tz) const {
    std::set<const Sprite *> base;
    std::stack<const Sprite*> stack;

    const Sprite* curr = this;
    while (curr) {
        base.insert(curr);
        if (curr==ref) break;
        curr = curr->parent_;
    }
    curr=ref;
    while (curr&&(base.find(curr)==base.end())) {
        curr->matrix().transformPoint(x, y, z, &x, &y, &z);
        curr = curr->parent_;
    }

    if (!curr) return false;

    const Sprite *self = this;
    while (self!=curr) {
        stack.push(self);
        self = self->parent_;
    }

    while (stack.empty() == false) {
        stack.top()->matrix().inverseTransformPoint(x, y, z, &x, &y, &z);
        stack.pop();
    }

    if (tx)
        *tx = x;

    if (ty)
        *ty = y;

    if (tz)
        *tz = z;

    return true;
}

bool Sprite::spriteToLocalMatrix(const Sprite *ref,Matrix &mat) const {
    std::set<const Sprite *> base;
    std::stack<const Sprite*> stack;

    const Sprite* curr = this;
    while (curr) {
        base.insert(curr);
        if (curr==ref) break;
        curr = curr->parent_;
    }
    curr=ref;
    while (curr&&(base.find(curr)==base.end())) {
        stack.push(curr);
        curr = curr->parent_;
    }
    if (!curr) return false;

    const Sprite *self = this;
    int st=0;
    while (self!=curr) {
        stack.push(self);
        st++;
        self = self->parent_;
    }

    if (st) {
        while (st--) {
            mat=mat*stack.top()->matrix();
            stack.pop();
        }
        mat.invert();
    }

    while (stack.empty() == false) {
        mat=mat*stack.top()->matrix();
        stack.pop();
    }

    return true;
}

void Sprite::objectBounds(float* minx, float* miny, float* maxx, float* maxy,
        bool visible) {
    std::vector<Matrix4> pxform;
    boundsHelper(Matrix(), minx, miny, maxx, maxy, pxform, nullptr, visible, false, BOUNDS_OBJECT);
}

void Sprite::localBounds(float* minx, float* miny, float* maxx, float* maxy,
        bool visible) {
    std::vector<Matrix4> pxform;
    boundsHelper(localTransform_.matrix(), minx, miny, maxx, maxy, pxform, nullptr,
            visible, false, BOUNDS_LOCAL);
}

float Sprite::width() {
	float minx, maxx;
	localBounds(&minx, 0, &maxx, 0);

	if (minx > maxx)
		return 0;

	return maxx - minx;
}

float Sprite::height() {
	float miny, maxy;
	localBounds(0, &miny, 0, &maxy);

	if (miny > maxy)
		return 0;

	return maxy - miny;
}

void Sprite::getDimensions(float& w,float &h)
{
	float minx,miny,maxx,maxy;
    extraBounds(&minx, &miny, &maxx, &maxy);
    w=(maxx>=minx)?maxx-minx:0;
    h=(maxy>=miny)?maxy-miny:0;
    if (w<reqWidth_) w=reqWidth_;
    if (h<reqHeight_) h=reqHeight_;
}

void Sprite::setMatrix(const float *m)
{
    localTransform_.setMatrix(m);
    invalidate(INV_TRANSFORM);
}

bool Sprite::hitTestPoint(float x, float y, bool visible,const Sprite *ref) {
	Matrix transform;
    std::vector<Matrix4> pxform;
	std::stack<const Sprite *> pstack;    
    std::set<const Sprite *> base;
    std::stack<const Sprite*> stack;

    if (ref) {
        //If ref is present, collect all possible parents from ref to self or stage
        const Sprite* curr = ref;
        while (curr) {
            base.insert(curr);
            if (curr==this) break;
            curr = curr->parent_;
        }
    }

    //Locate common root chain between (ref or stage) and self
	const Sprite *curr = this;    
    const Sprite *last=NULL;
    while (curr&&((!ref)||(base.find(curr)==base.end()))) {
        if (visible&&(!curr->isVisible_)) return false;
        pstack.push(curr);
		last=curr;
		curr = curr->parent_;
	}
    if (ref) last=curr;
    if (!last) return false;
    if (visible&&(!(ref?(last->isVisible_):(last->isStage())))) return false;

    if (ref) {
        //Transform ref point to common root / stage
        curr=ref;
        while (curr!=last) {
            curr->matrix().transformPoint(x, y, &x, &y);
            curr = curr->parent_;
        }
    }

    //Compute transform matrix from self to common root/ stage
	while (!pstack.empty()) {
		curr=pstack.top();
		pstack.pop();
        pxform.push_back(transform);
		transform = transform * curr->localTransform_.matrix();
	}

	float tx, ty;
	tx = x;
	ty = y;

    /*If transformation is affine and contains rotation, compute in local sprite bounds to fix https://github.com/gideros/gideros/issues/170
    Rationale for this is that the issue only occurs for rotated shapes (that is no longer axis aligned), and would be miss cache optimizations if used for general cases.
    */
    const float *tm=transform.data();
    int tt=transform.type;
    if (((tt==Matrix::M2D)||(tt==Matrix::M3D))&& //Affine (exclude translation only)
        (tm[1]||tm[2]||tm[4]||tm[6]||tm[8]||tm[9])) { //has rotation
        transform.inverseTransformPoint(tx,ty,&tx,&ty);
        ref=this; //We still use the REF code path, but maybe we should use BOUNDS_OBJECT below
        transform.identity();
        pxform.clear();
    }

	float minx, miny, maxx, maxy;
    boundsHelper(transform, &minx, &miny, &maxx, &maxy, pxform, nullptr, visible, false, ref?BOUNDS_REF:BOUNDS_GLOBAL, ref);

	return (tx >= minx && ty >= miny && tx <= maxx && ty <= maxy);
}

Stage *Sprite::getStage() const {
	const Sprite* curr = this;

	while (curr != NULL) {
		if (curr->isStage() == true)
			return static_cast<Stage*>(const_cast<Sprite*>(curr));

		curr = curr->parent();
	}

	return NULL;
}

void Sprite::recursiveDispatchEvent(Event* event, bool canBeStopped,
		bool reverse) {
	void *pool = application_->createAutounrefPool();

	std::vector<Sprite*> sprites;// NOTE: bunu static yapma. recursiveDispatchEvent icindeyken recursiveDispatchEvent cagirilabiliyor

	std::stack<Sprite*> stack;

	stack.push(this);

	while (stack.empty() == false) {
		Sprite* sprite = stack.top();
		stack.pop();

		sprites.push_back(sprite);

		for (int i = sprite->childCount() - 1; i >= 0; --i)
			stack.push(sprite->child(i));
	}

	if (reverse == true)
		std::reverse(sprites.begin(), sprites.end());

	for (std::size_t i = 0; i < sprites.size(); ++i) {
		sprites[i]->ref();
		application_->autounref(sprites[i]);
	}

	for (std::size_t i = 0; i < sprites.size(); ++i) {
		if (canBeStopped == false || event->propagationStopped() == false)
			sprites[i]->dispatchEvent(event);
		else
			break;
	}

	application_->deleteAutounrefPool(pool);
}

float Sprite::alpha() const {
	return alpha_;
}

void Sprite::invalidate(int changes) {

	if (changes&(INV_VISIBILITY))
        changes|=INV_LAYOUT|INV_CONSTRAINTS;

	if (changes&(INV_CLIP|INV_TRANSFORM|INV_VISIBILITY))
		changes|=INV_BOUNDS;

    if (changes&(INV_BOUNDS))
        changes|=INV_GRAPHICS;

    int meta=INV_TRANSFORM|INV_LAYOUT|INV_CONSTRAINTS|INV_VISIBILITY|INV_CLIP;

    int downchanges=changes&(INV_TRANSFORM|INV_BOUNDS|INV_SHADER)&~meta; //Bound, transform and shader changes impact children
    if (downchanges&&children_.size()) {
		faststack<Sprite> stack;
		stack.push_all(children_.data(),children_.size());
		while (true) {
			Sprite *h=stack.pop();
			if (h==nullptr) break;
            if ((h->changes_&downchanges)!=downchanges) {
                h->changes_=(ChangeSet)(h->changes_|downchanges);
                stack.push_all(h->children_.data(),h->children_.size());
            }
		}
	}

    int newChanges=changes&(~changes_);

    if (newChanges&(INV_GRAPHICS)) {
		changes|=INV_EFFECTS;
        if (viewports)
        for (auto it=viewports->begin();it!=viewports->end();it++)
            (*it)->invalidate(INV_GRAPHICS);
    }

    changes_=(ChangeSet)(changes_|(changes&(~meta)));

	changes=(ChangeSet)(changes&~(INV_VISIBILITY|INV_CLIP|INV_TRANSFORM|INV_GRAPHICS|INV_SHADER));
    if (changes&(INV_CONSTRAINTS))
        changes|=INV_LAYOUT;

    bool globalLayout=(changes&(INV_LAYOUT|INV_CONSTRAINTS));

	//Propagate to parents
    Sprite *h=parent_;
	while (changes&&h) {
        if (h->layoutState)
        {
            if (changes&(INV_LAYOUT|INV_CONSTRAINTS))
            {
                if (h->layoutState->optimizing) {
                    changes=(ChangeSet)(changes&(~(INV_LAYOUT|INV_CONSTRAINTS)));
                    globalLayout=false;
                }
                else
                    h->layoutState->dirty=true;
            }
            if (changes&INV_CONSTRAINTS) {
                h->layoutState->layoutInfoCache[0].valid=false;
                h->layoutState->layoutInfoCache[1].valid=false;
            }
        }
		else if (!(h->layoutConstraints&&h->layoutConstraints->group))
            changes=(ChangeSet)(changes&(~(INV_LAYOUT|INV_CONSTRAINTS)));

        h->changes_=(ChangeSet)(h->changes_|(changes&~meta));
		h=h->parent_;
	}

    if (globalLayout) {
        Stage *stage=getStage();
        if (stage)
            stage->needLayout=true;
    }
}

struct ClipRect {
	float xmin;
	float xmax;
	float ymin;
	float ymax;
};

Sprite::BoundsCacheSlot cacheSlot(Sprite::BoundsMode mode,bool visible, bool noSubs)
{
    switch (mode) {
    case Sprite::BOUNDS_OBJECT:
        if (noSubs) return Sprite::BOUNDSCACHE_MAX;
        return visible?Sprite::BOUNDSCACHE_OBJECT_V:Sprite::BOUNDSCACHE_OBJECT;
    case Sprite::BOUNDS_LOCAL:
        if (noSubs) return Sprite::BOUNDSCACHE_MAX;
        return visible?Sprite::BOUNDSCACHE_LOCAL_V:Sprite::BOUNDSCACHE_LOCAL;
    case Sprite::BOUNDS_GLOBAL:
        if (noSubs)
            return visible?Sprite::BOUNDSCACHE_GLOBAL_VS:Sprite::BOUNDSCACHE_GLOBAL_S;
        return visible?Sprite::BOUNDSCACHE_GLOBAL_V:Sprite::BOUNDSCACHE_GLOBAL;
    case Sprite::BOUNDS_REF:
        if (noSubs)
            return visible?Sprite::BOUNDSCACHE_REF_VS:Sprite::BOUNDSCACHE_REF_S;
        return visible?Sprite::BOUNDSCACHE_REF_V:Sprite::BOUNDSCACHE_REF;
    default:
        return Sprite::BOUNDSCACHE_MAX;
    }
}

void Sprite::boundsHelper(const Matrix4& transform, float* minx, float* miny,
        float* maxx, float* maxy, std::vector<Matrix> &parentXform,ClipRect *parentClip,
        bool visible, bool nosubs, BoundsMode mode, const Sprite *ref, bool *xformValid) {
    // Clear hierarchy caches when needed
    Sprite *hrun=this;
    if (changes_&INV_BOUNDS)
    while (hrun) {
        for (size_t i=0;i<BOUNDSCACHE_MAX;i++)
            hrun->boundsCacheValid[i]=false;
        hrun->revalidate(INV_BOUNDS);
        hrun=hrun->parent_;
    }

    //Handle local cache
    BoundsCacheSlot cacheMode=cacheSlot(mode,visible,nosubs);
    if ((cacheMode!=BOUNDSCACHE_MAX)&&boundsCacheValid[cacheMode])
    {
        if  ((mode==BOUNDS_GLOBAL)||
             ((mode==BOUNDS_REF)&&(boundsCacheRef==ref))||
             (parentClip==nullptr))  {
                if (minx)
                    *minx=boundsCache[cacheMode].minx;
                if (miny)
                    *miny=boundsCache[cacheMode].miny;
                if (maxx)
                    *maxx=boundsCache[cacheMode].maxx;
                if (maxy)
                    *maxy=boundsCache[cacheMode].maxy;
                return;
        }
    }

#if 1
    //Validate transform
    if (((!visible) || isVisible_)&&(!(xformValid&&(*xformValid)))) {
        this->worldTransform_ = transform;
        if (!nosubs) {
			faststack<Sprite> stack;
			stack.push_all((Sprite **) (children_.data()), children_.size());

			while (true) {
				Sprite *sprite = stack.pop();
				if (sprite == nullptr) break;

                if ((!visible) || sprite->isVisible_) {
                    sprite->worldTransform_ = sprite->parent_->worldTransform_
                            * sprite->localTransform_.matrix();

                    if (!visible)
                        stack.push_all(sprite->children_.data(),sprite->children_.size());
                    else {
                        int sc=sprite->skipSet_?sprite->skipSet_->size():0;
                        char *sd=sc?sprite->skipSet_->data():NULL;
						int sz = sprite->children_.size();
						for (int i = 0; i <sz; i++)
						{
							if ((i>=sc)||(!sd[i]))
								stack.push(sprite->children_[i]);
						}
                    }
                }
			}
            if (xformValid) *xformValid=true;
		}
	}

    ClipRect clip;
    if (visible) //Check parent hierarchy for clipping/invisibility
    {
        if (parentClip)
            clip=*parentClip;
        else {
            float gminx = -1e30, gminy = -1e30, gmaxx = 1e30, gmaxy = 1e30;
            const Sprite *sprite = this;
            Matrix4 t = transform;
            size_t pLevel= parentXform.size();
            while (sprite->parent_) {
                if (pLevel==0)
                    t = t * sprite->localTransform_.matrix().inverse();
                else
                    t = parentXform.at(--pLevel);
                sprite = sprite->parent_;
                if (visible && (sprite->clipw_ >= 0) && (sprite->cliph_ >= 0)) {
                    float x[4], y[4];

                    t.transformPoint(sprite->clipx_, sprite->clipy_, &x[0],
                            &y[0]);
                    t.transformPoint(sprite->clipx_ + sprite->clipw_ - 1,
                            sprite->clipy_, &x[1], &y[1]);
                    t.transformPoint(sprite->clipx_ + sprite->clipw_ - 1,
                            sprite->clipy_ + sprite->cliph_ - 1, &x[2], &y[2]);
                    t.transformPoint(sprite->clipx_,
                            sprite->clipy_ + sprite->cliph_ - 1, &x[3], &y[3]);

                    float cminx = 1e30, cminy = 1e30, cmaxx = -1e30, cmaxy =
                            -1e30;
                    for (int i = 0; i < 4; ++i) {
                        cminx = std::min(cminx, x[i]);
                        cminy = std::min(cminy, y[i]);
                        cmaxx = std::max(cmaxx, x[i]);
                        cmaxy = std::max(cmaxy, y[i]);
                    }
                    gminx = std::max(gminx, cminx);
                    gminy = std::max(gminy, cminy);
                    gmaxx = std::min(gmaxx, cmaxx);
                    gmaxy = std::min(gmaxy, cmaxy);
                }
            }
            clip.xmin=gminx;
            clip.xmax=gmaxx;
            clip.ymin=gminy;
            clip.ymax=gmaxy;
        }
    }

    //Compute bounds
	{
		float gminx = 1e30, gminy = 1e30, gmaxx = -1e30, gmaxy = -1e30;

		//Gather children bounds
        if (visible&&isVisible_)
        {

            if ((clipw_ >= 0) && (cliph_ >= 0)) {
                float x[4], y[4];
                //Transform clip coordinates
                worldTransform_.transformPoint(clipx_, clipy_, &x[0],
                        &y[0]);
                worldTransform_.transformPoint(clipx_ + clipw_ - 1,
                        clipy_, &x[1], &y[1]);
                worldTransform_.transformPoint(clipx_ + clipw_ - 1,
                        clipy_ + cliph_ - 1, &x[2], &y[2]);
                worldTransform_.transformPoint(clipx_,
                        clipy_ + cliph_ - 1, &x[3], &y[3]);

                float cminx = 1e30, cminy = 1e30, cmaxx = -1e30, cmaxy =
                        -1e30;
                for (int i = 0; i < 4; ++i) {
                    cminx = std::min(cminx, x[i]);
                    cminy = std::min(cminy, y[i]);
                    cmaxx = std::max(cmaxx, x[i]);
                    cmaxy = std::max(cmaxy, y[i]);
                }

                clip.xmin = std::max(clip.xmin, cminx);
                clip.ymin = std::max(clip.ymin, cminy);
                clip.xmax = std::min(clip.xmax,	cmaxx);
                clip.ymax = std::min(clip.ymax,	cmaxy);
            }
        }

        float eminx, eminy, emaxx, emaxy;
        extraBounds(&eminx, &eminy, &emaxx, &emaxy);

        if (eminx <= emaxx && eminy <= emaxy) {
            float x[4], y[4];

            float lgminx = 1e30, lgminy = 1e30, lgmaxx = -1e30, lgmaxy = -1e30;

            worldTransform_.transformPoint(eminx, eminy, &x[0],
                    &y[0]);
            worldTransform_.transformPoint(emaxx, eminy, &x[1],
                    &y[1]);
            worldTransform_.transformPoint(emaxx, emaxy, &x[2],
                    &y[2]);
            worldTransform_.transformPoint(eminx, emaxy, &x[3],
                    &y[3]);

            for (int i = 0; i < 4; ++i) {
                lgminx = std::min(lgminx, x[i]);
                lgminy = std::min(lgminy, y[i]);
                lgmaxx = std::max(lgmaxx, x[i]);
                lgmaxy = std::max(lgmaxy, y[i]);
            }


            //Clip
            if (visible)
            {
                lgminx = std::max(lgminx, clip.xmin);
                lgminy = std::max(lgminy, clip.ymin);
                lgmaxx = std::min(lgmaxx, clip.xmax);
                lgmaxy = std::min(lgmaxy, clip.ymax);
            }

            //Contribute to global bounds
            gminx = lgminx;
            gminy = lgminy;
            gmaxx = lgmaxx;
            gmaxy = lgmaxy;
        }

        if ((!nosubs)&&((!visible)||isVisible_)) {
            int sc=skipSet_?skipSet_->size():0;
            char *sd=sc?skipSet_->data():NULL;
            int sz = children_.size();
            for (int i = 0; i <sz; i++)
            {
                bool xformvalid_=true; //computed world transform is valid (and transform parameter isn't!)
                if ((!visible)||(((i>=sc)||(!sd[i]))&&children_[i]->isVisible_)) {
                    float lgminx = 1e30, lgminy = 1e30, lgmaxx = -1e30, lgmaxy = -1e30;
                    children_[i]->boundsHelper(transform, &lgminx, &lgminy,
                            &lgmaxx, &lgmaxy, parentXform,&clip,
                            visible, nosubs,mode,ref,&xformvalid_);
                    //Contribute to global bounds
                    gminx = std::min(lgminx, gminx);
                    gminy = std::min(lgminy, gminy);
                    gmaxx = std::max(lgmaxx, gmaxx);
                    gmaxy = std::max(lgmaxy, gmaxy);
                }
            }
		}

        //Whatever the sprite is, global bounds are always the same
        //Local and object bounds however are only valid at the sprite level
        if ((cacheMode!=BOUNDSCACHE_MAX)&&
                ((mode==BOUNDS_GLOBAL)||(mode==BOUNDS_REF)||(parentClip==nullptr))) {
            boundsCache[cacheMode].minx=gminx;
            boundsCache[cacheMode].miny=gminy;
            boundsCache[cacheMode].maxx=gmaxx;
            boundsCache[cacheMode].maxy=gmaxy;            
            boundsCacheValid[cacheMode]=true;
            if (mode==BOUNDS_REF)
                boundsCacheRef=ref;
        }

		if (minx)
			*minx = gminx;
		if (miny)
			*miny = gminy;
		if (maxx)
			*maxx = gmaxx;
		if (maxy)
			*maxy = gmaxy;
	}
#else
    //Validate transform
    if ((!visible) || isVisible_) {
        this->worldTransform_ = transform;
        if (!(nosubs||(xformValid&&(*xformValid)))) {
            faststack<Sprite> stack;
            stack.push_all((Sprite **) (children_.data()), children_.size());

            while (true) {
                Sprite *sprite = stack.pop();
                if (sprite == nullptr) break;

                if ((!visible) || sprite->isVisible_) {
                    sprite->worldTransform_ = sprite->parent_->worldTransform_
                            * sprite->localTransform_.matrix();

                    if (!visible)
                        stack.push_all(sprite->children_.data(),sprite->children_.size());
                    else {
                        int sc=sprite->skipSet_.size();
                        const char *sd=sprite->skipSet_.data();
                        int sz = sprite->children_.size();
                        for (int i = 0; i <sz; i++)
                        {
                            if ((i>=sc)||(!sd[i]))
                                stack.push(sprite->children_[i]);
                        }
                    }
                }
            }
            if (xformValid) *xformValid=true;
        }
    }

    //Compute bounds
    {
        float gminx = 1e30, gminy = 1e30, gmaxx = -1e30, gmaxy = -1e30;

        faststack<const Sprite> stack;
        std::stack<ClipRect> cstack;
        ClipRect noclip={-1e30,1e30,-1e30,1e30};
        stack.push(this);
        if (visible)
            cstack.push(noclip);

        //Gather children bounds
        while (true) {
            const Sprite *sprite = stack.pop();
            if (sprite == nullptr) break;
            ClipRect clip;
            if (visible)
            {
                clip= cstack.top();
                cstack.pop();
                if (!sprite->isVisible_)
                    continue;

                if ((sprite->clipw_ >= 0) && (sprite->cliph_ >= 0)) {
                    float x[4], y[4];
                    //Transform clip coordinates
                    sprite->worldTransform_.transformPoint(sprite->clipx_, sprite->clipy_, &x[0],
                            &y[0]);
                    sprite->worldTransform_.transformPoint(sprite->clipx_ + sprite->clipw_ - 1,
                            sprite->clipy_, &x[1], &y[1]);
                    sprite->worldTransform_.transformPoint(sprite->clipx_ + sprite->clipw_ - 1,
                            sprite->clipy_ + sprite->cliph_ - 1, &x[2], &y[2]);
                    sprite->worldTransform_.transformPoint(sprite->clipx_,
                            sprite->clipy_ + sprite->cliph_ - 1, &x[3], &y[3]);

                    float cminx = 1e30, cminy = 1e30, cmaxx = -1e30, cmaxy =
                            -1e30;
                    for (int i = 0; i < 4; ++i) {
                        cminx = std::min(cminx, x[i]);
                        cminy = std::min(cminy, y[i]);
                        cmaxx = std::max(cmaxx, x[i]);
                        cmaxy = std::max(cmaxy, y[i]);
                    }

                    clip.xmin = std::max(clip.xmin, cminx);
                    clip.ymin = std::max(clip.ymin, cminy);
                    clip.xmax = std::min(clip.xmax,	cmaxx);
                    clip.ymax = std::min(clip.ymax,	cmaxy);
                }
            }

            float eminx, eminy, emaxx, emaxy;
            sprite->extraBounds(&eminx, &eminy, &emaxx, &emaxy);

            if (eminx <= emaxx && eminy <= emaxy) {
                float x[4], y[4];

                float lgminx = 1e30, lgminy = 1e30, lgmaxx = -1e30, lgmaxy = -1e30;

                sprite->worldTransform_.transformPoint(eminx, eminy, &x[0],
                        &y[0]);
                sprite->worldTransform_.transformPoint(emaxx, eminy, &x[1],
                        &y[1]);
                sprite->worldTransform_.transformPoint(emaxx, emaxy, &x[2],
                        &y[2]);
                sprite->worldTransform_.transformPoint(eminx, emaxy, &x[3],
                        &y[3]);

                for (int i = 0; i < 4; ++i) {
                    lgminx = std::min(lgminx, x[i]);
                    lgminy = std::min(lgminy, y[i]);
                    lgmaxx = std::max(lgmaxx, x[i]);
                    lgmaxy = std::max(lgmaxy, y[i]);
                }


                //Clip
                if (visible)
                {
                    lgminx = std::max(lgminx, clip.xmin);
                    lgminy = std::max(lgminy, clip.ymin);
                    lgmaxx = std::min(lgmaxx, clip.xmax);
                    lgmaxy = std::min(lgmaxy, clip.ymax);
                }

                //Contribute to global bounds
                gminx = std::min(lgminx, gminx);
                gminy = std::min(lgminy, gminy);
                gmaxx = std::max(lgmaxx, gmaxx);
                gmaxy = std::max(lgmaxy, gmaxy);
            }

            if (!nosubs) {
                int sc=sprite->skipSet_.size();
                const char *sd=sprite->skipSet_.data();
                int sz = sprite->children_.size();
                for (int i = 0; i <sz; i++)
                {
                    if (visible) {
                        if ((i>=sc)||(!sd[i]))
                            stack.push(sprite->children_[i]);
                        cstack.push(clip);
                    }
                    else {
                        stack.push(sprite->children_[i]);
                    }
                }
            }
        }

        if (visible) //Check parent hierarchy for clipping/invisibility
        {
            const Sprite *sprite = this;
            Matrix4 t = transform;
            while (sprite->parent_) {
                if (parentXform.empty())
                    t = t * sprite->localTransform_.matrix().inverse();
                else {
                    t = parentXform.top();
                    parentXform.pop();
                }
                sprite = sprite->parent_;
                if (visible && (sprite->clipw_ >= 0) && (sprite->cliph_ >= 0)) {
                    float x[4], y[4];

                    t.transformPoint(sprite->clipx_, sprite->clipy_, &x[0],
                            &y[0]);
                    t.transformPoint(sprite->clipx_ + sprite->clipw_ - 1,
                            sprite->clipy_, &x[1], &y[1]);
                    t.transformPoint(sprite->clipx_ + sprite->clipw_ - 1,
                            sprite->clipy_ + sprite->cliph_ - 1, &x[2], &y[2]);
                    t.transformPoint(sprite->clipx_,
                            sprite->clipy_ + sprite->cliph_ - 1, &x[3], &y[3]);

                    float cminx = 1e30, cminy = 1e30, cmaxx = -1e30, cmaxy =
                            -1e30;
                    for (int i = 0; i < 4; ++i) {
                        cminx = std::min(cminx, x[i]);
                        cminy = std::min(cminy, y[i]);
                        cmaxx = std::max(cmaxx, x[i]);
                        cmaxy = std::max(cmaxy, y[i]);
                    }
                    gminx = std::max(gminx, cminx);
                    gminy = std::max(gminy, cminy);
                    gmaxx = std::min(gmaxx, cmaxx);
                    gmaxy = std::min(gmaxy, cmaxy);
                }
            }
        }

        boundsCache[cacheMode].minx=gminx;
        boundsCache[cacheMode].miny=gminy;
        boundsCache[cacheMode].maxx=gmaxx;
        boundsCache[cacheMode].maxy=gmaxy;

        if (minx)
            *minx = gminx;
        if (miny)
            *miny = gminy;
        if (maxx)
            *maxx = gmaxx;
        if (maxy)
            *maxy = gmaxy;
    }

    if (mode==BOUNDS_UNSPEC) return;
    boundsCache[cacheMode].valid=true;
#endif
}

void Sprite::getBounds(const Sprite* targetCoordinateSpace, float* minx,
        float* miny, float* maxx, float* maxy, bool visible) {
	bool found = false;
	Matrix transform;
	const Sprite *curr = this;
	while (curr) {
		if (curr == targetCoordinateSpace) {
			found = true;
			break;
		}
		transform = curr->localTransform_.matrix() * transform;
		curr = curr->parent_;
	}

	if (found == false) {
		Matrix inverse;
		const Sprite *curr = targetCoordinateSpace;
		while (curr) {
			inverse = inverse * curr->localTransform_.matrix().inverse();
			curr = curr->parent_;
		}

		transform = inverse * transform;
	}

    std::vector<Matrix4> pxform;
    boundsHelper(transform, minx, miny, maxx, maxy, pxform, nullptr, visible, false,BOUNDS_UNSPEC);
}

void Sprite::setBlendFunc(ShaderEngine::BlendFactor sfactor,
		ShaderEngine::BlendFactor dfactor) {
	sfactor_ = sfactor;
	dfactor_ = dfactor;
	invalidate(INV_GRAPHICS);
}

void Sprite::clearBlendFunc() {
	sfactor_ = (ShaderEngine::BlendFactor) -1;
	dfactor_ = (ShaderEngine::BlendFactor) -1;
	invalidate(INV_GRAPHICS);
}

void Sprite::setColorTransform(const ColorTransform& colorTransform) {
	if (colorTransform_ == 0)
		colorTransform_ = new ColorTransform();

	*colorTransform_ = colorTransform;
	invalidate(INV_GRAPHICS);
}

void Sprite::setAlpha(float alpha) {
	alpha_ = alpha;
	invalidate(INV_GRAPHICS);
}

void Sprite::eventListenersChanged() {
	Stage *stage = getStage();
	if (stage)
		stage->setSpritesWithListenersDirty();

	if (hasEventListener(Event::ENTER_FRAME))
		allSpritesWithListeners_.insert(this);
	else
		allSpritesWithListeners_.erase(this);
}

void Sprite::setStopPropagationMask(int mask) {
	stopPropagationMask_=mask;
	eventListenersChanged();
}


bool Sprite::setDimensions(float w,float h, bool forLayout)
{
//    bool changed=((reqWidth_!=w)||(reqHeight_!=h));
    G_UNUSED(forLayout);
    bool changed=(fabs(reqWidth_-w)+fabs(reqHeight_-h))>0.01;
    if (changed) {
        reqWidth_=w;
        reqHeight_=h;
        if (layoutState)
            layoutState->dirty=true;
        if (!forLayout) {
            //invalidate(INV_CONSTRAINTS);
            layoutSizesChanged();
        }
        redrawEffects();

        if (hasEventListener(LayoutEvent::RESIZED))
        {
            LayoutEvent event(LayoutEvent::RESIZED,w,h);
            dispatchEvent(&event);
        }
    }
    return changed;
}

void Sprite::set(const char* param, float value, GStatus* status) {
	int id = StringId::instance().id(param);
	set(id, value, status);
}

float Sprite::get(const char* param, GStatus* status) {
	int id = StringId::instance().id(param);
	return get(id, status);
}

void Sprite::set(int param, float value, GStatus* status) {
	switch (param) {
	case eStringIdX:
		setX(value);
		break;
	case eStringIdY:
		setY(value);
		break;
	case eStringIdZ:
		setZ(value);
		break;
	case eStringIdRotation:
		setRotation(value);
		break;
	case eStringIdRotationX:
		setRotationX(value);
		break;
	case eStringIdRotationY:
		setRotationY(value);
		break;
	case eStringIdScale:
		setScale(value);
		break;
	case eStringIdScaleX:
		setScaleX(value);
		break;
	case eStringIdScaleY:
		setScaleY(value);
		break;
	case eStringIdScaleZ:
		setScaleZ(value);
		break;
	case eStringIdAnchorX:
		setRefX(value);
		break;
	case eStringIdAnchorY:
		setRefY(value);
		break;
	case eStringIdAnchorZ:
		setRefZ(value);
		break;
	case eStringIdAlpha:
		setAlpha(value);
		break;
	case eStringIdRedMultiplier:
		setRedMultiplier(value);
		break;
	case eStringIdGreenMultiplier:
		setGreenMultiplier(value);
		break;
	case eStringIdBlueMultiplier:
		setBlueMultiplier(value);
		break;
	case eStringIdAlphaMultiplier:
		setAlphaMultiplier(value);
		break;
    case eStringIdSkewX:
        setSkewX(value);
        break;
    case eStringIdSkewY:
        setSkewY(value);
        break;
	default:
		if (status)
			*status = GStatus(2008, "param"); // Error #2008: Parameter '%s' must be one of the accepted values.
		break;
	}
}

float Sprite::get(int param, GStatus* status) {
	switch (param) {
	case eStringIdX:
		return x();
	case eStringIdY:
		return y();
	case eStringIdZ:
		return z();
	case eStringIdRotation:
		return rotation();
	case eStringIdRotationX:
		return rotationX();
	case eStringIdRotationY:
		return rotationY();
	case eStringIdScaleX:
		return scaleX();
	case eStringIdScaleY:
		return scaleY();
	case eStringIdScaleZ:
		return scaleZ();
	case eStringIdAnchorX:
		return refX();
	case eStringIdAnchorY:
		return refY();
	case eStringIdAnchorZ:
		return refZ();
	case eStringIdAlpha:
		return alpha();
	case eStringIdRedMultiplier:
		return getRedMultiplier();
	case eStringIdGreenMultiplier:
		return getGreenMultiplier();
	case eStringIdBlueMultiplier:
		return getBlueMultiplier();
	case eStringIdAlphaMultiplier:
		return getAlphaMultiplier();
    case eStringIdSkewX:
        return skewX();
    case eStringIdSkewY:
        return skewY();
	}

	if (status)
		*status = GStatus(2008, "param"); // Error #2008: Parameter '%s' must be one of the accepted values.

	return 0;
}

void Sprite::setRedMultiplier(float redMultiplier) {
	if (colorTransform_ == NULL)
		colorTransform_ = new ColorTransform();

	colorTransform_->setRedMultiplier(redMultiplier);
	invalidate(INV_GRAPHICS);
}

void Sprite::setGreenMultiplier(float greenMultiplier) {
	if (colorTransform_ == NULL)
		colorTransform_ = new ColorTransform();

	colorTransform_->setGreenMultiplier(greenMultiplier);
	invalidate(INV_GRAPHICS);
}

void Sprite::setBlueMultiplier(float blueMultiplier) {
	if (colorTransform_ == NULL)
		colorTransform_ = new ColorTransform();

	colorTransform_->setBlueMultiplier(blueMultiplier);
	invalidate(INV_GRAPHICS);
}

void Sprite::setAlphaMultiplier(float alphaMultiplier) {
	if (colorTransform_ == NULL)
		colorTransform_ = new ColorTransform();

	colorTransform_->setAlphaMultiplier(alphaMultiplier);
	invalidate(INV_GRAPHICS);
}

float Sprite::getRedMultiplier() const {
	if (colorTransform_ == NULL)
		colorTransform_ = new ColorTransform();

	return colorTransform_->redMultiplier();
}

float Sprite::getGreenMultiplier() const {
	if (colorTransform_ == NULL)
		colorTransform_ = new ColorTransform();

	return colorTransform_->greenMultiplier();
}

float Sprite::getBlueMultiplier() const {
	if (colorTransform_ == NULL)
		colorTransform_ = new ColorTransform();

	return colorTransform_->blueMultiplier();
}

float Sprite::getAlphaMultiplier() const {
	if (colorTransform_ == NULL)
		colorTransform_ = new ColorTransform();

	return colorTransform_->alphaMultiplier();
}

void Sprite::applyGhost(Sprite *parent,GhostSprite *g,bool leave) {
	if (leave) return;
    Sprite *lParent=parent;
    float ox=0,oy=0;
    while (lParent&&(lParent->layoutConstraints)&&
            ((lParent->layoutConstraints->inGroup)||(lParent->layoutConstraints->group)))
    {
        ox+=lParent->x();
        oy+=lParent->y();
        lParent=lParent->parent();
    }
    if (g->matrix)
        localTransform_.setMatrix(g->matrix);
    if (g->children)
    {
        for (auto it=g->children->begin();it!=g->children->end();it++)
            (*it)->getModel()->applyGhost(this,(*it));
    }
    GridBagLayout *pl=lParent?(lParent->layoutState):NULL;
    if (pl)
    {
        GridBagLayoutInfo *pi=pl->getCurrentLayoutInfo();
        if (pi&&pi->valid) {
            GridBagConstraints *lc=this->getLayoutConstraints();
            lc->gridx=g->gridx;
            lc->gridy=g->gridy;
            pl->placeChild(parent_,this,ox,oy,0,0);
        }
    }
    if (g->children)
    {
        for (auto it=g->children->begin();it!=g->children->end();it++)
            (*it)->getModel()->applyGhost(this,(*it),true);
    }
}

void Sprite::setGhosts(std::vector<GhostSprite *> *ghosts)
{
    if (ghosts_)
    {
        for (std::size_t i = 0; i < ghosts_->size(); ++i)
            delete (*ghosts_)[i];
        delete ghosts_;
    }
    ghosts_=ghosts;
    invalidate(INV_GRAPHICS);
}

GhostSprite::GhostSprite(Sprite *m)
{
    model=m;
    model->ref();
    children=nullptr;
    matrix=nullptr;
}

GhostSprite::~GhostSprite()
{
    model->unref();
    if (matrix)
        delete[] matrix;
    if (children) {
        for (auto it=children->begin(); it!=children->end();it++)
            delete (*it);
        delete children;
    }
}


SpriteProxy::SpriteProxy(Application* application,void *c,SpriteProxyDraw df,SpriteProxyDestroy kf) : Sprite(application)
{
	context=c;
	drawF=df;
	destroyF=kf;
}

SpriteProxy::~SpriteProxy()
{
	destroyF(context);
}

void SpriteProxy::doDraw(const CurrentTransform& m, float sx, float sy, float ex, float ey)
{
	drawF(context,m,sx,sy,ex,ey);
}

SpriteProxy *SpriteProxyFactory::createProxy(Application* application,void *c,SpriteProxyDraw df,SpriteProxyDestroy kf) {
	return new SpriteProxy(application,c,df,kf);
}

GEventDispatcherProxy *SpriteProxyFactory::createEventDispatcher(GReferenced *c)
{
	GEventDispatcherProxy *p=new GEventDispatcherProxy();
	p->object()->setProxy(c);
	return p;
}

