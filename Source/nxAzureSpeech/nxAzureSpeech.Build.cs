// Copyright Epic Games, Inc. All Rights Reserved.
using System.IO;
using UnrealBuildTool;

public class nxAzureSpeech : ModuleRules
{
    public nxAzureSpeech(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp17;

        // 启用 C++ 异常处理,这个很关键，否则出在打包的时候出现EHsc相关的提示，导致不能打包
        bEnableExceptions = true;

        foreach (var dllPath in Directory.GetFiles(Path.Combine(ModuleDirectory, "SpeechRecSdk", "runtimes"), "*.dll"))
        {
            RuntimeDependencies.Add(dllPath);
            System.IO.File.WriteAllText(Path.Combine(ModuleDirectory, "LogOutput.txt"), "DLL path: " + dllPath);
        }


        // 添加 include 路径
        PublicIncludePaths.AddRange(
            new string[] {
                Path.Combine(ModuleDirectory, "SpeechRecSdk", "build", "include", "cxx_api"),
                Path.Combine(ModuleDirectory, "SpeechRecSdk", "build", "include", "c_api")
            }
        );
        // 添加需要链接的库
        PublicAdditionalLibraries.AddRange(
            new string[] {
                Path.Combine(ModuleDirectory, "SpeechRecSdk", "build", "x64", "Release", "Microsoft.CognitiveServices.Speech.core.lib"),
            }
        );

        // 复制DLL文件到运行时依赖项,这个可以保证打包后正常
        string RuntimeDllPath = Path.Combine(ModuleDirectory, "SpeechRecSdk", "runtimes");

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            string BinariesPath = Path.Combine(Target.ProjectFile.Directory.ToString(), "Binaries", "Win64");
            foreach (var dllPath in Directory.GetFiles(RuntimeDllPath, "*.dll"))
            {
                string dllName = Path.GetFileName(dllPath);
                string destPath = Path.Combine(BinariesPath, dllName);

                // 添加到运行时依赖项
                RuntimeDependencies.Add(destPath, dllPath);

                // 记录日志
                System.IO.File.AppendAllText(Path.Combine(ModuleDirectory, "LogOutput.txt"), "DLL path: " + dllPath + " -> " + destPath + "\n");
            }
        }

        // 在 UWP 平台下添加 C# DLL 依赖项
        if (Target.Platform == UnrealTargetPlatform.Win64 && Target.Configuration == UnrealTargetConfiguration.Shipping)
        {
            string UwpDllPath = Path.Combine(ModuleDirectory, "SpeechRecSdk", "lib");  // 这里是 C# DLL 所在的目录
            RuntimeDependencies.Add(Path.Combine(UwpDllPath, "Microsoft.CognitiveServices.Speech.csharp.dll"));
        }




        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "nxAzureSpeech"
				// ... add other public dependencies that you statically link with here ...
			}
            );


        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "InputCore",
                "HTTP",
                "Json",
				// ... add private dependencies that you statically link with here ...	
			}
            );


        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
				// ... add any modules that your module loads dynamically here ...
			}
            );
    }
}
