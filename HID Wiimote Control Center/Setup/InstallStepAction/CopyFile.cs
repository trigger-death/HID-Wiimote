﻿/*

Copyright (C) 2017 Julian Löhr
All rights reserved.

Filename:
	CopyFile.cs

Abstract:
	Install Task to copy some files

*/
namespace HIDWiimote.ControlCenter.Setup.InstallStepAction
{
    class CopyFile : IInstallStepAction
    {
        string FileName;
        string InstallDir;
        string InstallFilePath;

        public CopyFile(string FileName, string InstallDir)
        {
            this.FileName = FileName;
            this.InstallDir = InstallDir;

            this.InstallFilePath = System.IO.Path.Combine(InstallDir, FileName);
        }

        public bool Do()
        {
            if (System.IO.File.Exists(InstallFilePath))
            {
                return false;
            }

            System.IO.File.Copy(FileName, InstallFilePath);
            return true;
        }

        public void Undo()
        {
            System.IO.File.Delete(InstallFilePath);
        }

        public string GetExceptionMessage()
        {
            return "An error occured while copying files!";
        }
    }
}
