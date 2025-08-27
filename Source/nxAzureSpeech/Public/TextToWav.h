// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TextToWav.generated.h"


UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class NXAZURESPEECH_API UTextToWav : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UTextToWav();

	UFUNCTION(BlueprintCallable, Category = "Azure Speech")
	void SynthesizeSpeech();


};
