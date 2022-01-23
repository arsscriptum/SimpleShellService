	[CmdletBinding()]
		param(
			[Parameter(Mandatory=$True,ValueFromPipeline=$True,Position=0)]
	        [ValidateScript({
	            if(-Not ($_ | Test-Path) ){
	                throw "File or folder does not exist"
	            }
	            if(-Not ($_ | Test-Path -PathType Leaf) ){
	                throw "The Path argument must be a file. Directory paths are not allowed."
	            }
	            return $true 
	        })]
	        [Alias('p')]
			[String]$Path
		)



Function script:Register-Svc {
	[CmdletBinding()]
		param(
			[Parameter(Mandatory=$True,ValueFromPipeline=$True,Position=0)]
	        [ValidateScript({
	            if(-Not ($_ | Test-Path) ){
	                throw "File or folder does not exist"
	            }
	            if(-Not ($_ | Test-Path -PathType Leaf) ){
	                throw "The Path argument must be a file. Directory paths are not allowed."
	            }
	            return $true 
	        })]
	        [Alias('p')]
			[String]$BinPath,
			[Parameter(Mandatory=$false,ValueFromPipeline=$True,Position=1)]
			[Alias('n','name')]
			[String]$ServiceName = "_socketservice"

		)
		Write-Host '[RegisterSvc] ' -f DarkRed -NoNewLine ; Write-Host "Stop-Service $ServiceName" -f Yellow           
		Stop-Service $ServiceName -ErrorAction Ignore
		Write-Host '[RegisterSvc] ' -f DarkRed -NoNewLine ; Write-Host "Remove-Service $ServiceName" -f Yellow           
		Remove-Service $ServiceName -ErrorAction Ignore

		$BinPath = (Resolve-Path -Path $BinPath).Path
		$BinCertProperties = Get-AuthenticodeSignature $BinPath
		Write-Host '[RegisterSvc] ' -f DarkRed -NoNewLine ; Write-Host "BinPath : $BinPath" -f Yellow            
		If ($BinCertProperties.Status -Notlike "Valid") {
			$Confirm = $True
			Write-Host '[RegisterSvc] ' -f DarkRed -NoNewLine ; Write-Host "$ServiceName is not signed with a valid certificate" -f Yellow         
		}
		
		Write-Host '[RegisterSvc] ' -f DarkRed -NoNewLine ; Write-Host "New-Service -Name $ServiceName -BinaryPathName $BinPath" -f Yellow         
		$Result = New-Service -Name $ServiceName -BinaryPathName $BinPath -DisplayName $ServiceName -Description "Asynchronous socket listening on TCP/27015" -StartupType Automatic -Verbose
		
		If (!(Test-Path C:\Temp)){
			New-Item -ItemType Directory -Path C:\ -Name Temp | Out-Null #Make the log path for the service log
		}

		Write-Host '[RegisterSvc] ' -f DarkRed -NoNewLine ; Write-Host "Start-Service -Name $ServiceName" -f Yellow         
		Start-Service $ServiceName

		$Result = Get-Service $ServiceName

		$status = $Result.Status
		$Name = $Result.Name
		$DisplayName = $Result.DisplayName
		Write-Host '[RegisterSvc] ' -f DarkRed -NoNewLine ; Write-Host "$ServiceName status $status" -f Yellow       
		Write-Host '[RegisterSvc] ' -f DarkRed -NoNewLine ; Write-Host "$ServiceName DisplayName $DisplayName" -f Yellow           
}
Write-Host '[RegisterSvc] ' -f DarkRed -NoNewLine ; Write-Host "Path is $Path" -f Yellow     
if(-not(Test-Path $Path)){
	Write-Host '[RegisterSvc] ' -f DarkRed -NoNewLine ; Write-Host "INVALID PATH" -f Yellow     
	return 0
}
. Register-Svc -BinPath $Path