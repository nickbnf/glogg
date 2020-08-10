from PyHandler import PyHandler
print("dupa from python")

class glogg(PyHandler):
    def __init__(self, obj):
        PyHandler.__init__(self, obj)
        print("glogg")

    def on_trigger(self, index):
        print("on_trigger", index)

    def on_popup_menu(self):
        print("on_popup_menu")

