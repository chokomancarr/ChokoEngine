import bpy
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
                poss = obj.location
                self.write(file2, " \x00pos {:f} {:f} {:f}".format(poss[0], poss[2], poss[1]))
                rott = obj.rotation_quaternion
                self.write(file2, " \x00rot {:f} {:f} {:f} {:f}".format(rott[0], rott[1], rott[2], rott[3]))
                scll = obj.scale
                self.write(file2, " \x00scl {:f} {:f} {:f}\n".format(scll[0], scll[2], scll[1]))
                
                print ("!writing to: " + dirr + name + "_blend\\" + obj.name + ".mesh.meta")
                file = open(dirr + name + "_blend\\" + obj.name + ".mesh.meta", "wb")
                obj.modifiers.new("EdgeSplit", 'EDGE_SPLIT')
                obj.modifiers["EdgeSplit"].split_angle = 1.0472 #60 degrees
                obj.modifiers["EdgeSplit"].use_edge_sharp = True
                obj.modifiers.new("Triangulate", 'TRIANGULATE')
                m = obj.to_mesh(bpy.context.scene, True, 'PREVIEW') #struct.pack("<i", len(m.loops))
                self.write(file, "KTO123" + obj.name + "\x00V") #V size4 [vert4+norm4 array] F size4 [3xface4 array] (U size1 [uv02 array] (uv12 array)) 
                file.write(struct.pack("<i", len(m.loops)))
                for loop in m.loops:
                    vert = m.vertices[loop.vertex_index]
                    #self.write(file, "    vrt {} {:f} {:f} {:f}\r\n".format(loop.index, vert.co[0], vert.co[2], vert.co[1]))
                    file.write(struct.pack("<fff", vert.co[0], vert.co[2], vert.co[1]))
                    #self.write(file, "    nrm {} {:f} {:f} {:f}\r\n".format(loop.index, vert.normal[0], vert.normal[2], vert.normal[1]))
                    file.write(struct.pack("<fff", vert.normal[0], vert.normal[2], vert.normal[1]))
                self.write(file, "F")
                file.write(struct.pack("<i", len(m.polygons)))
                for poly in m.polygons:
                    #self.write(file, "    tri {} ".format(poly.material_index))
                    #for loop_index in poly.loop_indices:
                    #self.write(file, " {} {} {}".format(poly.loop_indices[0], poly.loop_indices[2], poly.loop_indices[1]))
                    #self.write(file, "\r\n")
                    file.write(struct.pack("<B", poly.material_index))
                    file.write(struct.pack("<iii", poly.loop_indices[0], poly.loop_indices[2], poly.loop_indices[1]))
                if len(m.uv_layers) > 0:
                    self.write(file, "U")
                    file.write(struct.pack("<B", len(m.uv_layers)))
                    for uvl in m.uv_layers[0].data:
                        file.write(struct.pack("<ff", uvl.uv[0], uvl.uv[1]))
                    if len(m.uv_layers) > 1:
                        for uvl in m.uv_layers[1].data:
                            file.write(struct.pack("<ff", uvl.uv[0], uvl.uv[1]))
                #G groupSz1 [groupName NULL] (for each vert)[groupSz1 [groupId1 groupWeight4]]
                self.write(file, "G")
                file.write(struct.pack("<B", len(obj.vertex_groups)))
                for grp in obj.vertex_groups:
                    self.write(file, grp.name + "\x00")
                for loop in m.loops:
                    vert = m.vertices[loop.vertex_index]
                    file.write(struct.pack("<B", len(vert.groups)))
                    for grp in vert.groups:
                        file.write(struct.pack("<Bf", grp.group, grp.weight))
                
                #if obj.type == 'MESH' and m.shape_keys:
                #    for block in m.shape_keys.key_blocks:
                #        self.write(file, "    shp " + block.name + "\r\n")
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
                    mats[f-fr0].append([mat.to_translation(), mat.to_quaternion(), mat.to_scale()]);
                    i += 1
            
            print ("!writing to: " + path + action.name + ".animclip")
            file = open(path + action.name + ".animclip", "wb")
            self.write(file, "ANIM")
            file.write(struct.pack("<H", len(abones)*3)) #TRS
            file.write(struct.pack("<HH", fr0, fr1))
            
            arm.animation_data.action = action
            
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