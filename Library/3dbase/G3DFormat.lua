G3DFormat={}

local g3dNum=0

function G3DFormat.computeG3DSizes(g3d)
	if g3d.type=="group" then
		for _,v in pairs(g3d.parts) do
			G3DFormat.computeG3DSizes(v)
			if g3d.min then
				g3d.min={math.min(g3d.min[1],v.min[1]),math.min(g3d.min[2],v.min[2]),math.min(g3d.min[3],v.min[3])}
				g3d.max={math.max(g3d.max[1],v.max[1]),math.max(g3d.max[2],v.max[2]),math.max(g3d.max[3],v.max[3])}
			else
				g3d.min={v.min[1],v.min[2],v.min[3]}
				g3d.max={v.max[1],v.max[2],v.max[3]}
			end
		end
		if not g3d.min then
			g3d.min={0,0,0}
			g3d.max={0,0,0}
		end
	elseif g3d.type=="mesh" then
		local minx,miny,minz=100000,100000,100000
		local maxx,maxy,maxz=-100000,-100000,-100000
		for id=1,#g3d.indices do
			local i=g3d.indices[id]*3-2
			local x,y,z=g3d.vertices[i],g3d.vertices[i+1],g3d.vertices[i+2]
			minx=math.min(minx,x)
			miny=math.min(miny,y)
			minz=math.min(minz,z)
			maxx=math.max(maxx,x)
			maxy=math.max(maxy,y)
			maxz=math.max(maxz,z)
		end
		g3d.min={minx,miny,minz}
		g3d.max={maxx,maxy,maxz}
	elseif g3d.type=="voxel" then
		g3d.min={0,0,0}
	else
		assert(g3d.type,"No type G3D structure")
		assert(false,"Unrecognized object type: "..g3d.type)
	end
	local m=nil
	if g3d.transform then
		m=Matrix.new()
		m:setMatrix(unpack(g3d.transform))
	elseif g3d.srt then
		m=G3DFormat.srtToMatrix(g3d.srt)
	end
	if m then
		local x1,y1,z1=m:transformPoint(g3d.min[1],g3d.min[2],g3d.min[3])
		local x2,y2,z2=m:transformPoint(g3d.max[1],g3d.max[2],g3d.max[3])
		g3d.min[1],g3d.min[2],g3d.min[3]=x1><x2,y1><y2,z1><z2
		g3d.max[1],g3d.max[2],g3d.max[3]=x1<>x2,y1<>y2,z1<>z2
	end

	g3d.center={(g3d.max[1]+g3d.min[1])/2,(g3d.max[2]+g3d.min[2])/2,(g3d.max[3]+g3d.min[3])/2}
end

function G3DFormat.makeSerializable(g3d,mtls)
	if g3d.type=="group" then
		for _,v in pairs(g3d.parts) do
			G3DFormat.makeSerializable(v,mtls)
		end
	elseif g3d.type=="mesh" then
		local mat=g3d.material
		if mtls and type(mat)=="string" then 
			mat=mtls[mat]
			g3d.material=mat
		end
		if mat then
			mat.texture=nil
		end
		if mat and mat.embeddedtexture and not mat.embeddedtexture.b64 then
			local rt=RenderTarget.new(mat.embeddedtexture:getWidth(),mat.embeddedtexture:getHeight())
			local bm=Bitmap.new(mat.embeddedtexture)
			rt:clear(0,0)
			rt:draw(bm)
			g3dNum+=1
			local bname="_g3d_"..g3dNum..".png"			
			local bb=Buffer.new(bname) -- embedded texture buffer
			rt:save("|B|"..bname)
			mat.embeddedtexture={ type="png", b64=Cryptography.b64(bb:get()) }
		end
	elseif g3d.type=="voxel" then
		
	else
		assert(g3d.type,"No type G3D structure")
		assert(false,"Unrecognized object type: "..g3d.type)
	end
end

function G3DFormat.stackMatrix(source,root)
	local s={}
	while source and source~=root do 
		table.insert(s,source) 
		source=source:getParent() 
	end
	assert(source==root,"Root node isn't a parent of source node")
	local m=Matrix.new()
	while true do
		local p=table.remove(s)
		if not p then break end
		m:multiply(p:getMatrix())
	end
	return m
end

function G3DFormat.sprToSprMatrix(from,to,top)
	if Sprite.spriteToLocalMatrix then return to:spriteToLocalMatrix(from) end
	-- Mat MUL: if 1,2:A,3:B, then B=3->2, A=2->1, A*B=3->1
	local mi=G3DFormat.stackMatrix(to,top) --to->top
	mi:invert() --top->to
	mi:multiply(G3DFormat.stackMatrix(from,top)) --(top->to*from->top)->from->to
	return mi
