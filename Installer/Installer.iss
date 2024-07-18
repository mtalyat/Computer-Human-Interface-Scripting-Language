; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

#define MyAppName "CHISL"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "Mitchell Talyat"
#define MyAppURL "https://github.com/mtalyat/Computer-Human-Interface-Scripting-Language"
#define MyAppExeName "chisl.exe"
#define MyAppAssocName "CHISL Script File"
#define MyAppAssocExt ".chisl"
#define MyAppAssocKey StringChange(MyAppAssocName, " ", "") + MyAppAssocExt

[Setup]
; NOTE: The value of AppId uniquely identifies this application. Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{0511F00C-DD99-4264-B4A8-285DF26A57D0}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
;AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DisableDirPage=yes
; "ArchitecturesAllowed=x64compatible" specifies that Setup cannot run
; on anything but x64 and Windows 11 on Arm.
ArchitecturesAllowed=x64compatible
; "ArchitecturesInstallIn64BitMode=x64compatible" requests that the
; install be done in "64-bit mode" on x64 or Windows 11 on Arm,
; meaning it should use the native 64-bit Program Files directory and
; the 64-bit view of the registry.
ArchitecturesInstallIn64BitMode=x64compatible
ChangesAssociations=yes
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
; Uncomment the following line to run in non administrative install mode (install for current user only.)
;PrivilegesRequired=lowest
OutputBaseFilename=chisl_setup
SetupIconFile=C:\Users\mitch\source\repos\ComputerHumanInterface\Images\icon.ico
Compression=lzma
SolidCompression=yes
WizardStyle=modern

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "C:\Users\mitch\source\repos\ComputerHumanInterface\CHISL\x64\Release\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Users\mitch\source\repos\ComputerHumanInterface\CHISL\x64\Release\opencv_videoio_ffmpeg4100_64.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Users\mitch\source\repos\ComputerHumanInterface\CHISL\x64\Release\opencv_videoio_msmf4100_64.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Users\mitch\source\repos\ComputerHumanInterface\CHISL\x64\Release\opencv_videoio_msmf4100_64d.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Users\mitch\source\repos\ComputerHumanInterface\CHISL\x64\Release\opencv_world4100.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Users\mitch\source\repos\ComputerHumanInterface\CHISL\x64\Release\opencv_world4100d.dll"; DestDir: "{app}"; Flags: ignoreversion
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Registry]
Root: HKA; Subkey: "Software\Classes\{#MyAppAssocExt}\OpenWithProgids"; ValueType: string; ValueName: "{#MyAppAssocKey}"; ValueData: ""; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\{#MyAppAssocKey}"; ValueType: string; ValueName: ""; ValueData: "{#MyAppAssocName}"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\{#MyAppAssocKey}\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\{#MyAppExeName},0"
Root: HKA; Subkey: "Software\Classes\{#MyAppAssocKey}\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".myp"; ValueData: ""

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"

[Code]
const
  WM_SETTINGCHANGE = $1A;
  SMTO_ABORTIFHUNG = $0002;

type
  WPARAM = LongWord;
  LPARAM = LongInt;
  LRESULT = LongInt;

function SendMessageTimeout(hWnd: HWND; Msg: UINT; wParam: WPARAM; lParam: LPARAM; fuFlags: UINT; uTimeout: UINT; var lpdwResult: DWORD): LRESULT;
  external 'SendMessageTimeoutA@user32.dll stdcall';

procedure AddToPath;
var
  OldPath: string;
  NewPath: string;
  dwResult: DWORD;
begin
  if not RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SYSTEM\CurrentControlSet\Control\Session Manager\Environment', 'Path', OldPath) then
    OldPath := '';
  
  if Pos(';' + ExpandConstant('{app}'), OldPath) = 0 then begin
    if OldPath <> '' then
      NewPath := OldPath + ';' + ExpandConstant('{app}')
    else
      NewPath := ExpandConstant('{app}');
    
    RegWriteStringValue(HKEY_LOCAL_MACHINE, 'SYSTEM\CurrentControlSet\Control\Session Manager\Environment', 'Path', NewPath);
    // Notify Windows about the environment change
    SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, 0, 0, SMTO_ABORTIFHUNG, 5000, dwResult);
  end;
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
    AddToPath;
end;
