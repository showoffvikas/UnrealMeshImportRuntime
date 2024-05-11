// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;
using System;

public class MeshImport4 : ModuleRules
{
    private string ThirdPartyPath => Path.Combine(ModuleDirectory, "../../ThirdParty");

    private string DllSourceDirectory => Path.Combine(ThirdPartyPath, "Assimp", "bin", "x64");

    private void CopyFilestoBinary(ReadOnlyTargetRules target)
    {
        if (target.ProjectFile != null)
        {
            string ProjectDirectory = Path.GetDirectoryName(target.ProjectFile.ToString());
            if (ProjectDirectory != null)
            {
                string DestFileDirectory = Path.Combine(ProjectDirectory, "Binaries", "Win64");

                if (!Directory.Exists(DestFileDirectory))
                {
                    Directory.CreateDirectory(DestFileDirectory);
                }

                string[] DllFiles = Directory.GetFiles(DllSourceDirectory, "*.dll");

                foreach (string DllSourcePath in DllFiles)
                {
                    string DllFileName = Path.GetFileName(DllSourcePath);
                    string DllTargetPath = Path.Combine(DestFileDirectory, DllFileName);

                    if (File.Exists(DllSourcePath))
                    {
                        File.Copy(DllSourcePath, DllTargetPath, true);
                    }
                }
            }
        }
    }
    public MeshImport4(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "ProceduralMeshComponent", "ImageWrapper" });

        PrivateDependencyModuleNames.AddRange(new string[] { });
        PrivateDependencyModuleNames.AddRange(new string[] { "MeshDescription", "StaticMeshDescription" });



        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true

        PublicIncludePaths.Add(Path.Combine(ThirdPartyPath, "Assimp", "include"));
        PublicIncludePaths.Add(Path.Combine(ThirdPartyPath, "Assimp", "include", "assimp"));

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {

            string AssimpLibraryPath = Path.Combine(ThirdPartyPath, "Assimp", "lib", "x64", "assimp-vc143-mt.lib");
            PublicAdditionalLibraries.Add(AssimpLibraryPath);


            string pathVariable = Environment.GetEnvironmentVariable("PATH");
            pathVariable += ";" + DllSourceDirectory;
            Environment.SetEnvironmentVariable("PATH", pathVariable);

            // Delay-load the DLLs, so the project can start without them
            string[] DlLs = Directory.GetFiles(DllSourceDirectory, "*.dll");
            PublicRuntimeLibraryPaths.Add(DllSourceDirectory);

            foreach (string DllFilePath in DlLs)
            {
                PublicDelayLoadDLLs.Add(Path.GetFileName(DllFilePath));
                RuntimeDependencies.Add(Path.GetFileName(DllFilePath));
            }

            CopyFilestoBinary(Target);
        }
    }
}
