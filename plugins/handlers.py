from PyHandler import PyHandler
import subprocess
print("dupa from python")

class glogg(PyHandler):
    def __init__(self, obj):
        PyHandler.__init__(self, obj)
        print("glogg")

    def on_trigger(self, index):
        print("on_trigger", index)

    def on_popup_menu(self):
        print("on_popup_menu")

    def on_create_menu(self):
        print("on_create_menu")

    def on_search(self, file, pattern):
        print(file, pattern)
        cmd = "egrep -n -C 5 " + pattern + " '" + file + "'  | cut -f1 -d:"
        proc = subprocess.Popen(cmd, bufsize=0, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        e = proc.communicate()[0].decode("utf8").split("\n")
        #print(e)
        #e = [1, 3, 5]
        return e

