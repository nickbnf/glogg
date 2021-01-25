from PyHandler import PyHandler
import subprocess
import PyDialog
#print("dupa from python")

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

#    def on_display_line(self, line):
#        print("on_display_line", line)
#        # cmd = "echo " + line + " | cut -f1 -d' '"
#        # proc = subprocess.Popen(cmd, bufsize=0, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
#        # ret = proc.communicate()[0].decode("utf8").split("\n")[0]
#        ret = line.split()
#        ret = ret[0:2] + ret[4:]
#        #print(ret)
#        return " ".join(ret)


class UI(PyHandler):
    def __init__(self, obj):
        PyHandler.__init__(self, obj)
        print("UI")
        self.myapp = PyDialog.MyForm(self)
        self.myapp.show()

    # def on_popup_menu(self):
    #     print("on_popup_menu UI")
    #     #myapp.show()
    #     print(self.myapp.getValue())

    def value(self, x):
        if x.isdigit():
            return int(x)
        else:
            return None

# filters displayed lines by parsing input ranges in python syntax ( 0:1 :5 7:)
    def create_slice_by_string(self, line):
        all = line.split()
        ret = []
        col_list = self.myapp.getColumns().split()

        print(col_list)
        if len(col_list) > 0:
            for col_range in col_list:
                print(col_range)
                r = col_range.split(':')
                ret += all[self.value(r[0]):self.value(r[1])]
                print(r, len(r), ret)
        else:
            ret = all

        return " ".join(ret)

# filters displayed lines input with number of columns tha should not be displayed, 1 based (1 5 8).
# There is possible to remove columns counting from last coulumn, by using "-", so (-1 -2) removes last and
# second column form the end
    def filter_by_index(self, line):
        columns = line.split()

        remove_col_list = self.myapp.getColumns().split()
        remove_col_list_all = [int(x) for x in remove_col_list if x.lstrip('-').isdigit() != 0]

        remove_col_list = [x for x in remove_col_list_all if x > 0]
        remove_col_list += [len(columns) + x for x in remove_col_list_all if x < 0]

        ret = []
        for col in range(1, len(columns)):
            if not col in remove_col_list:
                ret.append(columns[col])

        return " ".join(ret)


    def filter_by_regex(self, line):
        return line

# filters displayed lines using shell execution with cut as parser
    def filter_by_shell(self, line):
        cmd = "echo " + line + " | cut -f1 -d' '"
        proc = subprocess.Popen(cmd, bufsize=0, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        ret = proc.communicate()[0].decode("utf8").split("\n")[0]

        return " ".join(ret)

# handler that executes chosen method of filtering displayed data
# input/output line to be displayed
#    def on_display_line(self, line):
#        #print("on_display_line", line)

#        return self.filter_by_index(line)

# handler to release resources, C++ destructoror like. Release UI here
    def on_release(self):
        print("on_release")
        #self.myapp.show()
        self.myapp.close()
        self.myapp = None

# handler to show UI, when was hidden. Used by the Toolbar.
    def on_show_ui(self):
        print("on_show_ui")
        self.myapp.show()

# handler that executes egrep using pattern from main APP, with possibility to exetend it by passing egrep features,
# context for example: -C 5, -A 3, -B 7
# input file to be grepped, search pattern from main APP
# output list of indexes of lines to be displayed
    def on_search(self, file, pattern, initialLine):
        print("on_search beg")
        print(file, pattern)

        offset = ""

        if initialLine:
            offset = "+"+str(initialLine)

        cmd = "head " + offset + " -n 5000 '" + file + "' | egrep -n -i " + self.myapp.getValue() + " \"" + pattern + "\" | cut -f1 -d:"
        print(cmd)
        print("\n\n",cmd,"\n\n")
        proc = subprocess.Popen(cmd, bufsize=0, shell=True, stdout=subprocess.PIPE)
        ret = proc.communicate()[0].decode("utf8").split("\n")

        print("on_search end")
        return ret
