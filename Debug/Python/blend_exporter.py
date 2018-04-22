import bpy
import bmesh
import os
import sys
import struct

class KTMExporter():
    args = sys.argv[sys.argv.index("--") + 1:]
    scene = bpy.context.scene
    timeline_markers = bpy.context.scene.timeline_markers
    obj = bpy.context.active_object
    arm = None

    # export prop
    frame_start = scene.frame_start
    frame_end = scene.frame_end
    frame_size = frame_end - frame_start + 1
    scale = 1.0
    
    arg = args[0]
    #meshOnly = args[2]
    #path = None

    frame_offset = 0

    def execute(self):
        print("---------export start----------")
        pos0 = self.arg.index("?")
        dirr = self.arg[:pos0]
        print (dirr)
        if os.access(dirr, os.W_OK) is False:
            print("!permission denied : " + dirr)
            return False
        name = self.arg[(pos0+1):]
        print(name)
        #self.path = os.path.join(dirr, name)
        print ("!writing to: " + dirr + name + ".blend.meta")
        
        #write mesh list to main .meta
        file2 = open(dirr + name + ".blend.meta", "wb")
        self.write(file2, "KTM123\n")
        for obj in self.scene.objects:
            if obj.type == 'MESH':
                print ("obj " + obj.name)
                self.write(file2, "obj " + obj.name)
                marm = None
                for mod in obj.modifiers:
                    if mod.type == 'ARMATURE':
                        marm = mod
                        break
                if marm != None:
                    obj.modifiers.remove(marm)
                    self.write(file2, "\x01")
                if obj.parent:
                    self.write(file2, " \x00prt " + obj.parent.name)
                    if obj.parent_type == "BONE":
                        self.write(file2, "\x02" + obj.parent_bone)
                poss = obj.location
                self.write(file2, " \x00pos {:f} {:f} {:f}".format(poss[0], poss[2], poss[1]))
                rott = obj.rotation_quaternion
                self.write(file2, " \x00rot {:f} {:f} {:f} {:f}".format(rott[0], rott[1], rott[2], rott[3]))
                scll = obj.scale
                self.write(file2, " \x00scl {:f} {:f} {:f}\n".format(scll[0], scll[2], scll[1]))
                
                
                bm = bmesh.new();
                bm.from_mesh(obj.to_mesh(bpy.context.scene, False, 'PREVIEW'));
                bpy.context.scene.objects.active = obj
                if bm.verts.layers.shape.keys():
                    obj.modifiers.clear()
                    #bpy.ops.object.shape_key_remove(all=True) #shape keys will be ignored when calling to_mesh(,True,)
                
                print ("!writing to: " + dirr + name + "_blend\\" + obj.name + ".mesh.meta")
                file = open(dirr + name + "_blend\\" + obj.name + ".mesh.meta", "wb")
                obj.modifiers.new("EdgeSplit", 'EDGE_SPLIT')
                obj.modifiers["EdgeSplit"].split_angle = 1.0472 #60 degrees
                obj.modifiers["EdgeSplit"].use_edge_sharp = True
                obj.modifiers.new("Triangulate", 'TRIANGULATE')
                m = obj.to_mesh(bpy.context.scene, True, 'PREVIEW') #struct.pack("<i", len(m.loops))
                
                #extract and clean vertex data
                verts = m.vertices
                vrts = [] #(vert_id, uv)
                loop2vrt = [] #int
                vrt2vert = [] #int
                
                if m.uv_layers:
                    uid = 0
                    vrtc = 0
                    for uvl in m.uv_layers[0].data:
                        pr = (m.loops[uid].vertex_index, uvl.uv)
                        try:
                            vid = vrts.index(pr)
                        except ValueError:
                            vrts.append(pr)
                            loop2vrt.append(vrtc)
                            vrt2vert.append(pr[0])
                            vrtc = vrtc + 1
                        else:
                            loop2vrt.append(vid)
                        uid = uid + 1
                
                vrtsz = len(vrts)
                
                #write data
                self.write(file, "KTO123" + obj.name + "\x00V") #V size4 [vert4+norm4 array] F size4 [3xface4 array] (U size1 [uv02 array] (uv12 array))
                file.write(struct.pack("<i", vrtsz))
                for vrt in vrts:
                    v = verts[vrt[0]]
                    file.write(struct.pack("<fff", v.co[0], v.co[2], v.co[1]))
                    file.write(struct.pack("<fff", v.normal[0], v.normal[2], v.normal[1]))
                
                self.write(file, "F")
                file.write(struct.pack("<i", len(m.polygons)))
                for poly in m.polygons:
                    file.write(struct.pack("<B", poly.material_index))
                    file.write(struct.pack("<iii", loop2vrt[poly.loop_indices[0]], loop2vrt[poly.loop_indices[2]], loop2vrt[poly.loop_indices[1]]))
                
                if m.uv_layers:
                    self.write(file, "U")
                    file.write(struct.pack("<B", 1))
                    for v in vrts:
                        file.write(struct.pack("<ff", v[1][0], v[1][1]))
                
                
                #G groupSz1 [groupName NULL] (for each vert)[groupSz1 [groupId1 groupWeight4]]
                self.write(file, "G")
                file.write(struct.pack("<B", len(obj.vertex_groups)))
                for grp in obj.vertex_groups:
                    self.write(file, grp.name + "\x00")
                for vrt in vrts:
                    vert = verts[vrt[0]]
                    file.write(struct.pack("<B", len(vert.groups)))
                    for grp in vert.groups:
                        file.write(struct.pack("<Bf", grp.group, grp.weight))
                
                if len(bm.verts.layers.shape.keys()) > 1:
                    isFirst = True
                    self.write(file, "S")
                    file.write(struct.pack("<B", len(bm.verts.layers.shape.keys()) - 1))
                    bm.verts.ensure_lookup_table()
                    
                    vert2bvt = []
                    for v2 in verts:
                        i = 0
                        for v in bm.verts:
                            if v2.co == v.co:
                                vert2bvt.append(i)
                                break
                            i = i + 1
                    
                    for key in bm.verts.layers.shape.keys():
                        if isFirst:
                            isFirst = False
                        else:
                            #lid = 0
                            val = bm.verts.layers.shape.get(key)
                            print("  key %s" % (key))
                            self.write(file, key + "\x00")
                            #sk=obj.data.shape_keys.key_blocks[key]
                            #print("   v=%f, f=%f" % (sk.value, sk.frame))
                            #for v in bm.verts:
                            #    delta = v[val] - v.co
                            #    file.write(struct.pack("<fff", delta[0], delta[2], delta[1]))
                                #if (delta.length > 0):
                                #print ("    %i = %s" % (lid, delta))
                                #lid = lid+1
                            for vt in vrt2vert:
                                v = bm.verts[vert2bvt[vt]]
                                delta = v[val] - v.co
                                file.write(struct.pack("<fff", delta[0], delta[2], delta[1]))
                    
                file.close()
            elif obj.type == "ARMATURE":
                self.export_arm(file2, obj, dirr, name)
        file2.close()
        self.export_anim(dirr + name + "_blend\\")
        print ("-------export end--------")
    
    def export_arm(self, file2, obj, dirr, name):
        obj.data.pose_position = 'REST'
        self.write(file2, "arm " + obj.name)
        if obj.parent:
            self.write(file2, " \x00prt " + obj.parent.name)
        poss = obj.location
        self.write(file2, " \x00pos {:f} {:f} {:f}".format(poss[0], poss[2], poss[1]))
        rott = obj.rotation_quaternion
        self.write(file2, " \x00rot {:f} {:f} {:f} {:f}".format(rott[0], rott[1], rott[2], rott[3]))
        scll = obj.scale
        self.write(file2, " \x00scl {:f} {:f} {:f}\n".format(scll[0], scll[2], scll[1]))
        
        print ("!writing to: " + dirr + name + "_blend\\" + obj.name + ".arma.meta")
        file = open(dirr + name + "_blend\\" + obj.name + ".arma.meta", "wb")
        self.write(file, "ARM\x00")
        self.write_bone(file, obj.data.bones)
        file.close()
    
    def write_bone(self, file, bones):
        for bone in bones:
            self.write(file, "B" + bone.name + "\x00")
            if bone.parent:
                self.write(file, bone.parent.name)
            self.write(file, "\x00")
            file.write(struct.pack("<fff", bone.head[0], bone.head[2], bone.head[1]))
            file.write(struct.pack("<fff", bone.tail[0], bone.tail[2], bone.tail[1]))
            file.write(struct.pack("<fff", bone.z_axis[0], bone.z_axis[2], bone.z_axis[1]))
            datamask = 0
            if self.dot(self.cross(bone.x_axis, bone.vector), bone.z_axis) > 0:
                datamask += 240
            if bone.use_connect:
                datamask += 15
            file.write(struct.pack("<B", datamask))
            #self.write_bone(file, bone.children)
            self.write(file, "b")
    
    def cross(self, a, b):
        c = [a[2]*b[1] - a[1]*b[2], a[1]*b[0] - a[0]*b[1], a[0]*b[2] - a[2]*b[0]]
        return c
    
    def dot(self, a, b):
        return a[0]*b[0] + a[1]*b[1] + a[2]*b[2]
    
    #pose.bones["foo"].location 0x10 ~ 0x12
    #pose.bones["foo"].rotation_quaternion 0x13 ~ 0x16
    #pose.bones["foo"].scale 0x17 ~ 0x19
    #key_blocks["foo"].value 0x20
    #location 0xf0 ~ 0xf2
    #rotation_euler 0xf3 ~ 0xf5
    #scale 0xf7 ~ 0xf9
    def export_anim(self, path):
        arm = None
        for obj in self.scene.objects:
            if obj.type == 'ARMATURE':
                arm = obj;
                break
        if arm == None:
            return
            
        #obj.data.pose_position = 'REST'
        
        abones = arm.pose.bones
        #restmats = [] #foreach bone {TRS}
        #for bn in abones:
        #    mat = self.bonelocalmat(bn)
        #    restmats.append([mat.to_translation(), mat.to_quaternion(), mat.to_scale()]);
        
        obj.data.pose_position = 'POSE'
        
        for action in bpy.data.actions:
            if len(action.fcurves) == 0:
                continue
            if action.id_root != "OBJECT":
                continue
            
            arm.animation_data.action = action
            
            frange = action.frame_range
            fr0 = max(int(frange[0]), 0)
            fr1 = max(int(frange[1]), 0)
            
            mats = [] #foreach frame { foreach bone {TRS} }
            for f in range(fr0, fr1 + 1):
                self.scene.frame_set(f)
                mats.append([]);
                i = 0
                for bn in abones:
                    mat = self.bonelocalmat(bn)
                    mats[f-fr0].append([mat.to_translation(), mat.to_quaternion(), mat.to_scale()])
                    i += 1
            
            print ("!writing to: " + path + action.name + ".animclip")
            file = open(path + action.name + ".animclip", "wb")
            self.write(file, "ANIM")
            file.write(struct.pack("<H", len(abones)*3)) #TRS
            file.write(struct.pack("<HH", fr0, fr1))
            
            i = 0
            for bn in abones:
                bfn = self.bonefullname(bn, "")
                self.write(file, "\x10") #FC_BoneLoc
                self.write(file, bfn + "\x00\x00")
                file.write(struct.pack("<H", fr1 - fr0 + 1))
                for f in range(fr0, fr1 + 1):
                    res = mats[f-fr0][i][0]
                    file.write(struct.pack("<ifff", f, res[0], res[2], res[1]))
                
                self.write(file, "\x11") #FC_BoneRot
                self.write(file, bfn + "\x00\x00")
                file.write(struct.pack("<H", fr1 - fr0 + 1))
                for f in range(fr0, fr1 + 1):
                    res = mats[f-fr0][i][1]
                    file.write(struct.pack("<iffff", f, -res[1], -res[3], -res[2], res[0]))
                
                self.write(file, "\x12") #FC_BoneScl
                self.write(file, bfn + "\x00\x00")
                file.write(struct.pack("<H", fr1 - fr0 + 1))
                for f in range(fr0, fr1 + 1):
                    res = mats[f-fr0][i][2]
                    file.write(struct.pack("<ifff", f, res[0], res[2], res[1]))
                
                i += 1
            
            file.close()
    
    def bonelocalmat (self, bone):
        if bone.parent == None:
            return bone.matrix
        else:
            return bone.parent.matrix.inverted() * bone.matrix
    
    def bonefullname (self, bone, par):
        if bone.parent:
            return self.bonefullname(bone.parent, bone.name + "/" + par)
        else:
            return bone.name + "/" + par

    def write (self, file, _str):
        file.write(_str.encode())

