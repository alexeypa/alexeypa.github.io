'
' Parse command line parameters.
'

NoDownload = false
NoInstall = true
NoReboot = true

For i = 0 To Wscript.Arguments.Count - 1
    If Wscript.Arguments(i) = "/nodownload" Then
        NoDownload = true
    Elseif Wscript.Arguments(i) = "/install" Then
        NoInstall = false
        NoDownload = false
    Elseif Wscript.Arguments(i) = "/reboot" And NoInstall = false Then
        NoReboot = false
    Else
        WScript.Echo "Usage: cscript //nologo WUA_SearchDownloadInstall.vbs [/nodownload | ( /install [ /reboot ] ) ]."
        WScript.Quit
    End If
Next

'
' Look for available updates.
'

Set updateSession = CreateObject("Microsoft.Update.Session")
Set updateSearcher = updateSession.CreateupdateSearcher()

WScript.Echo "Searching for updates..."

Set searchResult = updateSearcher.Search("IsInstalled=0 and Type='Software'")

If searchResult.Updates.Count > 0 Then
    WScript.Echo "List of applicable items on the machine:"

    For I = 0 To searchResult.Updates.Count-1
        Set update = searchResult.Updates.Item(I)
        WScript.Echo I + 1 & "> " & update.Title
    Next

    WScript.Echo vbCRLF
Else
    WScript.Echo "There are no applicable updates."
    WScript.Quit
End If

'
' Download all available updates.
'

If NoDownload Then
    WScript.Echo "'/nodownload' was specified. Exiting..."
    WScript.Quit
End If

Set updatesToDownload = CreateObject("Microsoft.Update.UpdateColl")

For I = 0 to searchResult.Updates.Count-1
    Set update = searchResult.Updates.Item(I)
    updatesToDownload.Add(update)
Next

WScript.Echo "Downloading updates..."

Set downloader = updateSession.CreateUpdateDownloader() 
downloader.Updates = updatesToDownload
downloader.Download()

WScript.Echo "List of downloaded updates:"

For I = 0 To searchResult.Updates.Count-1
    Set update = searchResult.Updates.Item(I)
    If update.IsDownloaded Then
       WScript.Echo I + 1 & "> " & update.Title 
    End If
Next

WScript.Echo vbCRLF

'
' Install all downloaded updates.
'

If NoInstall Then
    WScript.Echo "Skipping installation by default. Specify '/install' to apply downloaded updates."
    WScript.Quit
End If

Set updatesToInstall = CreateObject("Microsoft.Update.UpdateColl")

For I = 0 To searchResult.Updates.Count-1
    set update = searchResult.Updates.Item(I)
    If update.IsDownloaded = true Then
       updatesToInstall.Add(update) 
    End If
Next

WScript.Echo "Installing updates..."
Set installer = updateSession.CreateUpdateInstaller()
installer.Updates = updatesToInstall
Set installationResult = installer.Install()

'Output results of install
WScript.Echo "Installation Result: " & installationResult.ResultCode 
WScript.Echo "Reboot Required: " & installationResult.RebootRequired & vbCRLF 
WScript.Echo "Listing of updates installed and individual installation results:" 

For I = 0 to updatesToInstall.Count - 1
    WScript.Echo I + 1 & "> " & _
    updatesToInstall.Item(i).Title & _
    ": " & installationResult.GetUpdateResult(i).ResultCode         
Next

If NoReboot Then
    WScript.Quit
End If

If installationResult.RebootRequired Then
    WScript.Echo "Rebooting the machine..."

    Set WSHShell = WScript.CreateObject("WScript.Shell")
    WshShell.Run "shutdown.exe -r -t 0"
End If
