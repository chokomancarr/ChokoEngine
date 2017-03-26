import bpy
#import mathutils
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
            if obj.type != 'MESH':
                continue
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
            obj.modifiers.new("tria", 'TRIANGULATE')
            m = obj.to_mesh(bpy.context.scene, True, 'PREVIEW') #struct.pack("<i", len(m.loops))
            self.write(file, "KTO123" + obj.name + "\x00V") #V size4 [vert4+norm4 array] F size4 [3xface4 array] NULL
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
            self.write(file, "\x00")
            #if obj.type == 'MESH' and m.shape_keys:
            #    for block in m.shape_keys.key_blocks:
            #        self.write(file, "    shp " + block.name + "\r\n")
            file.close()
        file2.close()

    def write (self, file, _str):
        file.write(_str.encode())

if __name__ == "__main__":
    #print("----- start " + datetime.datetime.now().strftime("%H:%M:%S") + " -----")
    KTMExporter().execute()
    #print("----- end   " + datetime.datetime.now().strftime("%H:%M:%S") + " -----")