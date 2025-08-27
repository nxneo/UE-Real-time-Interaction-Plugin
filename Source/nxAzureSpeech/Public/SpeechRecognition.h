// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SpeechRecognition.generated.h" // 保持为最后一个#include文件


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSpeechRecognitionResultDelegate, const FString&, RecognizedText);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSpeechRecognizingDelegate, const FString&, RecognizingText);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAudio2FaceResponse, const FString&, ResponseContent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAudio2FaceRequestFailed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTextSynthesisComplete, const FString&, FileName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAudioDurationReceived, float, AudioDuration);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSpeechKeywordDetectedDelegate, const FString&, Keyword);
// 新增的委托用于显示购票二维码
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSpeechRecognitionShowDelegate, const FString&, ShowContent);
// 新增的委托用于显示网络错误
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSpeechRecognitionNetDelegate, const FString&, ShowContent_NET);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
//class NXAZURESPEECH_API USpeechRecognition : public UBlueprintAsyncActionBase
class NXAZURESPEECH_API USpeechRecognition : public UActorComponent
{
    GENERATED_BODY()
public:
    USpeechRecognition();
    // 启动连续语音识别的功能
    UFUNCTION(BlueprintCallable, Category = "Azure Speech")
    void StartContinuousSpeechRecognition();

    // 启动单次语音识别的功能
    UFUNCTION(BlueprintCallable, Category = "Azure Speech")
    void StartOnceSpeechRecognition();

    // 停止单次语音识别的功能
    UFUNCTION(BlueprintCallable, Category = "Azure Speech")
    void StopOnceSpeechRecognition();

    // 停止连续语音识别的功能
    UFUNCTION(BlueprintCallable, Category = "Azure Speech")
    void StopContinuousSpeechRecognition();


    // 用于(单次共用,连续不用)在蓝图中通知结果的委托
    UPROPERTY(BlueprintAssignable, Category = "Azure Speech")
    FSpeechRecognitionResultDelegate OnSpeechRecognitionResult;

    // 新的委托用于Recognizing事件
    UPROPERTY(BlueprintAssignable, Category = "Azure Speech")
    FSpeechRecognizingDelegate OnSpeechRecognizing;

    // 新的委托用于关键词检测
    UPROPERTY(BlueprintAssignable, Category = "Azure Speech")
    FSpeechKeywordDetectedDelegate OnSpeechKeywordDetected;

    // 新的委托用于显示购票二维码(后续可以扩展其它显示)
    UPROPERTY(BlueprintAssignable, Category = "Azure Speech")
    FOnSpeechRecognitionShowDelegate OnSpeechRecognitionShow;

    // 新的委托用于显示网络错误
    UPROPERTY(BlueprintAssignable, Category = "Azure Speech")
    FOnSpeechRecognitionNetDelegate OnSpeechRecognitionNet;

    //文本合成语音文件
    UFUNCTION(BlueprintCallable, Category = "Azure Speech")
    void TextSynthesizeWav(const FString& TextToSynthesize, const FString& FileName);

    UFUNCTION(BlueprintCallable, Category = "Azure Speech")
    void TextSynthesizeWav_0(const FString& TextToSynthesize, const FString& FileName);

    UPROPERTY(BlueprintAssignable, Category = "Speech Recognition")
    FOnTextSynthesisComplete OnTextSynthesisComplete;


    //单次识别初始化（蓝图中可能会循环调用单次识别，这里做一次性初始化）
    UFUNCTION(BlueprintCallable, Category = "Azure Speech")
    void InitializeSpeechRecognizer();

    //文件合成wav初始化（蓝图中会比较频繁生成wav文件给audio2face，这里做一次性初始化）
    UFUNCTION(BlueprintCallable, Category = "Azure Speech")
    void InitializeTextSynthesizeWav();


    UFUNCTION(BlueprintCallable, Category = "Audio2Face")
    void Audio2Face();

    UFUNCTION(BlueprintCallable, Category = "Audio2Face")
    void A2FPause();

    UPROPERTY(BlueprintAssignable, Category = "Audio2Face")
    FOnAudio2FaceResponse OnAudio2FaceResponse;

    UPROPERTY(BlueprintAssignable, Category = "Audio2Face")
    FOnAudio2FaceResponse OnAudio2FaceResponse_0;

    UPROPERTY(BlueprintAssignable, Category = "Audio2Face")
    FOnAudioDurationReceived OnAudioDurationReceived;

    UPROPERTY(BlueprintAssignable, Category = "Audio2Face")
    FOnAudio2FaceRequestFailed OnAudio2FaceRequestFailed;

    UFUNCTION(BlueprintCallable, Category = "Azure Speech")
    void SetA2FTrack(const FString& FileName);

    UFUNCTION(BlueprintCallable, Category = "Azure Speech")
    void GetA2FRange();

    UFUNCTION()
    void OnSpeechRecognitionResultHandler(const FString& Result);

private:
    float ParseAudioDurationInfo(const FString& ResponseContent);

};
