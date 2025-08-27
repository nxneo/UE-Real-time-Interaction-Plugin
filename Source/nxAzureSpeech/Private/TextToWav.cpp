// Fill out your copyright notice in the Description page of Project Settings.

#include "TextToWav.h"
#include <speechapi_cxx.h>

using namespace Microsoft::CognitiveServices::Speech;
using namespace Microsoft::CognitiveServices::Speech::Audio;

// Sets default values for this component's properties
UTextToWav::UTextToWav()
{
    // Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
    // off to improve performance if you don't need them.
    PrimaryComponentTick.bCanEverTick = true;

    // ...
}

void UTextToWav::SynthesizeSpeech()
{
    // 从Azure获取的订阅密钥和服务区域
    const FString SubscriptionKey = "a3e637f6a4fa4fe2a1fc1e081ee5994d";
    const FString ServiceRegion = "eastus";
    const FString OutputFilePath = "D:/audio2faceFiles/file_wav.wav";

    // 创建 SpeechConfig 对象
    auto speechConfig = SpeechConfig::FromSubscription(TCHAR_TO_UTF8(*SubscriptionKey), TCHAR_TO_UTF8(*ServiceRegion));

    // 创建 AudioConfig 对象，指定输出到.wav文件
    auto audioConfig = AudioConfig::FromWavFileOutput(TCHAR_TO_UTF8(*OutputFilePath));

    // 实例化 SpeechSynthesizer
    auto speechSynthesizer = SpeechSynthesizer::FromConfig(speechConfig, audioConfig);

    // 
    auto result = speechSynthesizer->SpeakTextAsync(TCHAR_TO_UTF8("测试...")).get();

    // 
    if (result->Reason == ResultReason::SynthesizingAudioCompleted)
    {
        UE_LOG(LogTemp, Warning, TEXT("测试..."));
    }
    else
    {
        //UE_LOG(LogTemp, Warning, TEXT("测试 Reason: %s"), *FString::Printf(TEXT("%s"), *result->Reason.ToString()));
    }
}

