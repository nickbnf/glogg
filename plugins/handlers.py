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
        print("on_search beg")
        print(file, pattern)
        cmd = "egrep -n -i " + pattern + " '" + file + "'  | cut -f1 -d:"
        proc = subprocess.Popen(cmd, bufsize=0, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        ret = proc.communicate()[0].decode("utf8").split("\n")

        cmd = "egrep -n -i " + pattern + " '" + file + "'"
        proc = subprocess.Popen(cmd, bufsize=0, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        e = proc.communicate()[0].decode("utf8").split("\n")
        print(len(e), e)
        #e = [1, 3, 5]
        print("on_search end")
        return ret

    def on_display_line(self, line):
        print("on_display_line", line)
        # cmd = "echo " + line + " | cut -f1 -d' '"
        # proc = subprocess.Popen(cmd, bufsize=0, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        # ret = proc.communicate()[0].decode("utf8").split("\n")[0]
        ret = line.split()
        ret = ret[0:2] + ret[4:]
        #print(ret)
        return " ".join(ret)
