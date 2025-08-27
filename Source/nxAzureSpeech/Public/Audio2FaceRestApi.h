// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Audio2FaceRestApi.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAudio2FaceResponseRest, const FString&, ResponseContent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAudio2FaceRequestFailedRest);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAudioDurationReceivedRest, float, AudioDuration);


UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class NXAZURESPEECH_API UAudio2FaceRestApi : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UAudio2FaceRestApi();

	UFUNCTION(BlueprintCallable, Category = "RestAudio2Face")
	void RestAudio2Face(bool bIsWakeUp);

	UFUNCTION(BlueprintCallable, Category = "RestAudio2Face")
	void RestA2FPause();

	UFUNCTION(BlueprintCallable, Category = "RestAudio2Face")
	void RestSetA2FTrack(const FString& FileName, bool bIsWakeUp);

	UFUNCTION(BlueprintCallable, Category = "RestAudio2Face")
	void RestGetA2FRange(bool bIsWakeUp);

	UPROPERTY(BlueprintAssignable, Category = "RestAudio2Face")
	FOnAudio2FaceResponseRest OnAudio2FaceResponse;

	UPROPERTY(BlueprintAssignable, Category = "RestAudio2Face")
	FOnAudio2FaceResponseRest OnAudio2FaceResponse_0;

	UPROPERTY(BlueprintAssignable, Category = "RestAudio2Face")
	FOnAudio2FaceResponseRest OnAudio2FaceResponse_0_wakeup;

	UPROPERTY(BlueprintAssignable, Category = "RestAudio2Face")
	FOnAudio2FaceResponseRest OnWakeUpResponse;

	UPROPERTY(BlueprintAssignable, Category = "RestAudio2Face")
	FOnAudioDurationReceivedRest OnAudioDurationReceived_wakeup;

	UPROPERTY(BlueprintAssignable, Category = "RestAudio2Face")
	FOnAudioDurationReceivedRest OnAudioDurationReceived;

	UPROPERTY(BlueprintAssignable, Category = "RestAudio2Face")
	FOnAudio2FaceRequestFailedRest OnAudio2FaceRequestFailed;

	UPROPERTY(BlueprintAssignable, Category = "RestAudio2Face")
	FOnAudio2FaceRequestFailedRest OnWakeUpRequestFailed;

	UFUNCTION(BlueprintCallable, Category = "Level Management")
	void CleanupBeforeLevelChange();

private:
	float RestParseAudioDurationInfo(const FString& ResponseContent);
	void RestGetA2FRange_C(TFunction<void(float)> Callback);
	// ¶¨Ê±Æ÷¾ä±ú
	FTimerHandle TimerHandle;
};
