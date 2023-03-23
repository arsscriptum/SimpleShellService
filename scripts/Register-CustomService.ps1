[CmdletBinding(SupportsShouldProcess)]
param()


function Write-ServiceLog {
    [CmdletBinding(SupportsShouldProcess)]
    param(
        [Parameter(Mandatory=$true, Position=0)]
        [string] $Message,
        [Parameter(Mandatory=$false)]
        [Alias('f')]
        [string]$ForegroundColor="Gray",
        [Parameter(Mandatory=$false)]
        [Alias('n')]
        [switch]$NoNewLine,
        [Parameter(Mandatory=$false)]
        [Alias('t')]
        [switch]$NoTitle,
        [Parameter(Mandatory=$false)]
        [Alias('r')]
        [switch]$Result,
        [Parameter(Mandatory=$false)]
        [Alias('e')]
        [switch]$ErrorMessage
    )
    $WriteTitle = $True
    if($NoTitle -eq $True){
        $WriteTitle = $False
    }

    if($Result -eq $True){
        $WriteTitle = $False
        $ForegroundColor="DarkGreen"
    }elseif($ErrorMessage){
        $ForegroundColor="DarkRed"
        Write-Host "[SERVICE] " -f DarkRed -NoNewLine
        Write-Host "[ERROR] " -f DarkYellow -NoNewLine
    }elseif($WriteTitle){
        Write-Host "[SERVICE] " -f DarkCyan -NoNewLine
        $LogMsg = "{0} " -f (Get-Date -UFormat "[%H:%M:%S]")
        Write-Host $LogMsg -f "DarkGray" -n
    }
    

    if($NoNewLine){
        Write-Host $Message -f $ForegroundColor -NoNewLine
    }else{
        Write-Host $Message -f $ForegroundColor
    }
}

New-Alias -Name "svclog" -Value "Write-ServiceLog" -Force -Scope Global -ErrorAction Ignore -Option AllScope


Function Register-CustomService {
    [CmdletBinding(SupportsShouldProcess)]
        param(
            [Parameter(Mandatory=$True,Position=0)]
            [ValidateScript({
                if(-Not ($_ | Test-Path) ){
                    throw "File or folder does not exist"
                }
                if(-Not ($_ | Test-Path -PathType Leaf) ){
                    throw "The Path argument must be a file. Directory paths are not allowed."
                }
                return $true 
            })] 
            [String]$Path,
            [Parameter(Mandatory=$True,Position=1)]
            [String]$Name,
            [Parameter(Mandatory=$False)]
            [String]$DisplayName,
            [Parameter(Mandatory=$false)]
            [Alias('d')]
            [String]$Description="",
            [Parameter(Mandatory=$false)]
            [Alias('s')]
            [switch]$Start,
            [Parameter(Mandatory=$false)]
            [Alias('r')]
            [switch]$Remove

        )

        if( $True -eq ([string]::IsNUllorEmpty($DisplayName)) ){
            if((([regex]::Matches($Name, "_" )).count) -ge 2){

                $s=$Name.IndexOf('_',1)
                if( $s -eq -1 ) { $s=$Name.IndexOf('_') }
                $s = $s + 1
                $i = $Name.IndexOf('_',$s+1) - 1
                
                $DisplayName = $Name.Substring($s, $i-$s+1)
            }elseif(($Name.Length) -gt 6){
                $DisplayName = $Name.Replace('_','')
                $l = $DisplayName.Length
                $DisplayName = $DisplayName.Substring(0, $l-5)
            }else{
                $DisplayName = $Name.Replace('_',' ')
                $DisplayName = $DisplayName.Trim()
            }
        }

        if( $True -eq ([string]::IsNUllorEmpty($Description)) ){
            $Description = "Service `"{0}`" created on {1}" -f $Name, (Get-Date -UFormat "%D at %HH%M")
        }
        svclog "=================================================================" -f Blue
        svclog "   Service Name is `"$Name`"" -f DarkGray
        svclog "   Display Name is `"$DisplayName`"" -f DarkGray
        svclog "   Service Path is `"$Path`"" -f DarkGray
        svclog "   Description is  `"$Description`"" -f DarkGray
        svclog "=================================================================" -f Blue

        $ErrorOccured = $False
        $Running    = $False
        $Exists     = $False
        $s          = (Get-Service "$Name" -ErrorAction Ignore)
        $Exists     = ($s -ne $Null)
        if($Exists){
            $Running = (($s.Status) -eq "Running")
        }

        try{
            if($Running){
                svclog "Stopping Service `"$Name`" ... " -n     
                
                Stop-Service $Name -ErrorAction Stop
                svclog "ok" -r 
            }

            
            if($Remove){
                Remove-Service $Name -Verbose -ErrorAction Stop  4> "$ENV:Temp\Verbose.txt"
                [string[]]$StrVerbose = Get-Content "$ENV:Temp\Verbose.txt"
                $OutVerbose = $StrVerbose[0]
                svclog "$OutVerbose" -f Red -n
                svclog " SUCCESS" -r 
            }


            $BinCertProperties = Get-AuthenticodeSignature $Path
          
            If ($BinCertProperties.Status -Notlike "Valid") {
                $Confirm = $True
                svclog "$Name is not signed with a valid certificate"    
            }
            

            $Result = New-Service -Name "$Name" -BinaryPathName "$Path" -DisplayName "$DisplayName" -Description "$Description" -StartupType Automatic -Verbose -ErrorAction Stop  4> "$ENV:Temp\Verbose.txt"
            [string[]]$StrVerbose = Get-Content "$ENV:Temp\Verbose.txt"
            $OutVerbose = $StrVerbose[0]
            svclog "$OutVerbose" -f Red -n
            svclog " SUCCESS" -r 
        

            if($Start){
                Start-Service $Name -ErrorAction Stop  4> "$ENV:Temp\Verbose.txt"
                [string[]]$StrVerbose = Get-Content "$ENV:Temp\Verbose.txt"
                $OutVerbose = $StrVerbose[0]
                svclog "$OutVerbose" -f Red -n
                svclog " SUCCESS" -r 
            }

            $Result = Get-Service $Name
        }catch{
            svclog "$_" -e 
            $ErrorOccured = $True
        }

        if($ErrorOccured -eq $True){
            return $Null
        }
        if($Result -ne $Null){
            $Result = $Result | Select Name, DisplayName, Status, Description
        }
        $Result
}