if __name__ == "__main__":
    KTMExporter().execute()

## This poem below is stolen from https://www.reddit.com/r/copypasta/comments/51cvdw/why_python_sucks/ ##

# Let's start with indentation errors. What the actual fuck? No sane programming language structures itself on tabs, you want clear blocks? You want an end, well fuck you! Better make sure it's all aligned well. Are you 4 layers down into indentation, better make sure that screen real estate is holding up, wouldn't want to cause you any fucking stupid problems or anything. Wanna use tabs? Sure, use tabs, prettiest things on earth, some spaces, why not, throw it all in, oh wait, I like either one but not both together. Your dirty friend doesn't know how to set up his vim (and it shouldn't damn well matter), :retab won't work because you've now created a horrible mess. Oh, wanna git clone some code, for sure, just make sure you use the exact same configuration, and please do follow the joke of a style guide, it's the only way our programs actually run. BECAUSE THEy ARE DEPENDANT ON STYLE WHAT THE FUCK. Semicolons are optional, lovely. Want to flatten an array, .flatten? Nooooo, gotta overload the + operator sum(fuck,+) this how disgusting is that. Want to unique a list? .uniq? Something like that? Noooo list(set(mylist)) are you kidding me?! Type conversion will solve all our issues. 100. most disguting syntax, and a loop just iterates through a range. Ohhh, backwards compatability? Nopeeeeee, wouldn't want that in a scripting language now would we, too bad our original design was so messed up we had to be so brave to basically rewrite the whole thing, not like anyone was using it or anything.... And holy crap the syntax, most ugly thing I've ever seen what on earth Curly braces noooooope, too easy and they have the gall to call this entire monster the "pythonist" way, must be . Want to overload default operators in your class, just what they look like right? Nooope, every single one has a godamn special name, let's take a look at the list right, couldn't make it simple or anything. Want a ternary, eh? Something simple two character ?: magic? No, fuck you, use [res1,res2](condition) or a assbackwards most confusing and ugly horrific res1 if condition else res 2, I mean what the actualy fuck is that? Oh and let's preface all our functions with a thousand ____ like the tears you cry when you try to make sure it's the correct number, oh and always make sure to pass in self, wouldn't want to have scope or get lost or anything nooooo.

# Well you may ask? Surely python must have something good right? All those scientists must use it for a reason (and 90% of them write horrific code and have no clue about it by the way, as is unsurprising, hence the python). Well yeah, it has some nice libraries, but that's like saying it's a nice car because someone decided to build a fancy trailer for it. And it automatically freezes string literals, that's slightly convenient.

## end copy-pasta ##