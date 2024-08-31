// Copyright Epic Games, Inc. All Rights Reserved.

using System;
using System.IO;
using UnrealBuildTool;

public class Bristlecone : ModuleRules
{


    //This is pulled from gameinputbase. we did try to take a dependency from that, but
    // we actually don't need it and I'll be phasing it out permanently unless I can find a way to make it
    //work without building the engine from source. right now, my goal is that bristle doesn't require that.
    protected virtual bool HasGameInputSupport(ReadOnlyTargetRules Target)
    {
        // Console platforms will override this function and determine if they have GameInput support on their own.
        // For the base functionality, we can only support Game Input on windows platforms.
        if (!Target.Platform.IsInGroup(UnrealPlatformGroup.Windows))
        {
            return false;
        }

        // The Game Input SDK is installed when you install the GRDK, which will set the environment variable "GRDKLatest"
        // We can then use that to look up the path to the GameInput.h and GameInput.lib files needed to use Game Input.
        //
        // https://learn.microsoft.com/en-us/gaming/gdk/_content/gc/input/overviews/input-overview
        string GRDKLatestPath = Environment.GetEnvironmentVariable("GRDKLatest");
        if (GRDKLatestPath != null)
        {
            System.Console.WriteLine("GRDK is installed but Bristlecone is using our weird pirate version. See GDKDependency dir.");
        }
        else
        {
            System.Console.WriteLine("GRDK is NOT installed but Bristlecone uses the version in GDKDependency dir. This may cause problems.");
        }
        return true; // Bristlecone packs with the relevant stuff because I couldn't stand it anymore.
    }


    public Bristlecone(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicIncludePaths.AddRange(
            new string[] {
                Path.Combine(PluginDirectory,"Source/Bristlecone")
            }
        );
        bool bHasGameInputSupport = HasGameInputSupport(Target);
        System.Console.WriteLine("Known support: " + bHasGameInputSupport);
        string gdkpath = Path.Combine(PluginDirectory, "GDKDependency", "GameKit", "Include");
        PrivateIncludePaths.Add(gdkpath);
        PublicIncludePaths.Add(gdkpath);
        string gdklibpath = Path.Combine(PluginDirectory, "GDKDependency", "GameKit", "Lib", "amd64", "GameInput.lib");
        PublicAdditionalLibraries.Add(gdklibpath);

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
            "DeveloperSettings"
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
                "NetCommon"
            });


        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
				// ... add any modules that your module loads dynamically here ...
			}
            );
    }
}