end

G3DFormat.srtToMatrix=Matrix.fromSRT

function G3DFormat.quaternionToEuler(w,x,y,z)
	-- roll (x-axis rotation)
	local sinr_cosp = 2 * (w * x + y * z)
	local cosr_cosp = 1 - 2 * (x * x + y * y)
	local rx = math.atan2(sinr_cosp, cosr_cosp)

	-- pitch (y-axis rotation)
	local sinp = 2 * (w * y - z * x)
	local ry
	if (math.abs(sinp) >= 1) then
		ry= 3.141592654 / 2
		if sinp<0 then ry=-ry end
	else
		ry = math.asin(sinp)
	end

	-- yaw (z-axis rotation)
	local siny_cosp = 2 * (w * z + x * y)
	local cosy_cosp = 1 - 2 * (y * y + z * z)
	local rz = math.atan2(siny_cosp, cosy_cosp)
	return ^>rx,^>ry,^>rz
end

function G3DFormat.buildG3DObject(obj,mtls,top,assetResolver)
	local m=D3.Mesh.new()
	m.name=obj.name
	m:setVertexArray(obj.vertices)
	m:setIndexArray(obj.indices)

	-- the 'naked' object
	mtls=mtls or {}
	local mtl={}
	if obj.material then 
		if type(obj.material)=="table" then
			mtl=obj.material
		else
			mtl=mtls[obj.material]
			assert(mtl,"No such material: "..obj.material)
		end
	end

	local smode=0
	-- COLOR?
	if obj.color then
		m:setColorTransform(obj.color[1],obj.color[2],obj.color[3],obj.color[4])
	end
	-- DIFFUSE
	if mtl.embeddedtexture and not mtl.texture then -- .glb embedded texture (buffer)
		if mtl.embeddedtexture.b64 then --Assume Base64
			local iext=mtl.embeddedtexture.type
			local data=Cryptography.unb64(mtl.embeddedtexture.b64)
			g3dNum+=1
			local bname="_g3d_"..g3dNum.."."..iext
			local bb=Buffer.new(bname) -- embedded texture buffer
			bb:set(data)
			mtl.texture=Texture.new("|B|"..bname,true,{ wrap=TextureBase.REPEAT, extend=false})
		else
			mtl.texture=mtl.embeddedtexture
		end
		mtl.texturew=mtl.texture:getWidth()
		mtl.textureh=mtl.texture:getHeight()
	end
	if mtl.textureFile and not mtl.texture then -- .fbx texture (file path)
		local path = mtl.modelpath or ""
		mtl.textureFile = string.gsub(mtl.textureFile, "\\", "/") -- fix for android (and linux?)
		local tpath=path..mtl.textureFile
		if assetResolver and assetResolver.resolvePath then
			tpath=assetResolver.resolvePath(tpath,"texture")
		end
		if tpath then
			mtl.texture=Texture.new(tpath,true,{ wrap=TextureBase.REPEAT, extend=false})
		end
		mtl.texturew=mtl.texture:getWidth()
		mtl.textureh=mtl.texture:getHeight()
	end
	if (mtl.texture~=nil) then
		m:setTexture(mtl.texture)
		local tc={}
		for i=1,#obj.texcoords,2 do
			tc[i]=obj.texcoords[i]*mtl.texturew
			tc[i+1]=obj.texcoords[i+1]*mtl.textureh
		end
		m:setTextureCoordinateArray(tc)
		m.hasTexture=true
		smode=smode|D3.Mesh.MODE_TEXTURE
	end
	-- NORMAL
	-- TO DO: .glb embedded normal map texture (buffer)
	if mtl.normalMapFile and not mtl.normalMap then -- .fbx normal map texture (file path)
		local path = mtl.modelpath or ""
		mtl.normalMapFile = string.gsub(mtl.normalMapFile, "\\", "/") -- fix for android (and linux?)
		local tpath=path..mtl.normalMapFile
		if assetResolver and assetResolver.resolvePath then
			tpath=assetResolver.resolvePath(tpath,"texture")
		end
		if tpath then
			mtl.normalMap=Texture.new(tpath,true,{ wrap=TextureBase.REPEAT, extend=false})
		end
	end
	if (mtl.normalMap~=nil) then
		m:setTexture(mtl.normalMap,1)
		m.hasNormalMap=true
		smode=smode|D3.Mesh.MODE_BUMP
	end
	if mtl.kd then
		m:setColorTransform(mtl.kd[1],mtl.kd[2],mtl.kd[3],mtl.kd[4])
	end
	if obj.normals then
		m.hasNormals=true
		m:setGenericArray(3,Shader.DFLOAT,3,#obj.normals/3,obj.normals)
		smode=smode|D3.Mesh.MODE_LIGHTING
	end
	if obj.colors then
		m.hasColors=true
		local tc={}
		local noAlpha=#obj.colors<#obj.vertices
		local skip=if noAlpha then 3 else 4
		for i=1,#obj.colors,skip do
			local n=1+((i-1)/skip)*2
			tc[n]=(((obj.colors[i]*255)&0xFF)<<16)|(((obj.colors[i+1]*255)&0xFF)<<8)|(((obj.colors[i+2]*255)&0xFF))
			tc[n+1]=if noAlpha then 1 else obj.colors[i+3]
		end
		m:setColorArray(tc)
		smode=smode|D3.Mesh.MODE_COLORED
	end
	if obj.animdata then
		m:setGenericArray(4,Shader.DFLOAT,4,#obj.animdata.bi/4,obj.animdata.bi)
		m:setGenericArray(5,Shader.DFLOAT,4,#obj.animdata.bw/4,obj.animdata.bw)
		if top and top.bones and obj.bones then
			m.animBones={}
			for k,v in ipairs(obj.bones) do
				m.animBones[k]={ boneref=v.node, poseMat=v.poseMat, poseIMat=v.poseIMat }
				if v.poseSrt then m.animBones[k].poseMat=G3DFormat.srtToMatrix(v.poseSrt) end
				top.animMeshes=top.animMeshes or {}
				top.animMeshes[m]=true
			end
			m.bonesTop=top
			smode=smode|D3.Mesh.MODE_ANIMATED
		end
	end
	m:updateMode(smode|D3.Mesh.MODE_SHADOW,0)
	return m
end

function G3DFormat.buildG3DVoxel(obj,mtls,top)
	local m=D3.Cube.new(1,1,1)
	m.name=obj.name
	m:setGenericArray(10,Shader.DFLOAT,4,#obj.voxels/4,obj.voxels)
	local fmap={}
	for i=0,5 do
		for j=1,4 do
			fmap[i*4+j]=1<<(i+8)
		end
	end
	m:setGenericArray(11,Shader.DFLOAT,1,24,fmap)
	Mesh.setInstanceCount(m,obj.voxelCount)
	m.colorMap=Texture.new(obj.colorMap,256,1,{extend=false, rawalpha=true})
	--m:setCullMode(Sprite.CULL_BACK)
	m:updateMode(D3.Mesh.MODE_LIGHTING|D3.Mesh.MODE_SHADOW|D3.Mesh.MODE_VOXEL,0)
	return m
end

function G3DFormat.buildG3D(g3d,mtl,top,assetResolver)
	local spr=nil
	if g3d.type=="group" then
		spr=D3.Group.new()
		local ltop=top or spr
		spr.name=g3d.name
		spr.objs={}
		if g3d.bones then
			spr.bones={}
			for k,v in pairs(g3d.bones) do spr.bones[k]=v end
		end
		spr.animations=g3d.animations
		for k,v in pairs(g3d.parts) do
			local m=G3DFormat.buildG3D(v,mtl,ltop,assetResolver)
			spr:addChild(m)
			spr.objs[k]=m
			if ltop and ltop.bones then
				if ltop.bones[k] then ltop.bones[k]=m end
			end
		end
	elseif g3d.type=="mesh" then
		spr=G3DFormat.buildG3DObject(g3d,mtl,top,assetResolver)
	elseif g3d.type=="voxel" then
		spr=G3DFormat.buildG3DVoxel(g3d,mtl,top,assetResolver)
	else
		assert(g3d.type,"No type G3D structure")
		assert(false,"Unrecognized object type: "..g3d.type)
	end
	if spr then
		spr.min=g3d.min
		spr.max=g3d.max
		spr.center=g3d.center
		if g3d.transform then
			local m=Matrix.new()
			m:setMatrix(unpack(g3d.transform))
			spr:setMatrix(m)
		elseif g3d.srt then
			spr:setMatrix(Matrix.fromSRT(g3d.srt))
		end
		spr.srt=g3d.srt
	end
	if spr.animMeshes then
		for m,_ in pairs(spr.animMeshes) do
			if m.animBones then
				for _,b in ipairs(m.animBones) do
					b.bone=spr.bones[b.boneref]
					local mi=Matrix.new()
					local p=b.bone
					if b.poseMat then
						mi:setMatrix(b.poseMat:getMatrix())
						p=m
						while p do
							p:setMatrix(Matrix.new())
							if p==m.bonesTop then break end
							p=p:getParent()
						end
						p=nil
					else
						mi=G3DFormat.sprToSprMatrix(p,m.bonesTop,m.bonesTop) -->Bone Pose to Mesh
					end
					b.bone.poseMat=Matrix.new()
					b.bone.poseMat:setMatrix(mi:getMatrix())
					mi:invert() -- -> Mesh to Bone Pose
					b.bone.poseIMat=mi
				end
			end
		end
	end
	return spr
end

function G3DFormat.mapCoords(v,t,n,faces)
	local imap={}
	local vmap={}
	imap.alloc=function(self,facenm,i)
		local iv=i.v or 0
		if (iv<0) then iv=(#v/3+1+iv) end
		iv=iv-1
		local it=i.t or 0
		if (it==nil) then
			it=-1
		else
			if (it<0) then it=(#t/2)+it+1 end
			it=it-1
		end
		local inm=i.n or 0
		if (inm<0) then inm=(#n/3)+inm+1 end
		inm=inm-1
		if inm==-1 then inm=facenm.code end
		local ms=iv..":"..it..":"..inm
		if vmap[ms]==nil then
			local ni=self.ni+1
			self.ni=ni
			table.insert(facenm.lvi,#self.lv)
			assert(v[iv*3+1],"Missing Vertex:"..iv)
			table.insert(self.lv,v[iv*3+1])
			table.insert(self.lv,v[iv*3+2])
			table.insert(self.lv,v[iv*3+3])
			if it>=0 then
				table.insert(self.lvt,t[it*2+1])
				table.insert(self.lvt,(1-t[it*2+2]))
			else 
				table.insert(self.lvt,0)
				table.insert(self.lvt,0)
			end
			if inm>=0 then
				table.insert(self.lvn,n[inm*3+1])
				table.insert(self.lvn,n[inm*3+2])
				table.insert(self.lvn,n[inm*3+3])
			else 
				local vngmap=self.vngmap[iv] or { }
				self.vngmap[iv]=vngmap
				table.insert(vngmap,#self.lvn)
				table.insert(facenm.lvni,#self.lvn)
				table.insert(self.lvn,0)
				table.insert(self.lvn,0)
				table.insert(self.lvn,0)
			end
			self.vmap[ms]=ni
		end
		return self.vmap[ms]
	end		
	imap.i={}
	imap.ni=0
	imap.lv={}
	imap.lvt={}
	imap.lvn={}
	imap.vmap={}
	imap.vngmap={}
	imap.gnorm=-2
	
	for _,face in ipairs(faces) do
		local itab={}
		local normtab={ code=imap.gnorm, lvi={}, lvni={} }
		for _,i in ipairs(face) do table.insert(itab,imap:alloc(normtab,i)) end
		imap.gnorm=imap.gnorm-1
		if (#itab>=3) then
			if #normtab.lvni>0 then -- Gen normals
				local ux=imap.lv[normtab.lvi[2]+1]-imap.lv[normtab.lvi[1]+1]
				local uy=imap.lv[normtab.lvi[2]+2]-imap.lv[normtab.lvi[1]+2]
				local uz=imap.lv[normtab.lvi[2]+3]-imap.lv[normtab.lvi[1]+3]
				local vx=imap.lv[normtab.lvi[3]+1]-imap.lv[normtab.lvi[1]+1]
				local vy=imap.lv[normtab.lvi[3]+2]-imap.lv[normtab.lvi[1]+2]
				local vz=imap.lv[normtab.lvi[3]+3]-imap.lv[normtab.lvi[1]+3]
				local nx=uy*vz-uz*vy
				local ny=uz*vx-ux*vz
				local nz=ux*vy-uy*vx
				local nl=math.sqrt(nx*nx+ny*ny+nz*nz)
				if nl==0 then nl=1 end
				nx=nx/nl
				ny=ny/nl
				nz=nz/nl
				for _,vni in ipairs(normtab.lvni) do
					imap.lvn[vni+1]=nx
					imap.lvn[vni+2]=ny
					imap.lvn[vni+3]=nz
				end
			end
			for ii=3,#itab,1 do
				table.insert(imap.i,itab[1])
				table.insert(imap.i,itab[ii-1])
				table.insert(imap.i,itab[ii])
			end
		end
	end
	for _,vm in pairs(imap.vngmap) do
		local nx,ny,nz=0,0,0
		for _,vn in ipairs(vm) do
			nx=nx+imap.lvn[vn+1]
			ny=ny+imap.lvn[vn+2]
			nz=nz+imap.lvn[vn+3]
		end
		local nl=math.sqrt(nx*nx+ny*ny+nz*nz)
		if nl==0 then nl=1 end
		nx=nx/nl
		ny=ny/nl
		nz=nz/nl
		for _,vn in ipairs(vm) do
			imap.lvn[vn+1]=nx
			imap.lvn[vn+2]=ny
			imap.lvn[vn+3]=nz
		end	
	end
	return { vertices=imap.lv, texcoords=imap.lvt, normals=imap.lvn, indices=imap.i }
end
