---
title: "Code sample"
date: 2018-01-28T21:48:10+01:00
anchor: "code"
weight: 20
---

```python
#!/usr/bin/python

# This script will monitor templates folder and automatically compile nunjucks html templates on change

import os
import sys
from datetime import datetime
from subprocess import call
from time import sleep

try:
    NUNJUCKS_REPO = sys.argv[1]
except IndexError:
    NUNJUCKS_REPO = os.getenv("NUNJUCKS_REPO")
try:
    TEMPLATES_DIR = sys.argv[2]
except IndexError:
    TEMPLATES_DIR = os.getenv("TEMPLATES_DIR")

FILES = {}
TIMER = 1

COLOUR_END = '\033[0m'
COLOUR_GREEN = '\033[92m'
COLOUR_BLUE = '\033[94m'


def check_templates(templates_dir=TEMPLATES_DIR):
    file_list = os.listdir(templates_dir)
    for item in file_list:
        if item.split(".")[-1] == "html":
            modified = os.path.getmtime(templates_dir + "/" + item)
            if item in FILES:
                if FILES[item] != modified:
                    FILES[item] = modified
                    precompile(templates_dir, item)
            else:
                print (COLOUR_BLUE + item + " is being watched" + COLOUR_END)
                FILES[item] = modified
                precompile(templates_dir, item)


def precompile(tempates_dir, filename):
    compiled_filename = filename.replace("html", "js")
    path_to_js = tempates_dir + "/" + compiled_filename
    path_to_html = tempates_dir + "/" + filename
    if os.path.exists(path_to_js):
        call("rm -f " + path_to_js, shell=True)
    command = NUNJUCKS_REPO + "/bin/precompile --name " + filename + " " + path_to_html
    command = command + " >> " + path_to_js
    call(command, shell=True)
    print(COLOUR_GREEN + datetime.now().strftime("%X" + " " + filename + " ...OK") + COLOUR_END)


if __name__ == '__main__':
    while True:
        check_templates()
        sleep(TIMER)
```
