# Build HTML documentation (if 'markdown' is available)
system(type markdown >/dev/null) {
    MARKDOWN += doc/documentation.markdown
}
else {
    message("markdown not found, HTML doc will not be generated")
}

doc_processor.name = markdown
doc_processor.input = MARKDOWN
doc_processor.output = doc/${QMAKE_FILE_BASE}.html
doc_processor.commands = markdown ${QMAKE_FILE_NAME} | \
    sed -f finish.sed >${QMAKE_FILE_OUT}

doc_processor.CONFIG += target_predeps
doc_processor.variable_out = doc.files

QMAKE_EXTRA_COMPILERS += doc_processor
