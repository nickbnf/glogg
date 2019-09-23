$ErrorActionPreference = 'Stop';

$packageArgs = @{
  packageName    = 'klogg'
  unzipLocation  = "$(Split-Path -parent $MyInvocation.MyCommand.Definition)"
  fileType       = 'exe'
  url64bit       = 'https://bintray.com/variar/generic/download_file?file_path=win%2Fklogg%2Fklogg-19.09.0.467-x64-setup.exe'
  checksum64     = '3a304737fe872d60693b927c67cd2fdf67cf720ef056e20f6c2fbb7c7f539df4'
  checksumType64 = 'sha256'
  url            = 'https://bintray.com/variar/generic/download_file?file_path=win%2Fklogg%2Fklogg-19.09.0.467-x86-setup.exe'
  checksum       = '89b916c3e64f59879e66908a2ada4301ad4371d2d008399dc39a8fcaffa60710'
  checksumType   = 'sha256'
  softwareName   = 'klogg'
  silentArgs     = '/S'
  validExitCodes = @(0)
}

Install-ChocolateyPackage @packageArgs