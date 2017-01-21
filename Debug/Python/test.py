import bpy

class Foo():
    def execute(self):
        print("!Hello from Python!")
        return

if __name__ == "__main__":
    Foo().execute()