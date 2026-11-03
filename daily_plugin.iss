; ============================================================
;  DailyPlugin - Inno Setup installer (.iss)
;  Build with:  ISCC.exe daily_plugin.iss
;  dp.exe is self-contained (templates embedded), so only the
;  executable needs installing.
; ============================================================

#ifndef MyAppVersion
  #define MyAppVersion "0.3.0"
#endif
#ifndef MyAppName
  #define MyAppName "DailyPlugin"
#endif
#ifndef MyAppExeDir
  #define MyAppExeDir "dist"
#endif

[Setup]
AppId={{8C9A5B4E-6E11-4B9D-8A1F-DailyPlugin}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher=Owlbyte
DefaultDirName={autopf}\DailyPlugin
DefaultGroupName=DailyPlugin
UninstallDisplayIcon={app}\dp.exe
Compression=lzma2
SolidCompression=yes
OutputDir=release
OutputBaseFilename=DailyPlugin-Setup-{#MyAppVersion}
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=lowest
WizardStyle=modern

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "chinesesimplified"; MessagesFile: "compiler:Languages\ChineseSimplified.isl"

[Tasks]
Name: "addpath"; Description: "Add DailyPlugin to your PATH (allows running 'dp' anywhere)"; Flags: checkedonce

[Files]
Source: "{#MyAppExeDir}\dp.exe"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\DailyPlugin"; Filename: "{app}\dp.exe"
Name: "{group}\Uninstall DailyPlugin"; Filename: "{uninstallexe}"

[Registry]
; Current-user PATH entry (machine PATH would need admin; keep per-user for PrivilegesRequired=lowest)
Root: HKCU; Subkey: "Environment"; ValueType: expandsz; ValueName: "Path"; ValueData: "{olddata};{app}"; Check: NeedsAddPath; Tasks: addpath; Flags: preservestringtype

[Code]
function NeedsAddPath(): boolean;
var
  OrigPath: string;
begin
  if not RegQueryStringValue(HKEY_CURRENT_USER, 'Environment', 'Path', OrigPath) then
  begin
    Result := True;
    exit;
  end;
  Result := (Pos(';' + ExpandConstant('{app}') + ';', ';' + OrigPath + ';') = 0);
end;