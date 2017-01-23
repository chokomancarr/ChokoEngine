import bpy
#import mathutils
import os
import sys

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
        self.write(file2, "KTM123\r\n")
        for obj in self.scene.objects:
            if obj.type != 'MESH':
                continue
            print ("obj " + obj.name)
            self.write(file2, "obj " + obj.name)
            if obj.parent:
                self.write(file2, " \x00prt " + obj.parent.name + "\r\n")
            else:
                self.write(file2, "\r\n")
                
            print ("!writing to: " + dirr + name + "_blend\\" + obj.name + ".mesh.meta")
            file = open(dirr + name + "_blend\\" + obj.name + ".mesh.meta", "wb")
            self.write(file, "KTO123\r\n")
            
            self.write(file, "  obj " + obj.name + " [\r\n")
            poss = obj.location
            self.write(file, "    pos {:f} {:f} {:f}\r\n".format(poss[0], poss[1], poss[2]))
            rott = obj.rotation_euler
            self.write(file, "    rot {:f} {:f} {:f}\r\n".format(rott[0], rott[1], rott[2]))
            scll = obj.scale
            self.write(file, "    scl {:f} {:f} {:f}\r\n\r\n".format(scll[0], scll[1], scll[2]))
            for vert in obj.data.vertices:
                self.write(file, "    vrt {} {:f} {:f} {:f}\r\n".format(vert.index, vert.co[0], vert.co[1], vert.co[2]))
            self.write(file, "\r\n")
            for poly in obj.data.polygons:
                self.write(file, "    tri ")
                for loop_index in poly.loop_indices:
                    self.write(file, " {}".format(obj.data.loops[loop_index].vertex_index))
                self.write(file, "\r\n")
            if obj.type == 'MESH' and obj.data.shape_keys:
                for block in obj.data.shape_keys.key_blocks:
                    self.write(file, "    shp " + block.name + "\r\n")
            self.write(file, "\r\n  ]\r\n")
            file.close()
        file2.close()

    def write (self, file, _str):
        file.write(_str.encode())

if __name__ == "__main__":
    #print("----- start " + datetime.datetime.now().strftime("%H:%M:%S") + " -----")
    KTMExporter().execute()
    #print("----- end   " + datetime.datetime.now().strftime("%H:%M:%S") + " -----")