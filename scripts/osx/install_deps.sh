#ninja
wget -v https://github.com/ninja-build/ninja/releases/download/v1.10.1/ninja-mac.zip -O ninja-mac.zip
unzip ninja-mac.zip

#qt 5.12
brew tap-new variar/qt
brew extract --version 5.12.3 qt variar/qt
brew install variar/qt/qt@5.12.3
brew link qt --force
