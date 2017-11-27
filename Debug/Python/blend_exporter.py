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
        print("python start")
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
        #self.execute_anim(dirr + name + "_blend\\")
    
    def export_arm(self, file2, obj, dirr, name):
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
    def execute_anim(self, path):
        for action in bpy.data.actions:
            if len(action.fcurves) == 0:
                continue
            print ("!writing to: " + path + action.name + ".animclip")
            file = open(path + action.name + ".animclip", "wb")
            self.write(file, "ANIM")
            curr_dim = 0
            for curve in action.fcurves:
                data_range = curve.range()
                typeint = 0
                if curve.data_path == "location":
                    typeint = 240 + curr_dim
                    curr_dim += 1
                    if curr_dim == 3:
                        curr_dim = 0
                    file.write(struct.pack("<B", curr_dim))
                    self.write(file, "\x00")
                elif curve.data_path == "rotation_euler":
                    typeint = 240 + curr_dim + 3
                    curr_dim += 1
                    if curr_dim == 3:
                        curr_dim = 0
                    file.write(struct.pack("<B", curr_dim))
                    self.write(file, "\x00")
                elif curve.data_path == "scale":
                    typeint = 240 + curr_dim + 7
                    curr_dim += 1
                    if curr_dim == 3:
                        curr_dim = 0
                    file.write(struct.pack("<B", curr_dim))
                    self.write(file, "\x00")
                else:
                    spl1 = curve.data_path.split("\"")
                    if len(spl1) < 3:
                        continue;
                    if spl1[0] == "key_blocks[" and spl1[2] == "].value":
                        typeint = 32
                    elif spl1[0] == "pose.bones[":
                        if spl1[2] == "].location":
                            typeint = 16 + curr_dim
                            curr_dim += 1
                            if curr_dim == 3:
                                curr_dim = 0
                        elif spl1[2] == "].rotation_quaternion":
                            typeint = 16 + curr_dim + 3
                            curr_dim += 1
                            if curr_dim == 4:
                                curr_dim = 0
                        elif spl1[2] == "].scale":
                            typeint = 16 + curr_dim + 7
                            curr_dim += 1
                            if curr_dim == 3:
                                curr_dim = 0
                    else:
                        continue
                    file.write(struct.pack("<B", typeint))
                    self.write(file, spl1[1] + "\x00")
                
                file.write(struct.pack("<B", len(curve.keyframe_points)))
                for key in curve.keyframe_points:
                    file.write(struct.pack("<ff", key.co[0], key.co[1]))
                    file.write(struct.pack("<ffff", key.handle_left[0], key.handle_left[1], key.handle_right[0], key.handle_right[1]))
            file.close()
    
    def write (self, file, _str):
        file.write(_str.encode())

if __name__ == "__main__":
    KTMExporter().execute()