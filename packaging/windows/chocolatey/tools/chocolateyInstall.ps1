$ErrorActionPreference = 'Stop';

$packageArgs = @{
  packageName    = 'klogg'
  unzipLocation  = "$(Split-Path -parent $MyInvocation.MyCommand.Definition)"
  fileType       = 'exe'
  url64bit       = 'https://github.com/variar/klogg/releases/download/v20.12/klogg-20.12.0.813-x64-setup.exe'
  checksum64     = 'b51762fa09c9646172dd9efcfb985dad9376548deda6e513dc08f0a558e075ca'
  checksumType64 = 'sha256'
  url            = 'https://github.com/variar/klogg/releases/download/v20.12/klogg-20.12.0.813-x86-setup.exe'
  checksum       = 'f98d57e9a03e34935c3f8465b6244d7ec740e8cd498cbdb06577eab693a2bae1'
  checksumType   = 'sha256'
  softwareName   = 'klogg'
  silentArgs     = '/S'
  validExitCodes = @(0)
}

Install-ChocolateyPackage @packageArgs