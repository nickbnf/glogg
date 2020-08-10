from PyHandler import PyHandler
print("dupa from python2")

class glogg2(PyHandler):
    def __init__(self, obj):
        PyHandler.__init__(self, obj)
        print("glogg2")
