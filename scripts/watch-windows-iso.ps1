param(
    [string]$Downloads = "$env:USERPROFILE\Downloads",
    [string]$VmDir = "D:\code\easy-vulkan-qemu\vm\windows-vulkan",
    [string]$Qemu = "D:\code\qemu\build-windows-rutabaga\qemu-system-x86_64.exe",
    [string]$BiosDir = "D:\code\qemu\pc-bios",
    [int]$PollSeconds = 60
)

$ErrorActionPreference = "Stop"
$logPath = Join-Path $VmDir "install-watcher.log"
$destinationIso = Join-Path $VmDir "windows11-zh-cn.iso"
$disk = Join-Path $VmDir "windows.qcow2"
$watchStarted = Get-Date

New-Item -ItemType Directory -Force -Path $VmDir | Out-Null
function Log([string]$Message) {
    $line = "{0:u} {1}" -f (Get-Date), $Message
    Add-Content -Path $logPath -Value $line
}

Log "等待微软官方 Windows ISO 下载完成。"
while (-not (Test-Path $destinationIso)) {
    $iso = Get-ChildItem -Path $Downloads -File -ErrorAction SilentlyContinue |
        Where-Object {
            $_.Length -gt 4GB -and
            $_.Extension -ne ".crdownload" -and
            $_.Extension -ne ".part" -and
            $_.LastWriteTime -ge $watchStarted.AddMinutes(-1)
        } |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1
    if ($iso) {
        Log "发现完成的 ISO：$($iso.FullName)，大小 $([math]::Round($iso.Length / 1GB, 2)) GiB。"
        Copy-Item -LiteralPath $iso.FullName -Destination $destinationIso -Force
        break
    }
    Start-Sleep -Seconds $PollSeconds
}

if (-not (Test-Path $disk)) {
    & "C:\msys64\ucrt64\bin\qemu-img.exe" create -f qcow2 $disk 64G | Out-Null
}

$uefiCode = Join-Path $VmDir "edk2-x86_64-code.fd"
$uefiSource = Join-Path $BiosDir "edk2-x86_64-code.fd.bz2"
if (-not (Test-Path $uefiCode) -and (Test-Path $uefiSource)) {
    & "C:\msys64\ucrt64\bin\bunzip2.exe" -k -f $uefiSource
    Copy-Item -LiteralPath (Join-Path $BiosDir "edk2-x86_64-code.fd") -Destination $uefiCode -Force
}

if (-not (Test-Path $Qemu)) {
    Log "找不到 QEMU：$Qemu"
    exit 2
}

$existing = Get-Process -Name "qemu-system-x86_64" -ErrorAction SilentlyContinue |
    Where-Object { $_.Path -eq $Qemu }
if ($existing) {
    Log "检测到 QEMU 已运行，PID=$($existing.Id)，不重复启动。"
    exit 0
}

$arguments = @(
    "-L", $BiosDir,
    "-name", "Windows 11 Vulkan VM,process=windows-vulkan",
    "-machine", "q35,accel=whpx",
    "-cpu", "max",
    "-m", "6144",
    "-smp", "4",
    "-bios", $uefiCode,
    "-drive", "file=$disk,if=ide,format=qcow2",
    "-drive", "file=$destinationIso,media=cdrom,readonly=on",
    "-boot", "menu=on,strict=on,order=d",
    "-device", "VGA",
    "-display", "sdl,gl=off",
    "-usb",
    "-device", "usb-tablet",
    "-netdev", "user,id=net0",
    "-device", "e1000,netdev=net0"
)

$commandPath = Join-Path $VmDir "install-command.txt"
Set-Content -Path $commandPath -Value (($arguments | ForEach-Object { '"' + $_ + '"' }) -join " ")
$process = Start-Process -FilePath $Qemu -ArgumentList $arguments -WorkingDirectory (Split-Path $Qemu) -PassThru
Set-Content -Path (Join-Path $VmDir "install.pid") -Value $process.Id
Log "已启动 Windows 安装 QEMU，PID=$($process.Id)。"

