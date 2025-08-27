#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "A2FRestForWait.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAudio2FaceResponseForWait, const FString&, ResponseContent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAudio2FaceRequestFailedForWait);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAudioDurationReceivedForWait, float, AudioDuration);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class NXAZURESPEECH_API UA2FRestForWait : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UA2FRestForWait();

	UFUNCTION(BlueprintCallable, Category = "A2FRestForwait")
	void Audio2FaceForwait();

	UFUNCTION(BlueprintCallable, Category = "A2FRestForwait")
	void A2FPauseForwait();

	UFUNCTION(BlueprintCallable, Category = "A2FRestForwait")
	void SetA2FTrackForwait(const FString& FileName);

	UFUNCTION(BlueprintCallable, Category = "A2FRestForwait")
	void GetA2FRangeForwait();

	UPROPERTY(BlueprintAssignable, Category = "A2FRestForwait")
	FOnAudio2FaceResponseForWait OnAudio2FaceResponse;

	UPROPERTY(BlueprintAssignable, Category = "A2FRestForwait")
	FOnAudio2FaceResponseForWait OnAudio2FaceResponse_0;

	UPROPERTY(BlueprintAssignable, Category = "A2FRestForwait")
	FOnAudioDurationReceivedForWait OnAudioDurationReceived;

	UPROPERTY(BlueprintAssignable, Category = "A2FRestForwait")
	FOnAudio2FaceRequestFailedForWait OnAudio2FaceRequestFailed;

private:
	float ParseAudioDurationInfoForwait(const FString& ResponseContent);
};
