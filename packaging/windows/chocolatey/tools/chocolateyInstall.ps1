$ErrorActionPreference = 'Stop';

$packageArgs = @{
  packageName    = 'klogg'
  unzipLocation  = "$(Split-Path -parent $MyInvocation.MyCommand.Definition)"
  fileType       = 'exe'
  url64bit       = 'https://bintray.com/variar/generic/download_file?file_path=win%2Fklogg%2Fklogg-19.02.0.366-x64-setup.exe'
  checksum64     = '5fbd02e5a673a07dbf5880e5a7aa685982b01fbcb399a59334f27c5efc6ff66f'
  checksumType64 = 'sha256'
  url            = 'https://bintray.com/variar/generic/download_file?file_path=win%2Fklogg%2Fklogg-19.02.0.366-x86-setup.exe'
  checksum       = '9547c6235c25d008115398f79a57726951b4dba554490846ce508f0e47bf2aa6'
  checksumType   = 'sha256'
  softwareName   = 'klogg'
  silentArgs     = '/S'
  validExitCodes = @(0)
}

Install-ChocolateyPackage @packageArgs