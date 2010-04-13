# Add header/footer around the Markdown generated documentation
# Copied from Damian Cugley (http://www.alleged.org.uk/2005/marky/)

1   {
    h
    i\
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"\
    "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">\
<html xml:lang="en-US" lang="en-US">\
<head>
    s/h2>/title>/g
    rmetadata.inc
    a\
</head>\
<body>
    rheader.inc
}

2   {
    x
    p
    x
}

$   {
    rfooter.inc
    a\
</body>\
</html>
}

