$ErrorActionPreference = 'Stop';

$packageArgs = @{
  packageName    = 'klogg'
  unzipLocation  = "$(Split-Path -parent $MyInvocation.MyCommand.Definition)"
  fileType       = 'exe'
  url64bit       = 'https://bintray.com/variar/generic/download_file?file_path=win%2Fklogg%2Fklogg-20.12.0.808-x64-setup.exe'
  checksum64     = '594c1eafb19d7465562354a981aeb01d0055609d5a1a40a5ceca93bd311a62c3'
  checksumType64 = 'sha256'
  url            = 'https://bintray.com/variar/generic/download_file?file_path=win%2Fklogg%2Fklogg-20.12.0.808-x86-setup.exe'
  checksum       = '5529b3aabbc6d06d2e2f7ed4a5ca18b754d8d9022f4d8fcc8813ada37f933878'
  checksumType   = 'sha256'
  softwareName   = 'klogg'
  silentArgs     = '/S'
  validExitCodes = @(0)
}

Install-ChocolateyPackage @packageArgs