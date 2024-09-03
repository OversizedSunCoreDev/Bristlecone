// Copyright Epic Games, Inc. All Rights Reserved.

using System;
using System.IO;
using UnrealBuildTool;

public class Bristlecone : ModuleRules
{

    public Bristlecone(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicIncludePaths.AddRange(
            new string[] {
                Path.Combine(PluginDirectory,"Source/Bristlecone")
            }
        );

        //may also need to add an explicit runtime dependency.
        // Get the engine path. Ends with "Engine/"
        string engine_path = EngineDirectory;
        // Now get the base of UE's modules dir (could also be Developer, Editor, ThirdParty)
        string src_path = engine_path + "\\Source\\Runtime\\";

        //Don't do this. We need it to avoid having to either patch the engine or rebuild most of sockets or use pointer arithmatic and void*
        PrivateIncludePaths.Add(src_path + "Sockets\\Private\\BSDSockets\\");
        PrivateIncludePaths.Add(src_path + "Sockets\\Private\\");
        PublicAdditionalLibraries.Add("qwave.lib"); // this will need to be fixed. god.


        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine",
            "Slate",
            "ApplicationCore",
            "InputCore",
            "Networking",
            "Sockets",
            "Settings",
            "DeveloperSettings",
            "Cabling"
        });

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "ApplicationCore",
                "Engine",
                "Slate",
                "SlateCore",
                "Engine",
                "InputCore",
                "Networking",
                "Sockets",
                "Settings",
                "DeveloperSettings",
                "NetCommon",
            "Cabling"
            });


        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
				// ... add any modules that your module loads dynamically here ...
			}
            );
    }
}
