#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include "maddy/parser.h"

int main( int argc, char** argv )
{
    if ( argc < 3 ) {
        std::cout << "Usage maddy input.md output.html" << std::endl;
        return 1;
    }
    std::ifstream markdownInput{ argv[ 1 ], std::ifstream::in };

    if ( !markdownInput ) {
        std::cerr << "Failed to open " << argv[ 1 ] << std::endl;
        return 1;
    }

    std::stringstream markdown;
    markdown << markdownInput.rdbuf();
    markdownInput.close();

    std::shared_ptr<maddy::Parser> parser = std::make_shared<maddy::Parser>();
    std::string html = parser->Parse( markdown );

    std::ofstream htmlOutput{ argv[ 2 ], std::ofstream::out };
    if ( !htmlOutput ) {
        std::cerr << "Failed to open " << argv[ 2 ] << std::endl;
        return 1;
    }

    htmlOutput << html;
    htmlOutput.close();
    return 0;
}
